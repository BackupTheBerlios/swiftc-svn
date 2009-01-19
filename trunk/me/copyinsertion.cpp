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

    // for each constrained arg
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( instr->arg_[i].constraint_ == NO_CONSTRAINT )
            continue;

        /*
         * check whether there is a constrained constant
         */
        if ( typeid(*instr->arg_[i].op_) == typeid(Const) )
        {
            InstrBase* instr = instrNode->value_;
            Const* cst = (Const*) instr->arg_[i].op_;

            // create new result
            Reg* newReg = function_->newSSA(cst->type_);

            // create and insert copy
            AssignInstr* newCopy = new AssignInstr('=', newReg, cst); 
            cfg_->instrList_.insert( instrNode->prev(), newCopy );

            // substitute operand with newReg
            instrNode->value_->arg_[i].op_ = newReg;

            continue;
        }

        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        bool sameArgWithDifferentConstraint = false;

        /*
         * check whether there is the same arg with a different constraint
         */
        for (size_t j = i + 1; j < instr->arg_.size(); ++j)
        {
            if ( instr->arg_[j].constraint_ == NO_CONSTRAINT )
                continue;

            if ( typeid(*instr->arg_[j].op_) != typeid(Reg) )
                continue;

            Reg* reg2 = (Reg*) instr->arg_[j].op_;

            if (reg == reg2 && reg->color_ == reg2->color_)
            {
                insertCopy(i, instrNode);
                sameArgWithDifferentConstraint = true;
            }
        }

        if (sameArgWithDifferentConstraint)
            break; // in this is reg can't be a liveThrough arg

        /*
         * check whether this is a liveThrough arg
         * and has a result with the same constraint
         */
        if ( !instr->livesThrough(reg) )
            continue;

        // is there a result with the same constraint?
        for (size_t j = 0; j < instr->res_.size(); ++j)
        {
            if ( instr->arg_[i].constraint_ == instr->res_[j].constraint_ )
            {
                insertCopy(i, instrNode);
                /*
                 * it is assumed that no other reg is constrained to the 
                 * current color because of the simple constraint property
                 */
                break;
            }
        } // for each res
    } // for each constrained arg
}

void CopyInsertion::insertCopy(size_t regIdx, InstrNode* instrNode)
{
    InstrBase* instr = instrNode->value_;
    swiftAssert( typeid(*instr->arg_[regIdx].op_) == typeid(Reg),
            "must be a Reg here" );
    Reg* reg = (Reg*) instr->arg_[regIdx].op_;

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
    instrNode->value_->arg_[regIdx].op_ = newReg;
}

} // namespace me
