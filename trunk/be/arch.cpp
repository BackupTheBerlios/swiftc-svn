#include "be/arch.h"

#include "me/functab.h"

#include <algorithm>
#include <typeinfo>

namespace be {

/*
 * init global
 */

Arch* arch = 0;

//------------------------------------------------------------------------------

RegAlloc::RegAlloc(me::Function* function)
    : CodePass(function)
{}

void RegAlloc::copyInsertion(me::InstrNode* instrNode)
{
    me::InstrBase* instr = instrNode->value_;

    if ( !instr->isConstrainted() )
        return;

    /* 
     * instruction is constrainted -> check if copies are needed
     */

    // for each constrainted live-through arg
    for (size_t i = 0; i < instr->rhs_.size(); ++i)
    {
        if ( instr->rhs_[i].constraint_ == me::InstrBase::NO_CONSTRAINT )
            continue;

        if ( typeid(*instr->rhs_[i].op_) != typeid(me::Reg) )
            continue;

        me::Reg* reg = (me::Reg*) instr->rhs_[i].op_;

        if ( !instr->livesThrough(reg) )
            continue;

        // -> reg lives-through and is constrained
        int constraint = instr->rhs_[i].constraint_;

        // is there a result with the same constraint?
        for (size_t j = 0; j < instr->lhs_.size(); ++j)
        {
            if ( constraint == instr->lhs_[j].constraint_ )
            {
                // -> copy needed
                //cfg_->instrList_.insert( instrNode->prev()
                
                // it is assumed that is no other reg constrainted to the same color
                // becase of the simple constraint property
                break;
            }
        } // for each res
    } // for each constrainted live-through arg
}

void RegAlloc::faithfulFix(me::InstrNode* instrNode, int typeMask, int numRegs)
{
    me::InstrBase* instr = instrNode->value_;

    int numRegsNeeded = 0;
    int numLhs = 0;
    for (size_t i = 0; i < instr->lhs_.size(); ++i)
    {
        if (instr->lhs_[i].reg_->typeCheck(typeMask) )
            ++numLhs;
    }

    int t = 0;
    int numRhs = 0;
    for (size_t i = 0; i < instr->rhs_.size(); ++i)
    {
        if ( typeid(*instr->rhs_[i].op_) != typeid(me::Reg) )
            continue;

        me::Reg* reg = (me::Reg*) instr->rhs_[i].op_;

        if ( reg->typeCheck(typeMask) )
            ++numRhs;

        if ( instr->livesThrough(reg) )
            ++t;
    }

    int numRegsAllowed = std::max(numLhs + t, numRhs);

    int diff = numRegsAllowed - numRegsNeeded;
    me::RegVec dummies;
    for (int i = 0; i < diff; ++i)
    {
        me::Reg* dummy = function_->newSSA(me::Op::R_INT32);

        // add dummy args
        instr->rhs_.push_back( me::InstrBase::Arg(dummy, -1) );
        dummies.push_back(dummy);
    }
}

//------------------------------------------------------------------------------

CodeGen::CodeGen(me::Function* function)
    : CodePass(function)
{}

} // namespace be
