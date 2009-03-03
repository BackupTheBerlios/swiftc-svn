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

#include "me/defusecalc.h"

#include <typeinfo>

#include "me/cfg.h"

namespace me {

/*
 * constructor
 */

DefUseCalc::DefUseCalc(Function* function)
    : CodePass(function)
{}

/*
 * further methods
 */

void DefUseCalc::process()
{
    /*
     * clean up if necessary
     */
    if (function_->firstDefUse_)
    {
        VARMAP_EACH(iter, function_->vars_)
        {
            Var* var = iter->second;
            var->uses_.clear();
        }
    }

    calcDef();
    calcUse();

    /* 
     * use this for debugging of the def-use calculation
     */
#if 0
    std::cout << "--- DEFUSE STUFF ---" << std::endl;
    REGMAP_EACH(iter, function_->vars_)
    {
        Var* var = iter->second;
        std::cout << var->def_.instrNode_->value_->toString() << std::endl;
        USELIST_EACH(iter, var->uses_)
            std::cout << "\t" << iter->value_.instrNode_->value_->toString() << std::endl;
    }
#endif

    function_->firstDefUse_ = true;
}

void DefUseCalc::calcDef()
{
    // knows the current BB in the iteration
    BBNode* currentBB;

    // iterate over the instruction list 
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(LabelInstr) )
            currentBB = cfg_->labelNode2BBNode_[iter]; // new basic block
        else
        {
            // for each var on the res
            for (size_t i = 0; i < instr->res_.size(); ++i)
            {
                Var* var = instr->res_[i].var_;
                var->def_.set(var, iter, currentBB); // store def
            }
        }
    }
}

void DefUseCalc::calcUse()
{
    // for each var
    VARMAP_EACH(iter, function_->vars_)
    {
        Var* var = iter->second;

        /* 
         * start with the definition of the var and walk the dominator tree
         * to find all uses since every use of a var in an SSA form program is
         * dominated by its definition except for phi-instructions.
         */
        calcUse(var, var->def_.bbNode_);
    }

    // now find all phi functions
    CFG_RELATIVES_EACH(bbIter, cfg_->nodes_)
    {
        BBNode* bbNode = bbIter->value_;
        BasicBlock* bb = bbNode->value_;

        for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
        {
            swiftAssert(typeid(*iter->value_) == typeid(PhiInstr),
                "must be a PhiInstr here");
            PhiInstr* phi = (PhiInstr*) iter->value_;

            for (size_t i = 0; i < phi->arg_.size(); ++i)
            {
                Var* var = dynamic_cast<Var*>( phi->arg_[i].op_ );
                swiftAssert(var, "must be a Var here");

                // put this as first use so liveness analysis will be a bit faster
                var->uses_.prepend( DefUse(var, iter, bbNode) );
            }
        }
    }
}

void DefUseCalc::calcUse(Var* var, BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;

    /* 
     * iterate over the instruction list in this bb and find all uses 
     * while ignoring phi functions
     */
    for (InstrNode* iter = bb->firstOrdinary_; iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        if ( instr->isVarUsed(var) )
        {
            var->uses_.append( DefUse(var, iter, bbNode) );
        }
    } // for each instruction

    // for each child of bb in the dominator tree
    BBLIST_EACH(iter, bb->domChildren_)
        calcUse(var, iter->value_);
}

} // namespace me
