#include "me/cfg.h"

#include <cstring>

#include <map>
#include <typeinfo>

#include "me/functab.h"

namespace me {

/*
 * constructor and destructor
 */

CFG::CFG(Function* function)
    : function_(function)
    , instrList_(function->instrList_)
    , entry_(0)
    , exit_(0)
    , cfErrorHandler_(0)
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
                ji->bbTargets_[i] = labelNode2BBNode_[ ji->instrTargets_[i] ];
                currentBB->link( ji->bbTargets_[i] );
            }

            // do not link this one with the predecessor basic block
            linkWithPredBB = false;
        }

        if ( dynamic_cast<AssignmentBase*>(instr) )
        {
            AssignmentBase* ab = (AssignmentBase*) instr;
            // if we have an assignment to a var update this in the map

            // for each var on the left hand side
            for (size_t i = 0; i < ab->numLhs_; ++i)
            {
                me::Reg* reg = ab->lhs_[i];

                if ( !reg->isSSA() )
                {
                    swiftAssert(currentBB, "currentBB must have been found here");

                    int varNr = reg->varNr_;
                    currentBB->value_->vars_[varNr] = reg;

                    if ( firstOccurance_.find(varNr) == firstOccurance_.end() )
                        firstOccurance_[varNr] = currentBB;
                }
            }
        }
    } // INSTRLIST_EACH

    // find all critical edges and insert empty basic blocks in order to eliminate them
    eliminateCriticalEdges();

    calcPostOrder(entry_);
}

void CFG::eliminateCriticalEdges()
{
    Relatives nodes(nodes_);
    
    // for each CFG node
    RELATIVES_EACH(iter, nodes)
    {
        BBNode* bbNode = iter->value_;
        BasicBlock* bb = bbNode->value_;
        
        if (bbNode->pred_.size() <= 1)
            continue;

        Relatives pred(bbNode->pred_);
        RELATIVES_EACH(predIter, pred)
        {
            BBNode* predNode = predIter->value_;
            BasicBlock* pred = predNode->value_;
            InstrNode* predLastInstr = pred->end_->prev();

            JumpInstr* ji = dynamic_cast<JumpInstr*>(predLastInstr->value_);
            if (ji == 0 || ji->numTargets_ <= 1)
                continue;

            size_t jumpIndex = 0;
            // find index of the jump target in question
            for (size_t jumpIndex = 0; jumpIndex < ji->numTargets_; ++jumpIndex)
            {
                if (ji->bbTargets_[jumpIndex]->value_ == bb)
                    break; // found
            }
            swiftAssert(jumpIndex < ji->numTargets_, "jump target not found");

            // -> edge between pred and iter is critical
            InstrNode* bbFirstInstr = bb->begin_;
            swiftAssert(typeid(*bb->begin_->value_) == typeid(LabelInstr), 
                    "must be a LabelInstr here");
            InstrNode* labelNode = bb->begin_;

            /*
             * insert new BasicBlock
             */

            InstrNode* newLabelNode = instrList_.insert( labelNode, new LabelInstr() );

            /* 
             * create new beginning Label and insert it in the instruction list
             * -> append to current bb's leading LabelInstr
             */
            BBNode* newBB = insert( new BasicBlock(labelNode, newLabelNode) );
            bb->begin_ = newLabelNode;// this is current bb's new leading label

            // rewire basic blocks
            Relative* it = predNode->succ_.find(bbNode);
            swiftAssert( it != predNode->succ_.sentinel(), "node must be found here" );
            predNode->succ_.erase(it);

            it = bbNode->pred_.find(predNode);
            swiftAssert( it != bbNode->pred_.sentinel(), "node must be found here" );
            bbNode->pred_.erase(it);
            
            predIter->value_->link(newBB);
            newBB->link(bbNode);

            // fix JumpInstr
            ji->bbTargets_[jumpIndex] = newBB;

            // fix labelNode2BBNode_
            labelNode2BBNode_[labelNode] = newBB;
            labelNode2BBNode_[newLabelNode] = bbNode;
        } // for each predecessor
    } // for each CFG node
}

void CFG::calcDomTree()
{
    // init dom array
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
 * error handling related to control flow
 */

void CFG::installCFErrorHandler(CFErrorHandler* cfErrorHandler)
{
    cfErrorHandler_ = cfErrorHandler;
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
    RegMap::iterator bound = function_->vars_.lower_bound(0); // 0 itself is not in the map
    for (RegMap::iterator iter = function_->vars_.begin(); iter != bound; ++iter)
    {
        ++iterCount;

        Reg* var = iter->second;
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
        BBNode* firstBB = firstOccurance_.find(var->varNr_)->second;

        // for each basic block from DF(fristBB)
        BBLIST_EACH(iter, firstBB->value_->domFrontier_)
            hasAlready[iter->value_->postOrderIndex_] = iterCount;

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
    RegMap& vars = function_->vars_;

    std::vector< std::stack<Reg*> > names;
    // since index 0 is reserved for literals the stack-array's size must be increased by one
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
    for (RegMap::iterator iter = vars.begin(); iter->first < 0 && iter != vars.end(); ++iter)
    {
        delete iter->second;
        vars.erase(iter);
    }
}

void CFG::rename(BBNode* bb, std::vector< std::stack<Reg*> >& names)
{
    // for each instruction in bb except the leading LabelInstr
    for (InstrNode* iter = bb->value_->firstPhi_; iter != bb->value_->end_; iter = iter->next())
    {
        AssignmentBase* ab = dynamic_cast<AssignmentBase*>(iter->value_);

        if (ab)
        {
            // if ab is an ordinary assignment
            if ( typeid(*ab) != typeid(PhiInstr) )
            {
                // replace vars on the right hand side
                for (size_t i = 0; i < ab->numRhs_; ++i)
                {
                    me::Reg* reg = dynamic_cast<me::Reg*>(ab->rhs_[i]);

                    if ( reg && !reg->isSSA() )
                    {
                        swiftAssert( !names[ reg->var2Index() ].empty(), "stack is empty here");
                        ab->rhs_[i] = names[ reg->var2Index() ].top();
                    }
                }
            }

            // replace var on the left hand side in all cases
            for (size_t i = 0; i < ab->numLhs_; ++i)
            {
                if ( !ab->lhs_[i]->isSSA() )
                {
#ifdef SWIFT_DEBUG
                    Reg* reg = function_->newSSA(ab->lhs_[i]->type_, &ab->lhs_[i]->id_);
#else // SWIFT_DEBUG
                    Reg* reg = function_->newSSA(ab->lhs_[i]->type_);
#endif // SWIFT_DEBUG

                    swiftAssert(size_t(-ab->lhsOldVarNr_[i]) < names.size(), "index out of bounds");
                    names[ -ab->lhsOldVarNr_[i] ].push(reg);
                    ab->lhs_[i] = reg;
                }
            }
        }  // type checking
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

            if ( names[ -phi->lhsOldVarNr_[0] ].empty() )
                continue; // var not found

            phi->rhs_[j] = names[ -phi->oldResultNr() ].top();
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
        AssignmentBase* ab = dynamic_cast<AssignmentBase*>(iter->value_);

        if (ab)
        {
            // for each var on the left hand side
            for (size_t i = 0; i < ab->numLhs_; ++i)
            {
                if (ab->lhsOldVarNr_[i] < 0) // if this is a var
                {
                    swiftAssert( names[- ab->lhsOldVarNr_[i]].size() > 0, "cannot pop here");
                    names[ - ab->lhsOldVarNr_[i] ].pop();
                }
            } // for ech var on the left hand side
        } // type checking
    } // for each AssignmentBase
}

/*
 * def-use-chains
 */

void CFG::calcDef()
{
    // knows the current BB in the iteration
    BBNode* currentBB;

    // iterate over the instruction list 
    INSTRLIST_EACH(iter, instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(LabelInstr) )
            currentBB = labelNode2BBNode_[iter]; // new basic block
        else
        {
            AssignmentBase* ab = dynamic_cast<AssignmentBase*>(instr);

            if (ab)
            {
                // for each var on the lhs
                for (size_t i = 0; i < ab->numLhs_; ++i)
                    ab->lhs_[i]->def_.set(iter, currentBB); // store def
            }
        }
    }
}

void CFG::calcUse()
{
    // for each var
    REGMAP_EACH(iter, function_->vars_)
    {
        Reg* var = iter->second;

        /* 
         * start with the definition of the var and walk the dominator tree
         * to find all uses since every use of a var in an SSA form program is
         * dominated by its definition except for phi-instructions.
         */
        calcUse(var, var->def_.bbNode_);
    }

    // now find all phi functions
    CFG_RELATIVES_EACH(bbIter, nodes_)
    {
        BBNode* bbNode = bbIter->value_;
        BasicBlock* bb = bbNode->value_;

        for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
        {
            swiftAssert(typeid(*iter->value_) == typeid(PhiInstr),
                "must be a PhiInstr here");
            PhiInstr* phi = (PhiInstr*) iter->value_;

            for (size_t i = 0; i < phi->numRhs_; ++i)
            {
                swiftAssert(typeid(*phi->rhs_[i]) == typeid(Reg),
                    "must be a Reg here");
                Reg* var = (Reg*) phi->rhs_[i];

                // put this as first use so liveness analysis will be a bit faster
                var->uses_.prepend( DefUse(iter, bbNode) );
            }
        }
    }

    /* 
     * use this for debugging of the def-use calculation
     */
#if 0
    REGMAP_EACH(iter, function_->vars_)
    {
        Reg* var = iter->second;
        std::cout << var->toString() << std::endl;
        USELIST_EACH(iter, var->uses_)
            std::cout << "\t" << iter->value_.instr_->value_->toString() << std::endl;
    }
#endif
}

void CFG::calcUse(Reg* var, BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;

    /* 
     * iterate over the instruction list in this bb and find all uses 
     * while ignoring phi functions
     */
    for (InstrNode* iter = bb->firstOrdinary_; iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;
        AssignmentBase* ab = dynamic_cast<AssignmentBase*>(instr);

        if (ab)
        {
            if ( ab->isRegUsed(var) )
                var->uses_.append( DefUse(iter, bbNode) );
        }
    } // for each instruction

    // for each child of bb in the dominator tree
    BBLIST_EACH(iter, bb->domChildren_)
        calcUse(var, iter->value_);
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
    calcDef();
    calcUse();
}

void CFG::reconstructSSAForm() 
{
    placePhiFunctions();
    renameVars();
    calcDef();
    calcUse();
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
        if (result[i] == '#')
            result[i] = '_';
    }

    return result;
}

std::string CFG::dumpIdoms() const
{
    std::ostringstream oss;

    for (size_t i = 0; i < size(); ++i)
        oss << '\t' << postOrder_[i]->value_->name() << " -> " << idoms_[i]->value_->name() << std::endl;

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

        BBLIST_EACH(iter, bb->value_->domChildren_)
            oss << iter->value_->value_->name() << " ";

        oss << std::endl;
    }

    return oss.str();
}

} // namespace me
