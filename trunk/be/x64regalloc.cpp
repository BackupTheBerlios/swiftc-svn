#include "be/x64regalloc.h"

#include <sstream>
#include <typeinfo>

#include "me/coloring.h"
#include "me/constpool.h"
#include "me/copyinsertion.h"
#include "me/defusecalc.h"
#include "me/functab.h"
#include "me/livenessanalysis.h"
#include "me/liverangesplitting.h"
#include "me/spiller.h"

namespace be {

/*
 * constructor
 */

X64RegAlloc::X64RegAlloc(me::Function* function)
    : me::RegAlloc(function)
    , omitFramePointer_(false)
{}

/*
 * methods
 */

/*
 *  register targeting
 *          |
 *          v
 *  spill general purpose registers
 *          |
 *          v
 *  spill XMM registers
 *          |
 *          v
 *  live range splitting
 *          |
 *          v
 *  color general purpose registers
 *          |
 *          v
 *  color XMM registers
 */
void X64RegAlloc::process()
{
    /*
     * do the register targeting
     */

    registerTargeting();
    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * spill general purpose registers
     */

    me::Colors rColors;
    for (int i = R00; i <= R15; ++i)
        rColors.insert(i);

    // do not use stack pointer as a free register
    rColors.erase(RSP);

    // check whether the frame pointer can be used
    if (omitFramePointer_)
        rColors.erase(RBP);

    me::Spiller( function_, rColors.size(), R_TYPE_MASK ).process();

    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * spill XMM registers
     */

    me::Colors fColors;
    for (int i = XMM00; i <= XMM15; ++i)
        fColors.insert(i);

    me::Spiller( function_, fColors.size(), F_TYPE_MASK ).process();

    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * copy insertion -> faithful fixing -> live range splitting
     */

    me::CopyInsertion(function_).process();
    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();


    /*
     * TODO FaithFulFixing is ignored at the moment !!!!!!
     */
    //me::FaithFulFixing(function_).process();
    // recalulate def-use and liveness stuff
    //me::DefUseCalc(function_).process();
    //me::LivenessAnalysis(function_).process();

    me::LiveRangeSplitting(function_).process();
    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * coloring
     */

    // general purpose registers
    me::Coloring(function_, F_TYPE_MASK, fColors).process();
    // XMM registers
    me::Coloring(function_, R_TYPE_MASK, rColors).process(); 
}

/*
 * forbidden instructions:
 *
 * integer:
 *      r1 = c / r1
 *
 * real:
 *      r1 = c / r1
 */

void X64RegAlloc::registerTargeting()
{
    me::BBNode* currentBB;

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(me::LabelInstr) )
        {
            currentBB = cfg_->labelNode2BBNode_[iter];
            continue;
        }

        // everything's fine for a NOP
        if ( typeid(*instr) == typeid(me::NOP) )
            continue;

        // so it is for a GotoInstr
        if ( typeid(*instr) == typeid(me::GotoInstr) )
            continue;

        if ( typeid(*instr) == typeid(me::AssignInstr) )
        {
            me::AssignInstr* ai = (me::AssignInstr*) instr;
            swiftAssert( ai->res_.size() == 1, "one result must be here" );
            swiftAssert( ai->arg_.size() == 1 || ai->arg_.size() == 2,
                    "one or two args must be here" );

            me::Reg* res = ai->res_[0].reg_;
            me::Op*  op1 = ai->arg_[0].op_;
            me::Op*  op2 = 0;

            if ( ai->arg_.size() == 2 )
                op2 = ai->arg_[1].op_;

            me::Op::Type type = res->type_;

            if (       ai->kind_ == me::AssignInstr::EQ
                    || ai->kind_ == me::AssignInstr::NE
                    || ai->kind_ == me::AssignInstr::LE
                    || ai->kind_ == me::AssignInstr::GE
                    || ai->kind_ == '<'
                    || ai->kind_ == '>')
            {
                // TODO
                continue;
            }

            /*
             * do we really need three regs?
             */

            enum ThreeRegs
            {
                NO,
                PERHAPS,
                YES
            };

            ThreeRegs threeRegs = NO;

            if (ai->arg_.size() > 1)
            {
                swiftAssert( ai->arg_.size() == 2, "must have exactly two args" );
            
                me::InstrBase::OpType opType1 = ai->getOpType(0);
                me::InstrBase::OpType opType2 = ai->getOpType(1);

                if (op1 != op2)
                {
                    if (opType1 != me::InstrBase::CONST && opType2 != me::InstrBase::CONST)
                    {
                        if (opType1 == me::InstrBase::REG && opType2 == me::InstrBase::REG)
                            threeRegs = YES;
                        else if (opType1 == me::InstrBase::REG && opType2 == me::InstrBase::REG_DEAD)
                            threeRegs = PERHAPS;
                    }
                }
            }

            if (threeRegs == YES)
                insertNOP(iter);
            else 
                if (threeRegs == PERHAPS)
            {
                /*
                 * insert artificial use for all critical cases
                 */

                if ( (ai->kind_ == '/')
                     || ( (type == me::Op::R_REAL32 || type == me::Op::R_REAL64) 
                         && (ai->kind_ == '-')) )
                {
                    insertNOP(iter);
                }
            }

            if (type == me::Op::R_REAL32 || type == me::Op::R_REAL64)
            {
                // TODO remove copy&paste code
                if ( typeid(*op1) == typeid(me::Const) )
                {
                    me::Const* cst = (me::Const*) op1;

                    if (type == me::Op::R_REAL32)
                        me::constpool->insert(cst->value_.real32_);
                    else
                        me::constpool->insert(cst->value_.real64_);
                }

                if ( op2 && typeid(*op2) == typeid(me::Const) )
                {
                    me::Const* cst = (me::Const*) op2;

                    if (type == me::Op::R_REAL64)
                        me::constpool->insert(cst->value_.real32_);
                    else
                        me::constpool->insert(cst->value_.real64_);
                }
                
                continue;
            }

            /*
             * do register targeting on mul, imul, div and idiv instructions
             */

            if (ai->kind_ == '*' || ai->kind_ == '/')
            {
                /*
                 * insert constants
                 */
                // TODO remove copy&paste code
                if ( typeid(*op1) == typeid(me::Const) )
                {
                    me::Const* cst = (me::Const*) op1;

                    if (type == me::Op::R_INT8 || type == me::Op::R_UINT8)
                        me::constpool->insert(cst->value_.uint8_);
                    else if (type == me::Op::R_INT16 || type == me::Op::R_UINT16)
                        me::constpool->insert(cst->value_.uint16_);
                    else if (type == me::Op::R_INT32 || type == me::Op::R_UINT32)
                        me::constpool->insert(cst->value_.uint32_);
                    else
                        me::constpool->insert(cst->value_.uint64_);
                }

                if ( op2 && typeid(*op2) == typeid(me::Const) )
                {
                    me::Const* cst = (me::Const*) op2;

                    if (type == me::Op::R_INT8 || type == me::Op::R_UINT8)
                        me::constpool->insert(cst->value_.uint8_);
                    else if (type == me::Op::R_INT16 || type == me::Op::R_UINT16)
                        me::constpool->insert(cst->value_.uint16_);
                    else if (type == me::Op::R_INT32 || type == me::Op::R_UINT32)
                        me::constpool->insert(cst->value_.uint32_);
                    else
                        me::constpool->insert(cst->value_.uint64_);
                }

                /*
                 * constrain properly
                 */
                ai->constrain();
                ai->arg_[0].constraint_ = RAX;
                ai->res_[0].constraint_ = RAX;

                me::Reg* newReg = function_->newSSA(type);

                // int8 and uint8 muls just go to ax and not al:dl
                if (type != me::Op::R_INT8 && type != me::Op::R_UINT8)
                {
                    if (ai->kind_ == '*')
                    {
                        // newReg is defined here
                        ai->res_.push_back( me::Res(newReg, 0, RDX) );
                    }
                    else
                    {
                        // define newReg with undef
                        cfg_->instrList_.insert( iter->prev(),
                                new me::AssignInstr('=', newReg, new me::Undef(type)) );
                        ai->arg_.push_back( me::Arg(newReg, RDX) );
                    }
                }
            }
        } // if AssignInstr
    } // for each instruction
}

void X64RegAlloc::insertNOP(me::InstrNode* instrNode)
{
    swiftAssert( typeid(*instrNode->value_) == typeid(me::AssignInstr),
            "must be an AssignInstr here");
    me::AssignInstr* ai = (me::AssignInstr*) instrNode->value_;

    swiftAssert( typeid(*ai->arg_[1].op_) == typeid(me::Reg),
            "must be a Reg here" );
    swiftAssert( ai->arg_.size() == 2, "must have exactly two args");
    me::Reg* reg = (me::Reg*) ai->arg_[1].op_;

    cfg_->instrList_.insert( instrNode, new me::NOP(reg) );
}

std::string X64RegAlloc::reg2String(const me::Reg* reg)
{
    me::Op::Type type = reg->type_;
    int color = reg->color_;

    swiftAssert(color >= 0, "no valid color here");

    std::ostringstream oss;

    // prepend leading per cent sign for registers
    oss << '%';

    switch (type)
    {
        case me::Op::R_BOOL: 
        case me::Op::R_INT8: 
        case me::Op::R_UINT8: 
            switch (color)
            {
                case RAX: oss << "al"; break;
                case RBX: oss << "bl"; break;
                case RCX: oss << "cl"; break;
                case RDX: oss << "dl"; break;
                case RSP: oss << "spl"; break;
                case RBP: oss << "bpl"; break;
                case RSI: oss << "sil"; break;
                case RDI: oss << "dil"; break;
                case R08: oss << "r8b"; break;
                case R09: oss << "r9b"; break;
                case R10: oss << "r10b"; break;
                case R11: oss << "r11b"; break;
                case R12: oss << "r12b"; break;
                case R13: oss << "r13b"; break;
                case R14: oss << "r14b"; break;
                case R15: oss << "r15b"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;

        case me::Op::R_INT16: 
        case me::Op::R_UINT16: 
            switch (color)
            {
                case RAX: oss << "ax"; break;
                case RBX: oss << "bx"; break;
                case RCX: oss << "cx"; break;
                case RDX: oss << "dx"; break;
                case RSP: oss << "sp"; break;
                case RBP: oss << "bp"; break;
                case RSI: oss << "si"; break;
                case RDI: oss << "di"; break;
                case R08: oss << "r8w"; break;
                case R09: oss << "r9w"; break;
                case R10: oss << "r10w"; break;
                case R11: oss << "r11w"; break;
                case R12: oss << "r12w"; break;
                case R13: oss << "r13w"; break;
                case R14: oss << "r14w"; break;
                case R15: oss << "r15w"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;

        case me::Op::R_INT32:
        case me::Op::R_UINT32:
            switch (color)
            {
                case RAX: oss << "eax"; break;
                case RBX: oss << "ebx"; break;
                case RCX: oss << "ecx"; break;
                case RDX: oss << "edx"; break;
                case RSP: oss << "esp"; break;
                case RBP: oss << "ebp"; break;
                case RSI: oss << "esi"; break;
                case RDI: oss << "edi"; break;
                case R08: oss << "r8d"; break;
                case R09: oss << "r9d"; break;
                case R10: oss << "r10d"; break;
                case R11: oss << "r11d"; break;
                case R12: oss << "r12d"; break;
                case R13: oss << "r13d"; break;
                case R14: oss << "r14d"; break;
                case R15: oss << "r15d"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;

        case me::Op::R_INT64:
        case me::Op::R_UINT64:
            switch (color)
            {
                case RAX: oss << "rax"; break;
                case RBX: oss << "rbx"; break;
                case RCX: oss << "rcx"; break;
                case RDX: oss << "rdx"; break;
                case RSP: oss << "rsp"; break;
                case RBP: oss << "rbp"; break;
                case RSI: oss << "rsi"; break;
                case RDI: oss << "rdi"; break;
                case R08: oss << "r8"; break;
                case R09: oss << "r9"; break;
                case R10: oss << "r10"; break;
                case R11: oss << "r11"; break;
                case R12: oss << "r12"; break;
                case R13: oss << "r13"; break;
                case R14: oss << "r14"; break;
                case R15: oss << "r15"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;
            
        case me::Op::R_REAL32:
        case me::Op::R_REAL64:
            switch (color)
            {
                case XMM00: oss << "xmm0"; break;
                case XMM01: oss << "xmm1"; break;
                case XMM02: oss << "xmm2"; break;
                case XMM03: oss << "xmm3"; break;
                case XMM04: oss << "xmm4"; break;
                case XMM05: oss << "xmm5"; break;
                case XMM06: oss << "xmm6"; break;
                case XMM07: oss << "xmm7"; break;
                case XMM08: oss << "xmm8"; break;
                case XMM09: oss << "xmm9"; break;
                case XMM10: oss << "xmm10"; break;
                case XMM11: oss << "xmm11"; break;
                case XMM12: oss << "xmm12"; break;
                case XMM13: oss << "xmm13"; break;
                case XMM14: oss << "xmm14"; break;
                case XMM15: oss << "xmm15"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;

        default:
            swiftAssert( false, "unreachable code" );
    } // switch (type)

    return oss.str();
}

} // namespace be
