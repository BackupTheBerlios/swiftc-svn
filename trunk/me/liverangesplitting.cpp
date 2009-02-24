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

#include "me/liverangesplitting.h"

#include <typeinfo>

namespace me {

/*
 * constructor and destructor
 */

LiveRangeSplitting::LiveRangeSplitting(Function* function)
    : CodePass(function)
{}

LiveRangeSplitting::~LiveRangeSplitting()
{
    VDUMAP_EACH(iter, phis_)
        delete iter->second;
}

/*
 * further methods
 */

void LiveRangeSplitting::process()
{
    BBNode* currentBB;

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(LabelInstr) )
        {
            currentBB = cfg_->labelNode2BBNode_[iter];
            continue;
        }

        if ( instr->isConstrained() )
            liveRangeSplit(iter, currentBB);
    }

    // HACK this should be superfluous when there is a good graph implementation
    cfg_->entry_->postOrderIndex_ = 0;     

    // new basic blocks have been inserted, so recompute dominance stuff
    cfg_->calcPostOrder(cfg_->entry_);
    cfg_->calcDomTree();
    cfg_->calcDomFrontier();

    // reconstruct SSA form for the newly inserted phi instructions
    VDUMAP_EACH(iter, phis_)
        cfg_->reconstructSSAForm(iter->second);
}

void LiveRangeSplitting::liveRangeSplit(InstrNode* instrNode, BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;
    InstrBase* instr = instrNode->value_;

    // create new basic block if necessary
    if ( bb->begin_ != instrNode->prev() )
    {
        cfg_->splitBB(instrNode, bbNode);
        swiftAssert(bb->begin_ == instrNode->prev(), "splitting went wrong");
    }

    /* 
     * insert new phi instruction for each live-in var
     */

    VARSET_EACH(iter, instr->liveIn_)
    {
        Var* var = *iter;
        Var* newVar = function_->cloneNewSSA(var);

        // create phi instruction
        swiftAssert( bbNode->pred_.size() == 1, "must have exactly one predecessor" );
        PhiInstr* phi = new PhiInstr(newVar, 1);

        // init sourceBB and arg
        phi->sourceBBs_[0] = bbNode->pred_.first()->value_;
        phi->arg_[0].op_ = newVar;

        // insert instruction
        InstrNode* phiNode = cfg_->instrList_.insert(bb->begin_, phi); 

        /*
         * keep track of def-use stuff
         */

        VarDefUse* vdu;

        // do we already have def-use information for this reg?
        VDUMap::iterator iter = phis_.find(var);
        if ( iter == phis_.end() )
        {
            vdu = new VarDefUse(); 
            vdu->defs_.append( DefUse(var, var->def_.instrNode_, var->def_.bbNode_) ); // orignal definition
            vdu->uses_ = var->uses_; // orignal uses
            phis_[var] = vdu;
        }
        else
            vdu = iter->second;

        vdu->defs_.append( DefUse(newVar, phiNode, bbNode) ); // newly created definition
        vdu->uses_.append( DefUse(var, phiNode, bbNode) ); // newly created use
    }

    bb->fixPointers();
    swiftAssert( bb->hasConstrainedInstr(),
            "this must be the constrained instruction" );
}

} // namespace me
