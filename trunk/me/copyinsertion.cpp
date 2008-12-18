#include "me/copyinsertion.h"

#include <typeinfo>

namespace me {

/*
 * constructor 
 */

CopyInsertion::CopyInsertion(Function* function)
    : CodePass(function)
{}

/*
 * further methods
 */

void CopyInsertion::process()
{
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( instr->isConstrained() )
            insertIfNecessary(iter);
    }
}

void CopyInsertion::insertIfNecessary(InstrNode* instrNode)
{
    InstrBase* instr = instrNode->value_;

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

                // create and insert copy
                AssignInstr* newCopy = new AssignInstr('=', newReg, reg); 
                cfg_->instrList_.insert( instrNode->prev(), newCopy );

                // substitute operand with newReg
                instr->arg_[i].op_ = newReg;
                
                // transfer constraint to new instruction
                newCopy->arg_[0].constraint_ = constraint;
                instr->arg_[i].constraint_ = InstrBase::NO_CONSTRAINT;
                
                /*
                 * it is assumed that no other reg is constrained to the same color
                 * because of the simple constraint property
                 */
                break;
            }
        } // for each res
    } // for each constrainted live-through arg

    // try to unconstrain instruction
    instr->unconstrainIfPossible();
}

} // namespace me
