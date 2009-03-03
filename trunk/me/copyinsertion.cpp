/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "me/copyinsertion.h"

#include <typeinfo>

#include "me/cfg.h"

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
        Const* cst = dynamic_cast<Const*>( instr->arg_[i].op_ );
        if (cst)
        {
            // create new result
            Var* newVar = function_->newSSAReg(cst->type_);

            // create and insert copy
            AssignInstr* newCopy = new AssignInstr('=', newVar, cst); 
            cfg_->instrList_.insert( instrNode->prev(), newCopy );

            // substitute operand with newReg
            instrNode->value_->arg_[i].op_ = newVar;

            continue;
        }

        Reg* reg = dynamic_cast<Reg*>( instr->arg_[i].op_ );
        if (!reg)
            continue;

        bool sameArgWithDifferentConstraint = false;

        /*
         * check whether there is the same arg with a different constraint
         */
        for (size_t j = i + 1; j < instr->arg_.size(); ++j)
        {
            if ( instr->arg_[j].constraint_ == NO_CONSTRAINT )
                continue;

            Reg* reg2 = dynamic_cast<Reg*>( instr->arg_[j].op_ );
            if (!reg2)
                continue;

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
    Var* newVar = function_->cloneNewSSA(reg);

    // create and insert copy
    AssignInstr* newCopy = new AssignInstr('=', newVar, reg); 
    cfg_->instrList_.insert( instrNode->prev(), newCopy );

    // substitute operand with newReg
    instrNode->value_->arg_[regIdx].op_ = newVar;
}

} // namespace me
