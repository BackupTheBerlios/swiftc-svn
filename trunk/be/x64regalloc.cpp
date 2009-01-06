#include "be/x64regalloc.h"

#include <sstream>
#include <typeinfo>

#include "me/coloring.h"
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

            me::Op::Type type = instr->arg_[0].op_->type_;

            if (ai->arg_.size() == 1)
                continue; // everything's fine in this case

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
            me::InstrBase::OpType opType1 = ai->getOpType(0);
            me::InstrBase::OpType opType2 = ai->getOpType(1);

            if (ai->arg_[0].op_ != ai->arg_[1].op_)
            {
                if (opType1 != me::InstrBase::CONST && opType2 != me::InstrBase::CONST)
                {
                    if (opType1 == me::InstrBase::REG && opType2 == me::InstrBase::REG)
                        threeRegs = YES;
                    else if (opType1 == me::InstrBase::REG && opType2 == me::InstrBase::REG_DEAD)
                        threeRegs = PERHAPS;
                }
            }

            if (threeRegs == PERHAPS)
            {
                /*
                 * insert artificial use for all critical cases
                 */

                if (     (ai->kind_ == '/')
                     || ( (type == me::Op::R_REAL32 || type == me::Op::R_REAL64) 
                         && (ai->kind_ == '-')) )
                {
                    insertNOP(iter);
                }
            }

            /*
             * do register targeting on mul, imul, div and idiv instructions
             */

            if (type == me::Op::R_REAL32 || type == me::Op::R_REAL64)
                continue;

            if (ai->kind_ == '*' || ai->kind_ == '/')
            {
                // constraint properly
                ai->constrain();
                ai->arg_[0].constraint_ = RAX;
                ai->res_[0].constraint_ = RAX;
            }
        } // if AssignInstr
    } // for each instruction
}

void X64RegAlloc::insertNOP(me::InstrNode* instrNode)
{
    swiftAssert( typeid(*instrNode->value_) == typeid(me::AssignInstr),
            "must be an AssignInstr here");
    me::AssignInstr* ai = (me::AssignInstr*) instrNode->value_;

    swiftAssert( ai->arg_.size() == 2, "must have exactly two args");
    me::Reg* reg = (me::Reg*) ai->arg_[1].op_;

    swiftAssert( typeid(*ai->arg_[1].op_) == typeid(me::Reg),
            "must be a Reg here" );
    swiftAssert( ai->getOpType(1) == me::InstrBase::REG_DEAD,
            "must not be in the live out of this instr" );

    cfg_->instrList_.insert( instrNode, new me::NOP(reg) );
}

std::string X64RegAlloc::reg2String(int reg)
{
    swiftAssert(reg >= 0, "no valid color here");

    std::ostringstream oss;

    switch (reg)
    {
        case RAX: oss << "rax"; break;
        case RBX: oss << "rbx"; break;
        case RCX: oss << "rcx"; break;
        case RDX: oss << "rdx"; break;
        case RBP: oss << "rbp"; break;
        case RSI: oss << "rsi"; break;
        case RDI: oss << "rdi"; break;
        case RSP: oss << "rsp"; break;
        case R08: oss << "r08"; break;
        case R09: oss << "r09"; break;
        case R10: oss << "r10"; break;
        case R11: oss << "r11"; break;
        case R12: oss << "r12"; break;
        case R13: oss << "r13"; break;
        case R14: oss << "r14"; break;
        case R15: oss << "r15"; break;

        case XMM00: oss << "xmm00"; break;
        case XMM01: oss << "xmm01"; break;
        case XMM02: oss << "xmm02"; break;
        case XMM03: oss << "xmm03"; break;
        case XMM04: oss << "xmm04"; break;
        case XMM05: oss << "xmm05"; break;
        case XMM06: oss << "xmm06"; break;
        case XMM07: oss << "xmm07"; break;
        case XMM08: oss << "xmm08"; break;
        case XMM09: oss << "xmm09"; break;
        case XMM10: oss << "xmm10"; break;
        case XMM11: oss << "xmm11"; break;
        case XMM12: oss << "xmm12"; break;
        case XMM13: oss << "xmm13"; break;
        case XMM14: oss << "xmm14"; break;
        case XMM15: oss << "xmm15"; break;

        default:
            swiftAssert( false, "unreachable code" );
    }

    return oss.str();
}

} // namespace be
