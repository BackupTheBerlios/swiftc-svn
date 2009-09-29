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

#include "me/cfg.h"

#include <cstring>

#include <map>
#include <typeinfo>
#include <iostream>

#include "me/functab.h"
#include "me/defusecalc.h"

namespace me {

/*
 * constructor and destructor
 */

CFG::CFG(Function* function)
    : function_(function)
    , instrList_(function->instrList_)
    , entry_(0)
    , exit_(0)
    , idoms_(0)
{}

CFG::~CFG()
{
    delete[] idoms_;
}

/*
 * graph creation
 */

void CFG::calcCFG()
{
    swiftAssert( typeid( *instrList_.first()->value_ ) == typeid(LabelInstr),
        "first instruction of a function must be a LabelInstr");
    swiftAssert( typeid( *instrList_.last ()->value_ ) == typeid(LabelInstr),
        "last instruction of a function must be a LabelInstr");

    InstrNode* end = 0;
    InstrNode* begin = 0;
    BBNode* prevBB = 0;

    // knows the basic blocks in order of the instruction list
    BBList bbList;

    // knows the first ordinary instruction in each basic block
    InstrNode* firstOrdinaryInstr = 0;

    // iterate over the instruction list and find basic blocks
    INSTRLIST_EACH(iter, instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(LabelInstr) )
        {
            begin = end;
            end = iter;

            if (begin) // we have found the next basic block
            {
                BBNode* bb = insert( new BasicBlock(begin, end, firstOrdinaryInstr) );
                bbList.append(bb);

                // keep acount of the label node and the basic block in the map
                labelNode2BBNode_[begin] = bb;

                // is this the first basic block?
                if (!entry_)
                    entry_ = bb; // mark as entry basic block

                prevBB = bb; // now this is the previous basic block
            }

            // reset firstOrdinaryInstr to null
            firstOrdinaryInstr = 0;
        }
        else if (!firstOrdinaryInstr) { 
            // -> we have found the first ordinary instruction of this basic block
            firstOrdinaryInstr = iter; 
        }

#ifdef SWIFT_DEBUG
        // check in the debug version whether a JumpInstr is followed by a LabelInstr
        if ( dynamic_cast<JumpInstr*>(instr) ) 
        {
            swiftAssert( typeid( *iter->next()->value_ ) == typeid(LabelInstr),
                "JumpInstr is not followed by a LabelInstr");
        }
#endif // SWIFT_DEBUG

    } // INSTRLIST_EACH
    // prevBB now holds the last basic block

    /*
     * now build the exit basic block, which consits of the end_ instruction of
     * the last basic block and the sentinel of the instrList_ as end instruction
     * and no ordinary instructions
     */
    exit_ = insert( new BasicBlock(prevBB->value_->end_, instrList_.sentinel(), 0) );
    labelNode2BBNode_[prevBB->value_->end_] = exit_;

    /*
     * now we know of all starting labels and their associated basic block
     * so let us calculate the CFG
     */

    /*
     * iterate once again over the instrList_ in order to connect all basic blocks
     * and find all assignments of a var in a basic block and its first occurance
     */
    BBNode* currentBB = 0;
    bool linkWithPredBB = true;

    INSTRLIST_EACH(iter, instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(LabelInstr) )
        {
            BBNode* next = labelNode2BBNode_[iter];

            /*
             * link if the last instruction was an ordinary assignment
             * and we have a currentBB
             */
            if (linkWithPredBB && currentBB)
                currentBB->link(next);

            currentBB = next;
            linkWithPredBB = true;

            continue;
        }

        if ( dynamic_cast<JumpInstr*>(instr) )
        {
            JumpInstr* ji = (JumpInstr*) instr;

            for (size_t i = 0; i < ji->numTargets_; ++i)
            {
                swiftAssert( labelNode2BBNode_.contains(ji->instrTargets_[i]), 
                            "must be found here" );
                ji->bbTargets_[i] = labelNode2BBNode_[ ji->instrTargets_[i] ];
                currentBB->link( ji->bbTargets_[i] );
            }

            // do not link this one with the predecessor basic block
            linkWithPredBB = false;
        }

        // for each var on the left hand side
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            me::Var* var = instr->res_[i].var_;

            if ( !var->isSSA() )
            {
                swiftAssert(currentBB, "currentBB must have been found here");

                int varNr = var->varNr_;
                currentBB->value_->vars_[varNr] = var;

                if ( !firstOccurance_.contains(varNr) )
                    firstOccurance_[varNr] = currentBB;
            }
        }
    } // INSTRLIST_EACH

    // find all critical edges and insert empty basic blocks in order to eliminate them
    eliminateCriticalEdges();

    calcPostOrder(entry_);
}

void CFG::eliminateCriticalEdges()
{
    // for each CFG node
    Relatives nodes(nodes_);
    RELATIVES_EACH(iter, nodes)
    {
        BBNode* bbNode = iter->value_;
        BasicBlock* bb = bbNode->value_;
        
        if (bbNode->pred_.size() <= 1)
            continue;

        // we have than one pred -> iterate them
        Relatives pred(bbNode->pred_);
        RELATIVES_EACH(predIter, pred)
        {
            BBNode* predNode = predIter->value_;
            BasicBlock* pred = predNode->value_;
            InstrNode* predLastInstr = pred->end_->prev();

            JumpInstr* ji = dynamic_cast<JumpInstr*>(predLastInstr->value_);
            if (ji == 0 || ji->numTargets_ <= 1)
                continue;

            // -> edge between pred and bbNode is critical
            swiftAssert(typeid(*bb->begin_->value_) == typeid(LabelInstr), 
                    "must be a LabelInstr here");

            // is this a self loop?
            bool selfLoop = false;
            if (pred == bb)
                selfLoop = true;
            /*
             * Insert new basic block like this:
             *
             * +-------------------+
             * | predNode/pred     |
             * |      JumpInstr    | <--- JumpInstr must eventually be fixed
             * +----+---------+----+
             *      |         |
             *      |         |
             *                v
             *           +--------------------+
             *           | newBB              | <--- critical edge eliminated
             *           |     labelNode      | <--- use bb's former leading label
             *           +-------------+------+
             *                        |
             *                        |          |
             *                        v          v
             *                   +--------------------+
             *                   | bbNode/bb          |
             *                   |     newLabelNode   | <--- create new label here
             *                   ---------------------+
             */                          

            InstrNode* labelNode = bb->begin_;

            // create new beginning Label and insert it in the instruction list
            InstrNode* newLabelNode = instrList_.insert( labelNode, new LabelInstr() );

            // create newBBNode
            BBNode* newBBNode = insert( new BasicBlock(labelNode, newLabelNode) );

            // change current bb's leading LabelInstr
            bb->begin_ = newLabelNode;

            // rewire basic blocks
            Relative* it = predNode->succ_.find(bbNode);
            swiftAssert( it != predNode->succ_.sentinel(), "node must be found here" );
            predNode->succ_.erase(it);

            it = bbNode->pred_.find(predNode);
            swiftAssert( it != bbNode->pred_.sentinel(), "node must be found here" );
            bbNode->pred_.erase(it);
            
            predNode->link(newBBNode);
            newBBNode->link(bbNode);

            // fix JumpInstr
            size_t jumpIndex = 0;
            RELATIVES_EACH(succIter, predNode->succ_)
            {
                ji->bbTargets_[jumpIndex] = succIter->value_;
                ji->instrTargets_[jumpIndex] = succIter->value_->value_->begin_;
                ++jumpIndex;
            }

            // fix labelNode2BBNode_
            labelNode2BBNode_[labelNode] = newBBNode;
            labelNode2BBNode_[newLabelNode] = bbNode;

            /*
             * add or fix jump instructions of all predecessors of bbNode which
             * are not newBBNode
             */

            RELATIVES_EACH(bbPredIter, bbNode->pred_)
            {
                BBNode* bbPred = bbPredIter->value_;
                if (bbPred == newBBNode)
                    continue;

                JumpInstr* prevJump = dynamic_cast<JumpInstr*>( bbPred->value_->end_->prev_->value_ );
                if ( !prevJump)
                {
                    GotoInstr* gi = new GotoInstr(newLabelNode);
                    gi->bbTargets_[0] = bbNode;
                    instrList_.insert(bbPred->value_->end_->prev_, new InstrNode(gi));
                }
            }

            /*
             * handle self loops
             */

            if (selfLoop)
            {
                swiftAssert(false, "TODO");
                // TODO perhaps this can be done smarter
                BasicBlock* newBB = newBBNode->value_;

                // find newBB's predecessor
                InstrNode* backIter = newBB->begin_->prev();
                while (true)
                {
                    InstrBase* instr = backIter->value_;
                    swiftAssert( !dynamic_cast<JumpInstr*>(instr),
                            "there may not be a jump" );

                    if ( typeid(*instr) == typeid(LabelInstr) )
                    {
                        /*
                         * this is the preceding basic block 
                         * without a jump at the end
                         */

                        BBNode* toBeFixedNode = labelNode2BBNode_[backIter];

                        GotoInstr* gi = new GotoInstr(toBeFixedNode->succ_.first()->value_->value_->begin_);
                        gi->bbTargets_[0] = toBeFixedNode->succ_.first()->value_;
                        instrList_.insert( toBeFixedNode->value_->end_->prev(), gi );

                        break;
                    }
                    // iterate backwards
                    backIter = backIter->prev();
                }
            }
        } // for each predecessor
    } // for each CFG node
}

void CFG::calcDomTree()
{
    // init dom array
    if (idoms_)
        delete[] idoms_;

    // clear domChildren_
    RELATIVES_EACH(iter, nodes_)
        iter->value_->value_->domChildren_.clear();

    idoms_ = new BBNode*[size()];
    memset(idoms_, 0, sizeof(BBNode*) * size());

    idoms_[entry_->postOrderIndex_] = entry_;

    bool changed = true;

    while (changed)
    {
        changed = false;

        // iterate over the CFG in reverse post-order
        for (size_t i = size() - 2; long(i) >= 0; --i) // void entry node
        {
            // current node
            BBNode* bb = postOrder_[i];
            swiftAssert(bb != entry_, "do not process the entry node");

            // pick one which has been processed
            BBNode* newIdom = 0;
            for (Relative* iter = bb->pred_.first(); iter != bb->pred_.sentinel(); iter = iter->next())
            {
                BBNode* processedBB = iter->value_;

                if ( processedBB->postOrderIndex_ > i)
                {
                    newIdom = processedBB;
                    // found a processed node
                    break;
                }
            }

            swiftAssert(newIdom, "no processed predecessor found");

            // for all other predecessors
            RELATIVES_EACH(iter, bb->pred_)
            {
                BBNode* predBB = iter->value_;

                if (bb == newIdom)
                    continue;

                if ( idoms_[predBB->postOrderIndex_] != 0 )
                    newIdom = intersect(predBB, newIdom);
            }

            if (idoms_[bb->postOrderIndex_] != newIdom )
            {
                idoms_[bb->postOrderIndex_] = newIdom;
                changed = true;
            }
        } // for
    } // while

    /*
     * Now we can walk the idom array to get each dominator.
     * Let's compute the reserve set, too, so we can easily access
     * children of basic blocks and not only their parents.
     */

    for (size_t i = 0; i < size(); ++i)
    {
        BBNode* bb = postOrder_[i];
        swiftAssert(i == bb->postOrderIndex_, "i and postOrderIndex_ bust be consistent");
        idoms_[i]->value_->domChildren_.append(bb); // append child
    }

    // remove the entry -> entry cycle from the domChildren_
    entry_->value_->domChildren_.erase( entry_->value_->domChildren_.find(entry_) );
}

BBNode* CFG::intersect(BBNode* b1, BBNode* b2)
{
    BBNode* finger1 = b1;
    BBNode* finger2 = b2;

    while (finger1->postOrderIndex_ != finger2->postOrderIndex_)
    {
        while (finger1->postOrderIndex_ < finger2->postOrderIndex_)
            finger1 = idoms_[finger1->postOrderIndex_];
        while (finger2->postOrderIndex_ < finger1->postOrderIndex_)
            finger2 = idoms_[finger2->postOrderIndex_];
    }

    return finger1;
}

void CFG::calcDomFrontier()
{
    // clear domFrontier_
    RELATIVES_EACH(iter, nodes_)
        iter->value_->value_->domFrontier_.clear();

    for (size_t i = 0; i < size(); ++i)
    {
        BBNode* bb = postOrder_[i];

        // if the number of predecessors >= 2
        if (bb->pred_.size() >= 2)
        {
            // for all predecessors of bb
            RELATIVES_EACH(iter, bb->pred_)
            {
                BBNode* runner = iter->value_;

                while ( runner != idoms_[bb->postOrderIndex_] )
                {
                    // add bb to the runner's dominance frontier
                    runner->value_->domFrontier_.append(bb);
                    runner = idoms_[runner->postOrderIndex_];
                }
            }
        }
    }
}

/*
 * phi functions
 */

void CFG::placePhiFunctions()
{
    /*
     * hasAlready[bb->postOrderIndex_] knows
     * whether a phi-function in bb has already been put
     */
    int hasAlready[size()];

    /*
     * hasBeenAdded[bb->postOrderIndex_] knows
     * whether bb has already been added to the work list
     */
    int hasBeenAdded[size()];

    // set all blocks to "not added" and "not already"
    memset(hasAlready, 0, sizeof(int) * size()); // init to 0
    memset(hasBeenAdded, 0, sizeof(int) * size()); // init to 0

    int iterCount = 0;

    // for each var not in SSA form, i.e. varNr_ > 0
    VarMap::iterator bound = function_->vars_.lower_bound(0); // 0 itself is not in the map
    for (VarMap::iterator iter = function_->vars_.begin(); iter != bound; ++iter)
    {
        ++iterCount;

        Var* var = iter->second;
        swiftAssert( !var->isSSA(), "var is already in SSA form" );

        BBList work;

        // init work list with all basic blocks which assign to var
        for (size_t i = 0; i < size(); ++i)
        {
            BBNode* bb = postOrder_[i];

            if ( bb->value_->vars_.find(var->varNr_) != bb->value_->vars_.end() )
            {
                hasBeenAdded[bb->postOrderIndex_] = iterCount;
                work.append(bb);
            }

        }

        /*
         * mark dominance frontier of the basic block with the first occurance
         * of var as hasAlready
         */
        FirstOccurance::iterator iter = firstOccurance_.find(var->varNr_);

        if ( iter != firstOccurance_.end() )
        {
            BBNode* firstBB = iter->second;

            // for each basic block from DF(fristBB)
            BBLIST_EACH(iter, firstBB->value_->domFrontier_)
                hasAlready[iter->value_->postOrderIndex_] = iterCount;
        }

        // for each basic block in the work list
        while ( !work.empty() )
        {
            // take a basic block from the list
            BBNode* bb = work.first()->value_;
            work.removeFirst();

            // for each basic block from DF(bb)
            BBLIST_EACH(iter, bb->value_->domFrontier_)
            {
                BBNode* df = iter->value_;

                // do we already have a phi function for this node and this var?
                if ( hasAlready[df->postOrderIndex_] >= iterCount )
                    continue; // yes -> so process next one
                // else

                // place phi function
                PhiInstr* phiInstr = new PhiInstr( var, df->pred_.size() );
                df->value_->firstPhi_ = instrList_.insert(df->value_->begin_, phiInstr);

                // update data structures
                hasAlready[df->postOrderIndex_] = iterCount;
                if (hasBeenAdded[df->postOrderIndex_] == false)
                {
                    hasBeenAdded[df->postOrderIndex_] = iterCount;
                    work.append(df);
                }
            }
        } // while
    } // for each var
}

void CFG::renameVars()
{
    VarMap& vars = function_->vars_;

    std::vector< std::stack<Var*> > names;
    // since index 0 is reserved for constants the stack-array's size must be increased by one
    names.resize( -vars.begin()->first + 1 );

    // start with the first basic block
    rename(entry_, names);

    /*
     * all vars i.e. varNr < 0 are not needed anymore from this point
     */

    // for each basic block
    for (size_t i = 0; i < size(); ++i)
        postOrder_[i]->value_->vars_.clear();

    // for each vars, no temps
    for (VarMap::iterator iter = vars.begin(); iter->first < 0 && iter != vars.end(); ++iter)
    {
        Var* var =  iter->second;
        vars.erase(iter);
        delete var;
    }
}

void CFG::rename(BBNode* bb, std::vector< std::stack<Var*> >& names)
{
    // for each instruction in bb except the leading LabelInstr
    for (InstrNode* iter = bb->value_->firstPhi_; iter != bb->value_->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        // if instr is an ordinary assignment
        if ( typeid(*instr) != typeid(PhiInstr) )
        {
            // replace vars on the right hand side
            for (size_t i = 0; i < instr->arg_.size(); ++i)
            {
                me::Var* var = dynamic_cast<me::Var*>( instr->arg_[i].op_ );

                if ( var && !var->isSSA() )
                {
                    swiftAssert( !names[ var->var2Index() ].empty(), 
                            "stack is empty here, this normally means "
                            "that the programm is not in SSA form");
                    instr->arg_[i].op_ = names[ var->var2Index() ].top();
                }
            }
        }

        // replace var on the left hand side in all cases
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            if ( !instr->res_[i].var_->isSSA() )
            {
                Var* var = function_->cloneNewSSA(instr->res_[i].var_);

                swiftAssert(size_t(-instr->res_[i].oldVarNr_) < names.size(), "index out of bounds");
                names[ -instr->res_[i].oldVarNr_ ].push(var);
                instr->res_[i].var_ = var;
            }
        }
    } // for each instruction

    // for each successor of bb
    RELATIVES_EACH(iter, bb->succ_)
    {
        BBNode* succ = iter->value_;
        size_t j = succ->whichPred(bb);

        // for each phi function in succ 
        for (InstrNode* iter = succ->value_->firstPhi_; iter != succ->value_->firstOrdinary_; iter = iter->next())
        {
            // we must always find a PhiInstr here
            swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a PhiInstr here" );

            PhiInstr* phi = static_cast<PhiInstr*>(iter->value_);

            if ( names[ -phi->res_[0].oldVarNr_ ].empty() )
                continue; // var not found

            phi->arg_[j].op_ = names[ -phi->oldResultNr() ].top();
            phi->sourceBBs_[j] = bb;
        }
    }

    // for each child of bb in the dominator tree
    BBLIST_EACH(iter, bb->value_->domChildren_)
    {
        BBNode* domChild = iter->value_;
        rename(domChild, names);
    }

    // for each AssignmentBase in bb
    for (InstrNode* iter = bb->value_->firstPhi_; iter != bb->value_->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        // for each var on the left hand side
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            if (instr->res_[i].oldVarNr_ < 0) // if this is a var
            {
                swiftAssert( names[- instr->res_[i].oldVarNr_].size() > 0, "cannot pop here");
                names[ - instr->res_[i].oldVarNr_ ].pop();
            }
        } // for ech var on the left hand side
    } // for each AssignmentBase
}

/*
 * SSA form construction
 */

void CFG::constructSSAForm() 
{
    calcCFG();
    calcDomTree();
    calcDomFrontier();
    placePhiFunctions();
    renameVars();
}

BBSet CFG::calcIteratedDomFrontier(BBSet bbs)
{
    BBList work;
    BBSet result;

    BBSET_EACH(iter, bbs)
        work.append(*iter);

    // for each basic block in the work list
    while ( !work.empty() )
    {
        // take a basic block from the list
        BBNode* bb = work.first()->value_;
        work.removeFirst();

        // for each basic block from DF(bb)
        BBLIST_EACH(iter, bb->value_->domFrontier_)
        {
            BBNode* df = iter->value_;

            if ( !result.contains(df) )
            {
                // df -> not in result so add to result and to work list
                swiftAssert(df->pred_.size() > 1,
                        "current basic block must have more than 1 predecessor");

                result.insert(df);
                work.append(df);
            }
        }
    }

    return result;
}

/*
 *  +-------------------+     +--------------------+
 *  | predNode/pred     |     | predNode/pred      |
 *  |     JumpInstr     |     |     JumpInstr      | <--- JumpInstr must eventually be fixed
 *  +--------------+----+     +----+---------------+
 *                 |               |
 *                 |               |
 *                 v               v
 *              +---------------------+
 *              | newNode/newBB       | <--- If this is the first basic block update entry_
 *              |     labelNode       | <--- bbNode's former leading label
 *              |     phiNodes        | <--- phi instructions are fine: they have sourceBBs_
 *              +---------+-----------+
 *                        |
 *                        |
 *                        v
 *              +---------------------+
 *              | bbNode/bb           |
 *              |     newLabelNode    | <--- newly created label
 *              |     instrNode       | <--- the split instruction
 *              |     ...             | <--- nothing to do from here on
 *              +---------------------+
 *                 |               |
 *                 |               |
 *                 v               v
 *  +-------------------+     +--------------------+
 *  | ...               |     | ...                |
 *  +-------------------+     +--------------------+
 */
void CFG::splitBB(me::InstrNode* instrNode, me::BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;
    InstrNode* labelNode = bb->begin_;

    // create new beginning Label and insert it in the instruction list
    InstrNode* newLabelNode = instrList_.insert( instrNode->prev(), new LabelInstr() );

    // create new basic block
    BBNode* newNode = insert( new BasicBlock(labelNode, newLabelNode) );
    BasicBlock* newBB = newNode->value_;

    // update entry_ if necessary
    if (entry_ == bbNode)
        entry_ = newNode;

    // fix bottom basic block
    bb->begin_ = newLabelNode;

    /*
     * rewire new basic block
     */

    // for each former predecessor of bottomNode
    RELATIVES_EACH(iter, bbNode->pred_)
    {
        BBNode* predNode = iter->value_;
        BasicBlock* pred = predNode->value_;

        // fix succ of pred
        Relative* it = predNode->succ_.find(bbNode);
        swiftAssert( it != predNode->succ_.sentinel(), "node must be found here" );
        predNode->succ_.erase(it);
        predNode->link(newNode);

        /*
         * fix JumpInstrs if applicable
         */
        JumpInstr* ji = dynamic_cast<JumpInstr*>(pred->end_->prev()->value_);
        if (ji)
        {
            // find index of the jump target in question
            for (size_t jumpIndex = 0; jumpIndex < ji->numTargets_; ++jumpIndex)
            {
                if (ji->bbTargets_[jumpIndex] == bbNode)
                {
                    // and fix it
                    ji->bbTargets_[jumpIndex] = newNode;
                    ji->instrTargets_[jumpIndex] = newNode->value_->begin_;
                    break;
                }
            }
        } // if JumpInstr
    } // for each pred

    // clear bbNode's preds in all cases 
    bbNode->pred_.clear();

    // and link top with bottom in all cases
    newNode->link(bbNode);

    newBB->fixPointers();
    bb->fixPointers();
 
    // fix labelNode2BBNode_
    labelNode2BBNode_[labelNode] = newNode;
    labelNode2BBNode_[newLabelNode] = bbNode;
}


void CFG::mergeBB(BBNode* topNode, BBNode* bottomNode)
{
    swiftAssert(    topNode->succ_.size() == 1, "must exactly have one successor" );
    swiftAssert( bottomNode->pred_.size() == 1, "must exactly have one predecessor" );

    BasicBlock* top    =    topNode->value_;
    BasicBlock* bottom = bottomNode->value_;

    GotoInstr* gi = dynamic_cast<GotoInstr*>(top->end_->prev_->value_);
    if (gi)
    {
        swiftAssert( gi->instrTargets_[0] == bottom->begin_, "wrong target" );
        delete top->end_->prev_->value_;
        instrList_.erase(top->end_->prev_);
    }

    topNode->succ_.clear();
    RELATIVES_EACH(iter, bottomNode->succ_)
    {
        BBNode* succNode = iter->value_;

        topNode->succ_.append(succNode);
        Relative* relative = succNode->pred_.find(bottomNode);
        relative->value_ = topNode;
        //succNode->pred_.erase( succNode->pred_.find(bottomNode) );
        //succNode->pred_.append(topNode);
    }

    /*
     * check whether bottom's dominance frontier hast phi functions
     * and substitute the sourceBBs accordingly
     */

    //BBLIST_EACH(iter, bottom->domFrontier_)
    //{
        //BBNode* bbNode = iter->value_;
        //BasicBlock* bb = bbNode->value_;

        ////bb->fixPointers();

        //// for each phi function in bb
        //for (InstrNode* instrNode = bb->firstPhi_; instrNode != bb->firstOrdinary_; instrNode = instrNode->next())
        //{
            //swiftAssert( typeid(*instrNode) == typeid(PhiInstr),
                    //"must be a PhiInstr here" );

            //PhiInstr* phi = (PhiInstr*) instrNode->value_;

            //// for each source basic block
            //for (size_t i = 0; i < phi->arg_.size(); ++i)
            //{
                //BBNode* sourceNode = phi->sourceBBs_[i];

                //if (sourceNode == bbNode)
                    //phi->sourceBBs_[i] = topNode;
            //}
        //}
    //}

    labelNode2BBNode_.erase(bottom->begin_);
    delete bottom->begin_->value_;
    instrList_.erase(bottom->begin_);
    top->end_ = bottom->end_;
    top->fixPointers();

    nodes_.erase( nodes_.find(bottomNode) );
    delete bottomNode;
}

/*
 * SSA reconstruction and rewiring
 */

void CFG::reconstructSSAForm(VarDefUse* vdu)
{
    BBSet defBBs;

    /*
     * calculate iterated dominance frontier for all defining basic blocks
     */
    DEFUSELIST_EACH(iter, vdu->defs_)
        defBBs.insert(iter->value_.bbNode_);

    BBSet iDF = calcIteratedDomFrontier(defBBs);

    // for each use
    DEFUSELIST_EACH(iter, vdu->uses_)
    {
        DefUse& du = iter->value_;
        BBNode* bbNode = du.bbNode_;
        InstrNode* instrNode = du.instrNode_;
        InstrBase* instr = instrNode->value_;

#ifdef SWIFT_DEBUG
        bool found = false;
#endif // SWIFT_DEBUG

        // for each arg for which instr->arg_[i] in defs
        for (size_t i = 0; i < instr->arg_.size(); ++i)
        {
            Var* useVar = (Var*) dynamic_cast<Var*>(instr->arg_[i].op_);
            if (!useVar) 
                continue;

            DEFUSELIST_EACH(iter, vdu->defs_)
            {
                Var* defVar = iter->value_.var_;

                // substitute arg with proper definition
                if (useVar == defVar)
                {
#ifdef SWIFT_DEBUG
                    found = true;
#endif // SWIFT_DEBUG
                    instr->arg_[i].op_ = findDef(i, instrNode, bbNode, vdu, iDF);
                }
            } // for each def
        } // for each arg of use
        swiftAssert(found, "no arg found in defs");
    } // for each use
}

Var* CFG::findDef(size_t p, InstrNode* instrNode, BBNode* bbNode, VarDefUse* vdu, BBSet& iDF)
{
    if ( typeid(*instrNode->value_) == typeid(PhiInstr) )
    {
        PhiInstr* phi = (PhiInstr*) instrNode->value_;
        bbNode = phi->sourceBBs_[p];
        instrNode = bbNode->value_->end_->prev();
    }

    BasicBlock* bb = bbNode->value_;

    while (true)
    {
        // iterate backwards over all instructions in this basic block
        while (instrNode != bb->begin_)
        {
            InstrBase* instr = instrNode->value_; 

            // defines instr one of vdu?
            for (size_t i = 0; i < instr->res_.size(); ++i)
            {
                Var* instrVar = dynamic_cast<Var*>(instr->res_[i].var_);

                if (!instrVar)
                    continue;

                DEFUSELIST_EACH(iter, vdu->defs_)
                {
                    Var* defVar = iter->value_.var_;

                    if (instrVar == defVar)
                        return instrVar; // yes -> return the defined var
                }
            }

            // move backwards
            instrNode = instrNode->prev();
        }

        // is this basic block in the iterated dominance frontier?
        BBSet::iterator bbIter = iDF.find(bbNode);
        if ( bbIter != iDF.end() )
        {
            // -> place phi function
            swiftAssert(bbNode->pred_.size() > 1, 
                    "current basic block must have more than 1 predecessors");
            Var* var = vdu->defs_.first()->value_.var_;

            // create new result
            Var* newVar = function_->cloneNewSSA(var);

            // create phi instruction
            PhiInstr* phi = new PhiInstr( newVar, bbNode->pred_.size() );

            // init sourceBBs
            size_t counter = 0;
            RELATIVES_EACH(predIter, bbNode->pred_)
            {
                phi->sourceBBs_[counter] = predIter->value_;
                ++counter;
            }

            instrNode = instrList_.insert(bb->begin_, phi); 
            bb->firstPhi_ = instrNode;

            // register new definition
            vdu->defs_.append( DefUse(newVar, instrNode, bbNode) );

            for (size_t i = 0; i < phi->arg_.size(); ++i)
                phi->arg_[i].op_ = findDef(i, instrNode, bbNode, vdu, iDF);

            return phi->result();
        }

        swiftAssert( entry_ != bbNode, "unreachable code");

        // go up dominance tree -> update bbNode, bb and instrNode
        bbNode = idoms_[bbNode->postOrderIndex_];
        bb = bbNode->value_;
        instrNode = bb->end_->prev();
    }

    // unreachable
    return 0;
}

/*
 * domination stuff
 */

bool CFG::dominates(Var* x, Var* y) const
{
    swiftAssert(x != y, "vars must be distinct");

    const DefUse& dx = x->def_;
    const DefUse& dy = y->def_;

    return dominates(dx.instrNode_, dx.bbNode_, dy.instrNode_, dy.bbNode_);
}

bool CFG::dominates(InstrNode* i1, BBNode* b1, InstrNode* i2, BBNode* b2) const
{
    if (i1 == i2)
        return true; // a label always dominates itself

    if (b1 != b2) 
    {
        /*
         * -> the definitions are in different basic blocks
         * does b1 dominates b2?
         */
        return b1->value_->hasDomChild(b2);
    }

    /*
     * -> the definitions are in same basic block
     * check whether i1 precedes i2
     */

    const BasicBlock* bb = b1->value_;

    for (const InstrNode* iter = i2->prev(); iter != bb->begin_; iter = iter->prev())
    {
        if (iter == i1)
            return true;
    }

#ifdef SWIFT_DEBUG
    // check in the debug version whether i2 precedes i1

    for (const InstrNode* iter = i1->prev(); iter != bb->begin_; iter = iter->prev())
    {
        if (iter == i2)
            return false;
    }

    swiftAssert(false, "i1 and i2 seem to be in different basic blocks");
#endif // SWIFT_DEBUG

    return false;
}

/*
 * loop finding
 */

void CFG::findLoops()
{
    //std::cout << "-- " << *function_->id_ << " --" << std::endl;
    // for each node
    RELATIVES_EACH(fromIter, nodes_)
    {
        BBNode* fromNode = fromIter->value_;
        BasicBlock* from = fromNode->value_;

        // for each successor
        RELATIVES_EACH(toIter, fromNode->succ_)
        {
            BBNode* toNode = toIter->value_;
            BasicBlock* to = toNode->value_;

            /*
             * examine edge 'from' -> 'to':
             * does 'to' dominate 'from'?
             */

            if ( to->hasDomChild(fromNode) )
            {
                // yes, so 'from' -> 'to' is a back edge

                // find loop body
                std::cout << from->name() << " -> " << to->name() << std::endl;
            }
        }
    }
}

/*
 * ommiting the interference graph
 */

bool CFG::interferenceCheck(Var* x, Var* y) const
{
    swiftAssert(x != y, "vars must be distinct");

    Var* t;
    Var* b;

    if ( dominates(x, y) )
    {
        t = x;
        b = y;
    }
    else if ( dominates(y, x) )
    {
        t = y;
        b = x;
    }
    else
        return false;

    // -> now t dominates b

    if ( b->def_.bbNode_->value_->liveOut_.contains(t) ) 
        return true;

    DefUse& db = b->def_;

    // for all uses iter of t
    DEFUSELIST_EACH(iter, t->uses_)
    {
        DefUse& ut = iter->value_;

        if ( dominates(db.instrNode_, db.bbNode_, ut.instrNode_, ut.bbNode_) )
            return true;
    }

    return false;
}

RegSet CFG::intNeighbors(Reg* reg, int typeMask, bool spilled)
{
    RegSet result;
    BBSet walked;

    // for all uses iter of reg
    DEFUSELIST_EACH(iter, reg->uses_)
        findInt(reg, iter->value_.bbNode_, result, walked, typeMask, spilled);

    return result;
}

void CFG::findInt(Reg* reg, BBNode* bbNode, RegSet& result, BBSet& walked, int typeMask, bool spilled)
{
    BasicBlock* bb = bbNode->value_;

    // remember as visited
    walked.insert(bbNode);

    InstrNode* last = bb->end_->prev();

    RegSet l;
    VARSET_EACH(iter, last->value_->liveOut_)
    {
        Reg* liveOut = (*iter)->isReg(typeMask, spilled);
        if (liveOut)
            l.insert(liveOut);
    }

    // found a new result?
    if ( l.contains(reg) )
    {
        REGSET_EACH(lIter, l)
        {
            Reg* lReg = *lIter;
            if (lReg != reg)
                result.insert(lReg);
        }
    }

    for (InstrNode* iter = last; iter != bb->begin_ && iter != reg->def_.instrNode_ && iter != bb->firstOrdinary_->prev_; iter = iter->prev())
    {
        InstrBase* instr = iter->value_;

        // for each result
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            Reg* res = instr->res_[i].var_->isReg(typeMask, spilled);
            if (res)
                l.erase(res);
        }
        // -> l = l \ instr->res

        // for each arg
        for (size_t i = 0; i < instr->arg_.size(); ++i)
        {
            Reg* arg = instr->arg_[i].op_->isReg(typeMask, spilled);
            if (arg)
                l.insert(arg);
        }
        // -> l = l U instr->arg

        // found a new result?
        if ( l.contains(reg) )
        {
            REGSET_EACH(lIter, l)
            {
                Reg* lReg = *lIter;
                if (lReg != reg)
                    result.insert(lReg);

                if ( instr->isConstrained() )
                {
                    // for each result
                    for (size_t i = 0; i < instr->res_.size(); ++i)
                    {
                        Reg* res = instr->res_[i].var_->isReg(typeMask, spilled);
                        int constraint = instr->res_[i].constraint_;
                        if (res && constraint != NO_CONSTRAINT)
                            result.insert(res);
                    }

                    // for each arg
                    for (size_t i = 0; i < instr->arg_.size(); ++i)
                    {
                        Reg* arg = instr->arg_[i].op_->isReg(typeMask, spilled);
                        int constraint = instr->arg_[i].constraint_;
                        if (arg && constraint != NO_CONSTRAINT && arg != reg)
                            result.insert(arg);
                    }
                }
            }
        }
    }

    // for each unvisted predecessor of bbNode
    RELATIVES_EACH(iter, bbNode->pred_)
    {
        BBNode* predNode = iter->value_;

        if ( walked.contains(predNode) )
            continue;

        InstrNode* predLast = predNode->value_->end_->prev();
        DefUse& dv = reg->def_;
        if ( dominates(dv.instrNode_, dv.bbNode_, predLast, predNode) )
            findInt(reg, predNode, result, walked, typeMask, spilled);
    }
}

/*
 * dump methods
 */

std::string CFG::name() const
{
    // make the id readable for dot
    std::string result = *function_->id_;

    for (size_t i = 0; i < result.size(); ++i)
    {
        if (result[i] == '$')
            result[i] = '_';
    }

    return result;
}

std::string CFG::dumpIdoms() const
{
    std::ostringstream oss;

    for (size_t i = 0; i < size(); ++i)
    {
        oss << '\t' 
            << postOrder_[i]->value_->name() 
            << " -> " 
            << idoms_[i]->value_->name() 
            << std::endl;
    }

    return oss.str();
}

std::string CFG::dumpDomChildren() const
{
    std::ostringstream oss;

    for (size_t i = 0; i < size(); ++i)
    {
        BBNode* bb = postOrder_[i];

        oss << '\t' << bb->value_->name() << ":\t";

        BBLIST_EACH(iter, bb->value_->domChildren_)
             oss << iter->value_->value_->name() << ' ';

        oss << std::endl;
    }

    return oss.str();
}

std::string CFG::dumpDomFrontier() const
{
    std::ostringstream oss;

    for (size_t i = 0; i < size(); ++i)
    {
        BBNode* bb = postOrder_[i];

        oss << '\t' << bb->value_->name() << ":\t";

        BBLIST_EACH(iter, bb->value_->domFrontier_)
            oss << iter->value_->value_->name() << " ";

        oss << std::endl;
    }

    return oss.str();
}

} // namespace me
