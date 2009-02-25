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

#include "me/stackcoloring.h"

#include <algorithm>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"
#include "me/op.h"
#include "me/stacklayout.h"

namespace me {

/*
 * constructors
 */

StackColoring::StackColoring(Function* function)
    : CodePass(function)
    , colorCounter_(0)
{}

/*
 * methods
 */

void StackColoring::process()
{
    /*
     * start with the first true basic block
     * and perform a pre-order walk of the dominator tree
     */

    colorRecursive(cfg_->entry_);
}

void StackColoring::colorRecursive(BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;

    // for each phi function in this bb
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;
        swiftAssert( typeid(*instr) == typeid(PhiInstr), "must be a PhiInstr here" );
        MemVar* phiRes = dynamic_cast<MemVar*>( instr->res_[0].var_ );
        if (!phiRes)
            continue;

        int color = ((MemVar*) instr->arg_[0].op_)->color_;

#ifdef SWIFT_DEBUG
        // check in the debug version whether all phi-args have the same color
        for (size_t i = 1; i < instr->arg_.size(); ++i)
            swiftAssert( ((Reg*) instr->arg_[i].op_)->color_ == color,
                    "colors not identical here" );
#endif // SWIFT_DEBUG

        phiRes->color_ = color;
    }

    // for each ordinary instruction in this bb
    for (InstrNode* iter = bb->firstOrdinary_; iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        // for each var on the left hand side -> assign a color for result
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            Var* var = instr->res_[i].var_;

            // only stack locations are considered here
            if ( var->type_ != Op::R_STACK )
                continue;

            swiftAssert( typeid(*var) == typeid(MemVar), "must be a MemVar here" );

            if ( typeid(*instr) == typeid(Store) )
            {
                swiftAssert( typeid(*instr->arg_[1].op_) == typeid(MemVar),
                        "must be a Reg here" );
                var->color_ = ((MemVar*) instr->arg_[1].op_)->color_;
            }
            else
            {
                // update the stackLayout_ and fetch color
                function_->stackLayout_->appendMem( (MemVar*) var );
            }
        }
    } // for each instruction

    // for each child of bb in the dominator tree
    BBLIST_EACH(bbIter, bb->domChildren_)
    {
        BBNode* domChild = bbIter->value_;
        colorRecursive(domChild);
    }
}

} // namespace me
