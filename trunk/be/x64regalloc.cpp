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
    if (!omitFramePointer_)
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
    me::Coloring(function_, R_TYPE_MASK, rColors).process();

    // XMM registers
    me::Coloring(function_, F_TYPE_MASK, fColors).process(); 
}

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

            me::Op::Type type = ai->res_[0].reg_->type_;

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
             * forbidden instructions:
             * r1 =  c / r1     ( reg = c / reg_dead )
             *      rewrite as:
             *      tmp = c
             *      a = tmp / b
             *
             * r1 = r2 / r1     ( reg = reg / reg_dead)
             *      rewrite as:
             *      a = b / c
             *      NOP(c)
             *      
             */

            if (ai->kind_ == '/')
            {
                swiftAssert( ai->arg_.size() == 2, "must have exactly two args" );
            
                me::InstrBase::OpType opType1 = ai->getOpType(0);
                me::InstrBase::OpType opType2 = ai->getOpType(1);

                if (opType2 == me::InstrBase::REG_DEAD)
                {
                    if (opType1 == me::InstrBase::CONST)
                    {
                        swiftAssert( typeid(*ai->arg_[1].op_) == typeid(me::Const), 
                                "must be a Const here");
                        me::Const* cst = (me::Const*) ai->arg_[1].op_;

                        // create reg which will hold the constant, the assignment and insert
                        me::Reg* newReg = function_->newSSA(type);
                        me::AssignInstr* newCopy = new me::AssignInstr('=', newReg, cst);
                        cfg_->instrList_.insert( iter->prev(), newCopy );

                        // substitute operand with newReg
                        instr->arg_[1].op_ = newReg;

                        currentBB->value_->fixPointers();
                    }
                    else if (opType1 == me::InstrBase::REG)
                    {
                        // insert artificial use for all critical cases
                        me::Reg* reg = (me::Reg*) ai->arg_[1].op_;
                        cfg_->instrList_.insert( iter, new me::NOP(reg) );

                        currentBB->value_->fixPointers();
                    }
                } // if op2 REG_DEAD
            } // if div

            /*
             * do register targeting on mul, div and idiv instructions
             */

            if (type == me::Op::R_REAL32 || type == me::Op::R_REAL64)
                continue;

            if (ai->kind_ == '*' || ai->kind_ == '/')
            {
                if ( ai->kind_ == '*' && 
                        (type == me::Op::R_INT8  || type == me::Op::R_INT16 || 
                         type == me::Op::R_INT32 || type == me::Op::R_INT64) )
                {
                    // this is signed int = signed int * signed int
                    continue; // -> everything's fine
                }
                
                /*
                 * constrain properly
                 */
                ai->constrain();
                ai->arg_[0].constraint_ = RAX;
                ai->res_[0].constraint_ = RAX;

                // int8 and uint8 just go to ax/al/ah and not dl:al or something
                if (type != me::Op::R_INT8 && type != me::Op::R_UINT8)
                {
                    // RDX is destroyed in all cases
                    ai->res_.push_back( me::Res(function_->newSSA(type), 0, RDX) );

                    if (ai->kind_ == '/')
                    {
                        // no input reg may be RDX

                        // define a new Reg which is initialized with undef
                        me::Reg* dummy = function_->newSSA(type);
                        cfg_->instrList_.insert( iter->prev(), 
                            new me::AssignInstr('=', dummy, new me::Undef(type)) );

                        // avoid that RDX is spilled here
                        ai->arg_.push_back( me::Arg(dummy, RDX) );

                        currentBB->value_->fixPointers();
                    }
                }
            }
        } // if AssignInstr
    } // for each instruction
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
