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

#include "me/vectorizer.h"

#include "me/cfg.h"
#include "me/functab.h"

namespace me {

/*
 * constructor
 */

Vectorizer::Vectorizer(Function* function)
    : CodePass(function)
    , simdFunction_( me::functab->insertFunction(
                new std::string(*function->id_ + "simd"), false) )
{}

/*
 * virtual methods
 */

void Vectorizer::process()
{
    // map function_->lastLabelNode_ to simdFunction_->lastLabelNode_
    src2dstLabel_[function_->lastLabelNode_] = simdFunction_->lastLabelNode_;

    // for each instruction
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        currentInstrNode_ = iter;
        InstrBase* instr = iter->value_;

        InstrNode* simdInstr = instr->toSimd(this);
        swiftAssert(simdInstr, "is 0");
        simdFunction_->instrList_.append(simdInstr);
    }

    simdFunction_->cfg_->calcCFG();
    simdFunction_->cfg_->calcDomTree();
    simdFunction_->cfg_->calcDomFrontier();

    // find and eliminate all if-else clauses
    eliminateIfElseClauses(simdFunction_->cfg_->entry_);

    // HACK this should be superfluous when there is a good graph implementation
    simdFunction_->cfg_->entry_->postOrderIndex_ = 0;     

    // new basic blocks have been inserted, so recompute dominance stuff
    simdFunction_->cfg_->calcPostOrder(simdFunction_->cfg_->entry_);
    simdFunction_->cfg_->calcDomTree();
    simdFunction_->cfg_->calcDomFrontier();

    simdFunction_->cfg_->placePhiFunctions();
    simdFunction_->cfg_->renameVars();
}

/*
 * further methods
 */

Function* Vectorizer::getSimdFunction()
{
    return simdFunction_;
}

Function* Vectorizer::function()
{
    return function_;
}

int Vectorizer::getSimdLength()
{
    // TODO
    return 4;
}

void Vectorizer::eliminateIfElseClauses(BBNode* bbNode)
{
    /*
     * do a post-order walk of the dominance tree so we start with the most
     * nested one
     */

    BasicBlock* bb = bbNode->value_;

    BBLIST_EACH(iter, bbNode->value_->domChildren_)
    {
        BBNode* current = iter->value_;
        eliminateIfElseClauses(current);
    }

    if (bbNode->succ_.empty() || bbNode->succ_.size() == 1)
        return;

    swiftAssert( bbNode->succ_.size() == 2, "more than 2 successor are not allowed" );
    swiftAssert( typeid(*bb->end_->prev_->value_) == typeid(BranchInstr),
            "last instruction must be a BranchInstr" );

    BranchInstr* branch = (BranchInstr*) bb->end_->prev_->value_;

    BBNode*   ifChildNode = branch->bbTargets_[BranchInstr::TRUE_TARGET];
    BBNode* elseChildNode = branch->bbTargets_[BranchInstr::FALSE_TARGET];
    BasicBlock*   ifChild =   ifChildNode->value_;
    BasicBlock* elseChild = elseChildNode->value_;

    swiftAssert( !  ifChild->domFrontier_.empty(), "must not be empty" );
    swiftAssert( !elseChild->domFrontier_.empty(), "must not be empty" );

    if (ifChild->domFrontier_.size() != 1)
        return;
    if (elseChild->domFrontier_.size() != 1)
        return;

    if ( ifChild->domFrontier_.first()->value_ != elseChild->domFrontier_.first()->value_ )
        return;

    // this is the node where the control flow merges after the division by bb
    BBNode* nextNode = ifChild->domFrontier_.first()->value_;
    //BasicBlock* next = nextNode->value_;

    swiftAssert( nextNode->pred_.size() == 2, "must exactly have two predecessors" );

    // find last if-branch block
    BBNode* lastIfNode = ifChildNode; 
    while (lastIfNode->succ_.first()->value_ != nextNode) 
    {
        swiftAssert(lastIfNode->succ_.size() == 1, "must exactly have 1 successor");
        swiftAssert(!lastIfNode->value_->hasPhiInstr(), "must not have phi functions");

        // move forward
        lastIfNode = lastIfNode->succ_.first()->value_;
    }
    BasicBlock* lastIf = lastIfNode->value_;

    // find last else-branch block
    BBNode* lastElseNode = elseChildNode; 
    while (lastElseNode->succ_.first()->value_ != nextNode) 
    {
        swiftAssert(lastElseNode->succ_.size() == 1, "must exactly have 1 successor");
        swiftAssert(!lastElseNode->value_->hasPhiInstr(), "must not have phi functions");

        // move forward
        lastElseNode = lastElseNode->succ_.first()->value_;
    }
    BasicBlock* lastElse = lastElseNode->value_;

    /*
     * We now have this situation:
     *
     *                 +----------+
     *                 |    bb    |
     *                 +----------+
     *                   |      |
     *                   v      v
     *          +---------+  +-----------+
     *          | ifChild |  | elseChild |
     *          +---------+  +-----------+
     *               |             |
     *               v             v
     *              ...           ... 
     *          +---------+  +-----------+
     *          | lastIf  |  | lastElse  |
     *          +---------+  +-----------+
     *                   |      |
     *                   v      v
     *                 +----------+
     *                 |   next   |
     *                 +----------+
     *
     * Make this linear:
     *
     *                 +-----------+
     *                 |    bb     |
     *                 +-----------+
     *                       |
     *                       v
     *                 +-----------+
     *                 |  ifChild  |
     *                 +-----------+
     *                       |
     *                       v
     *                      ...
     *                 +-----------+
     *                 |  lastIf   |
     *                 +-----------+
     *                       |
     *                       v
     *                 +-----------+
     *                 | elseChild |
     *                 +-----------+
     *                       |
     *                       v
     *                      ...
     *                 +-----------+
     *                 | lastElse  |
     *                 +-----------+
     *                       |
     *                       v
     *                 +-----------+
     *                 |   next    |
     *                 +-----------+
     */

    // erase bb's elseChild successor
    bbNode->succ_.erase( bbNode->succ_.find(elseChildNode) );

    // erase elseChild's predecessor
    swiftAssert( elseChildNode->pred_.size() == 1, "must have exactly one predecessor" );
    elseChildNode->pred_.clear();

    // erase lastIf's successor
    swiftAssert( lastIfNode->succ_.size() == 1, "must have exactly one successor" );
    lastIfNode->succ_.clear();

    // erase next's lastIf predecessor
    nextNode->pred_.erase( nextNode->pred_.find(lastIfNode) );

    // link lastIf to elseChild
    lastIfNode->link(elseChildNode);

    /*
     * now subtitute instructions
     */

    // remove branch
    simdFunction_->cfg_->instrList_.erase( bb->end_->prev_ );
    bb->fixPointers();

    // remove gotos
    if ( typeid(*lastIf->end_->prev_->value_) == typeid(GotoInstr) )
    {
        simdFunction_->cfg_->instrList_.erase(lastIf->end_->prev_);
    }

    if ( typeid(*lastElse->end_->prev_->value_) == typeid(GotoInstr) )
    {
        simdFunction_->cfg_->instrList_.erase(lastElse->end_->prev_);
    }

    // add mask stuff
    //next->begin_
}

} // namespace me
