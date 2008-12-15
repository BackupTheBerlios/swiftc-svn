#include "me/arch.h"

#include "me/functab.h"

#include <algorithm>
#include <typeinfo>

namespace me {

//------------------------------------------------------------------------------

/*
 * init global
 */

Arch* arch = 0;

//------------------------------------------------------------------------------

RegAlloc::RegAlloc(Function* function)
    : CodePass(function)
{}

void RegAlloc::copyInsertion(InstrNode* instrNode)
{
    InstrBase* instr = instrNode->value_;

    if ( !instr->hasConstraint() )
        return;

    /* 
     * instruction is constrainted -> check if copies are needed
     */

    // for each constrainted live-through arg
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( instr->arg_[i].constraint_ == InstrBase::NO_CONSTRAINT )
            continue;

        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        if ( !instr->livesThrough(reg) )
            continue;

        // -> reg lives-through and is constrained
        int constraint = instr->arg_[i].constraint_;

        // is there a result with the same constraint?
        for (size_t j = 0; j < instr->res_.size(); ++j)
        {
            if ( constraint == instr->res_[j].constraint_ )
            {
                // -> copy needed

                // create new result
#ifdef SWIFT_DEBUG
                Reg* newReg = function_->newSSA(reg->type_, &reg->id_);
#else // SWIFT_DEBUG
                Reg* newReg = function_->newSSA(reg->type_);
#endif // SWIFT_DEBUG

                // insert copy
                cfg_->instrList_.insert( instrNode->prev(), new AssignInstr('=', newReg, reg) );

                // substitute operand with newReg
                instr->arg_[i].op_ = newReg;
                
                /*
                 * it is assumed that is no other reg constrainted to the same color
                 * becase of the simple constraint property
                 */
                break;
            }
        } // for each res
    } // for each constrainted live-through arg
}

void RegAlloc::faithfulFix(InstrNode* instrNode, int typeMask, int numRegs)
{
    InstrBase* instr = instrNode->value_;

    int numRegsNeeded = 0;

    // count number of results with proper type
    int numLhs = 0;
    for (size_t i = 0; i < instr->res_.size(); ++i)
    {
        if (instr->res_[i].reg_->typeCheck(typeMask) )
            ++numLhs;
    }

    int numLiveThrough = 0;

    // count number of args with proper type and number of live-through vars
    int numRhs = 0;
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        if ( reg->typeCheck(typeMask) )
            ++numRhs;

        if ( instr->livesThrough(reg) )
            ++numLiveThrough;
    }

    int numRegsAllowed = std::max(numLhs + numLiveThrough, numRhs);

    int diff = numRegsAllowed - numRegsNeeded;
    RegVec dummies;
    for (int i = 0; i < diff; ++i)
    {
        Reg* dummy = function_->newSSA(Op::R_INT32);

        // add dummy args
        instr->arg_.push_back( InstrBase::Arg(dummy, -1) );
        dummies.push_back(dummy);
    }
}


//------------------------------------------------------------------------------

CodeGen::CodeGen(Function* function)
    : CodePass(function)
{}

} // namespace me
