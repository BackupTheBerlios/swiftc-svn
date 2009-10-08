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
    , simdFunction_( functab->insertFunction(
                new std::string(*function->id_ + "simd"), false) )
{}

/*
 * virtual methods
 */

void Vectorizer::process()
{
    // map function_->functionEpilogue_ to simdFunction_->functionEpilogue_
    src2dstLabel_[function_->functionEpilogue_] = simdFunction_->functionEpilogue_;

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

    // for each label instruction
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        if ( typeid(*iter->value_) != typeid(LabelInstr) )
            continue;

        swiftAssert( cfg_->labelNode2BBNode_.contains(iter), "must be found here" );
        swiftAssert( src2dstLabel_.contains(iter), "must be found here" );
        swiftAssert( simdFunction_->cfg_->labelNode2BBNode_.contains( 
                    src2dstLabel_[iter]), "must be found here" );

        // create map: srcBB -> dstBB
        src2dstBBNode_[ cfg_->labelNode2BBNode_[iter] ] = 
            simdFunction_->cfg_->labelNode2BBNode_[ src2dstLabel_[iter] ];
    }

    // for each phi function
    INSTRLIST_EACH(iter, simdFunction_->instrList_)
    {
        PhiInstr* phi = dynamic_cast<PhiInstr*>(iter->value_);
        if (!phi)
            continue;

        for (size_t i = 0; i < phi->arg_.size(); ++i)
        {
            swiftAssert( src2dstBBNode_.contains(phi->sourceBBs_[i]),
                        "must be found here" );
            phi->sourceBBs_[i] = src2dstBBNode_[ phi->sourceBBs_[i] ];
        }
    }

    // for each BBNode
    CFG_RELATIVES_EACH(iter, simdFunction_->cfg_->nodes_)
        iter->value_->value_->fixPointers();

    simdFunction_->cfg_->calcDomTree();
    simdFunction_->cfg_->calcDomFrontier();
    simdFunction_->cfg_->findLoops();

    // find and eliminate all if-else clauses
    eliminateIfElseClauses(simdFunction_->cfg_->entry_);

    // HACK this should be superfluous when there is a good graph implementation
    simdFunction_->cfg_->entry_->postOrderIndex_ = 0;     

    // new basic blocks have been inserted, so recompute dominance stuff
    simdFunction_->cfg_->calcPostOrder(simdFunction_->cfg_->entry_);
    simdFunction_->cfg_->calcDomTree();
    simdFunction_->cfg_->calcDomFrontier();
    simdFunction_->cfg_->findLoops();

    vectorizeLoops(simdFunction_->cfg_->entry_);

    //simdFunction_->cfg_->placePhiFunctions();
    //simdFunction_->cfg_->renameVars();
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

    if ( simdFunction_->cfg_->loops_.find(bbNode) != simdFunction_->cfg_->loops_.end() )
        return; // this is a loop

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

    // this is the node where the control flow merges after the split at bb
    BBNode* nextNode = ifChild->domFrontier_.first()->value_;
    BasicBlock* next = nextNode->value_;

    swiftAssert( nextNode->pred_.size() == 2, "must exactly have two predecessors" );

    // find last if-branch block and last else-block
    BBNode* lastIfNode   = nextNode->pred_.first()->value_;
    BBNode* lastElseNode = nextNode->pred_.last()->value_;

    if ( simdFunction_->cfg_->dominates(ifChildNode, lastIfNode) )
    {
        swiftAssert( simdFunction_->cfg_->dominates(elseChildNode, lastElseNode),
                "elseChild must dominate lastElseNode")
    }
    else
    {
        std::swap(lastIfNode, lastElseNode);

        swiftAssert( simdFunction_->cfg_->dominates(  ifChildNode, lastIfNode),
                "ifChild must dominate lastIfNode")
        swiftAssert( simdFunction_->cfg_->dominates(elseChildNode, lastElseNode),
                "elseChild must dominate lastElseNode")
    }

    BasicBlock* lastIf   = lastIfNode->value_;
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
    Op* mask = branch->getOp(); // store mask
    delete bb->end_->prev_->value_;
    simdFunction_->instrList_.erase( bb->end_->prev_ );
    bb->fixPointers();

    // remove goto
    if ( typeid(*lastIf->end_->prev_->value_) == typeid(GotoInstr) )
    {
        delete lastIf->end_->prev_->value_;
        simdFunction_->instrList_.erase(lastIf->end_->prev_);
        lastIf->fixPointers();
    }

    // remove goto
    if ( typeid(*lastElse->end_->prev_->value_) == typeid(GotoInstr) )
    {
        delete lastElse->end_->prev_->value_;
        simdFunction_->instrList_.erase(lastElse->end_->prev_);
        lastElse->fixPointers();
    }

    /*
     * add mask stuff
     */

    std::vector<InstrBase*> and_ToBeInserted;
    std::vector<InstrBase*> andnToBeInserted;
    std::vector<InstrBase*>   orToBeInserted;
    std::vector<InstrNode*> toBeErased;

    next->fixPointers();

    // for each phi function
    for (InstrNode* iter = next->firstPhi_; iter != next->firstOrdinary_; iter = iter->next_)
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a PhiInstr here" );
        toBeErased.push_back(iter);
        PhiInstr* phi = (PhiInstr*) iter->value_;

        swiftAssert( phi->arg_.size() == 2, "must exactly have two results" );
        swiftAssert( typeid(*phi->arg_[0].op_) == typeid(Reg), "TODO" );
        swiftAssert( typeid(*phi->arg_[1].op_) == typeid(Reg), "TODO" );
        swiftAssert( typeid(*phi->result())    == typeid(Reg), "TODO" );

        Reg* ifReg; 
        Reg* elseReg; 
        Reg* resReg = (Reg*) phi->result(); 

        if (phi->sourceBBs_[0] == lastIfNode)
        {
            swiftAssert( phi->sourceBBs_[1] == lastElseNode, 
                    "the other one must be the else-BB" );

            ifReg   = (Reg*) phi->arg_[0].op_;
            elseReg = (Reg*) phi->arg_[1].op_;
        }
        else 
        {
            swiftAssert( phi->sourceBBs_[0] == lastElseNode, 
                    "this must be the else-BB" );
            swiftAssert( phi->sourceBBs_[1] == lastIfNode, 
                    "the other one must be the if-BB" );

              ifReg = (Reg*) phi->arg_[1].op_;
            elseReg = (Reg*) phi->arg_[0].op_;
        }

#ifdef SWIFT_DEBUG
        std::string andStr = "and";
        std::string andnStr = "andn";
        Reg* and_Reg = simdFunction_->newSSAReg(resReg->type_, & andStr);
        Reg* andnReg = simdFunction_->newSSAReg(resReg->type_, &andnStr);
#else // SWIFT_DEBUG
        Reg* and_Reg = simdFunction_->newSSAReg(resReg->type_);
        Reg* andnReg = simdFunction_->newSSAReg(resReg->type_);
#endif // SWIFT_DEBUG

        AssignInstr* and_ = new AssignInstr(AssignInstr::AND,  and_Reg, mask, ifReg);
        AssignInstr* andn = new AssignInstr(AssignInstr::ANDN, andnReg, mask, elseReg);
        AssignInstr*  _or = new AssignInstr(AssignInstr::OR, resReg, and_Reg, andnReg);

        and_ToBeInserted.push_back(and_);
        andnToBeInserted.push_back(andn);
          orToBeInserted.push_back(_or);
    }

    // erase
    for (size_t i = 0; i < toBeErased.size(); ++i)
    {
        delete toBeErased[i]->value_;
        simdFunction_->instrList_.erase(toBeErased[i]);
    }

    /*
     * do the actual insert
     */
    for (size_t i = 0; i <   orToBeInserted.size(); ++i)
        simdFunction_->instrList_.insert(next->begin_,   orToBeInserted[i]);
    for (size_t i = 0; i < andnToBeInserted.size(); ++i)
        simdFunction_->instrList_.insert(next->begin_, andnToBeInserted[i]);
    for (size_t i = 0; i < andnToBeInserted.size(); ++i)
        simdFunction_->instrList_.insert(next->begin_, and_ToBeInserted[i]);

    next->fixPointers();
}

void Vectorizer::vectorizeLoops(BBNode* bbNode)
{
    /*
     * do a post-order walk of the dominance tree so we start with the most
     * nested one
     */

    BasicBlock* bb = bbNode->value_;

    BBLIST_EACH(iter, bbNode->value_->domChildren_)
    {
        BBNode* current = iter->value_;
        vectorizeLoops(current);
    }

    // is this a loop?
    Loops::iterator loopIter = simdFunction_->cfg_->loops_.find(bbNode);
    if ( loopIter == simdFunction_->cfg_->loops_.end() )
        return;

    Loop* loop = loopIter->second;
    swiftAssert(bbNode == loop->header_, "must be the header");

    // for each back edge
    for (size_t i = 0; i < loop->backEdges_.size(); ++i)
    {
    }

    // for each exit edge
    for (size_t i = 0; i < loop->exitEdges_.size(); ++i)
    {
    }

    swiftAssert( typeid(*bb->end_->prev_->value_) == typeid(BranchInstr),
            "must be a BranchInstr here" );
    BranchInstr* branch = (BranchInstr*) bb->end_->prev_->value_;
    
    // only exit if everything is zero
    if ( loop->body_.contains(branch->bbTargets_[BranchInstr::FALSE_TARGET]) )
    {
        // -> we must invert the comparision and the branch
        swiftAssert( !loop->body_.contains(branch->bbTargets_[BranchInstr::TRUE_TARGET]), 
                "true-target must not be in the loop" );

        std::cout << "not tested" << std::endl;
        swiftAssert( typeid(*branch->getOp()) == typeid(Reg), "TODO" );
        Reg* reg = (Reg*) branch->getOp();

#ifdef SWIFT_DEBUG
        std::string notStr = "not";
        Reg* notReg = simdFunction_->newSSAReg(reg->type_, &notStr);
#else // SWIFT_DEBUG
        Reg* notReg = simdFunction_->newSSAReg(reg->type_);
#endif // SWIFT_DEBUG

        AssignInstr* _not = new AssignInstr(AssignInstr::NOT, notReg, reg);
        simdFunction_->instrList_.insert(bb->end_->prev_->prev_, _not);
        branch->arg_[0].op_ = notReg;

        std::swap( branch->instrTargets_[0], branch->instrTargets_[1] );
        std::swap( branch->bbTargets_[0], branch->bbTargets_[1] );
    }
    else
        swiftAssert( loop->body_.contains(branch->bbTargets_[BranchInstr::TRUE_TARGET]), 
                "true-target must be in the loop" );

    swiftAssert( loop->backEdges_.size() == 1, "TODO" );
    BBNode* lastNode = loop->backEdges_[0].from_;
    BasicBlock* lastBB = lastNode->value_;

    std::vector<InstrBase*> and_ToBeInserted;
    std::vector<InstrBase*> andnToBeInserted;
    std::vector<InstrBase*>   orToBeInserted;
    std::vector<InstrBase*> copyToBeInserted;

    Op* mask = branch->getOp();

    // for each phi function in the loop header
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next_)
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a PhiInstr here" );
        PhiInstr* phi = (PhiInstr*) iter->value_;

        // get loop argument
        size_t sourceIndex;
        Reg* sourceReg;
        swiftAssert( phi->arg_.size() == 2, "TODO" );
        if ( loop->body_.contains(phi->sourceBBs_[0]) )
        {
            sourceIndex = 0;
            sourceReg = (Reg*) phi->arg_[0].op_;
        }
        else
        {
            swiftAssert( loop->body_.contains(phi->sourceBBs_[1]), 
                    "must be the loop source" );
            sourceIndex = 1;
            sourceReg = (Reg*) phi->arg_[1].op_;
        }

#ifdef SWIFT_DEBUG
        std::string newStr = "new_" + sourceReg->id_;
        Reg* newReg = simdFunction_->newSSAReg(sourceReg->type_, &newStr);
#else // SWIFT_DEBUG
        std::string newStr = "new_" + sourceReg->id_;
        Reg* newReg = simdFunction_->newSSAReg(sourceReg->type_, &newStr);
#endif // SWIFT_DEBUG

        // subtitute phi source arg
        phi->arg_[sourceIndex].op_ = newReg;

        // save old value
        swiftAssert( typeid(*lastBB->end_->prev_->value_) == typeid(GotoInstr),
                "must be a GotoInstr here" );
        InstrNode* insertionPoint = lastBB->end_->prev_->prev_;

#ifdef SWIFT_DEBUG
        std::string andStr = "and";
        std::string andnStr = "andn";
        Reg* and_Reg = simdFunction_->newSSAReg(sourceReg->type_, & andStr);
        Reg* andnReg = simdFunction_->newSSAReg(sourceReg->type_, &andnStr);
#else // SWIFT_DEBUG
        Reg* and_Reg = simdFunction_->newSSAReg(sourceReg->type_);
        Reg* andnReg = simdFunction_->newSSAReg(sourceReg->type_);
#endif // SWIFT_DEBUG

        AssignInstr* and_ = new AssignInstr(AssignInstr::AND,  and_Reg, mask, sourceReg);
        AssignInstr* andn = new AssignInstr(AssignInstr::ANDN, andnReg, mask, (Reg*) phi->result() );
        AssignInstr*  _or = new AssignInstr(AssignInstr::OR, newReg, and_Reg, andnReg);

        and_ToBeInserted.push_back(and_);
        andnToBeInserted.push_back(andn);
          orToBeInserted.push_back(_or);

        simdFunction_->instrList_.insert(insertionPoint, _or);
        simdFunction_->instrList_.insert(insertionPoint, andn);
        simdFunction_->instrList_.insert(insertionPoint, and_);
    }
}

} // namespace me
