#include "cfg.h"

#include "me/functab.h"

/*
    constructor and destructor
*/

CFG::CFG(Function* function)
    : function_(function)
    , instrList_(function->instrList_)
{}

CFG::~CFG()
{
    delete[] idoms_;
}

/*
    methods
*/

void CFG::calcCFG()
{
    swiftAssert( typeid( *instrList_.first()->value_ ) == typeid(LabelInstr),
        "first instruction of a function must be a LabelInstr");
    swiftAssert( typeid( *instrList_.last ()->value_ ) == typeid(LabelInstr),
        "last instruction of a function must be a LabelInstr");

    InstrList::Node* end = 0;
    InstrList::Node* begin = 0;
    BBNode* prevBB = 0;

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
                BBNode* bb = insert( new BasicBlock(begin, end) );

                // keep acount of the label node and the basic block in the map
                labelNode2BBNode_[begin] = bb;

                prevBB = bb; // now this is the previous basic block
            }
        }

#ifdef SWIFT_DEBUG
        // check in the debug version whether a GotoInstr or a BranchInstr is followed by a LabelInstr
        if ( typeid(*instr) == typeid(BranchInstr) || typeid(*instr) == typeid(GotoInstr) )
        {
            swiftAssert( typeid( *iter->next()->value_ ) == typeid(LabelInstr),
                "BranchInstr or GotoInstr is not followed by a LabelInstr");
        }
#endif // SWIFT_DEBUG
    }
    // prevBB now holds the last basic block

    /*
        now we know of all starting labels and their associated basic block
        so let us calculate the CFG
    */

    BBNode* currentBB = 0;

    // iterate once again over the instruction list in order to connect the blocks properly
    INSTRLIST_EACH(iter, instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(LabelInstr) )
        {
            BBNode* prevBB = currentBB;
            // we have found a new basic block
            currentBB = labelNode2BBNode_[iter];

            if (currentBB == 0)
                break; // -> this is the last label

            // check whether there is already a connection between currentBB and prevBB
            if ( prevBB && currentBB->pred_.search(prevBB) == currentBB->pred_.sentinel() )
            {
                // no -> test whether a connection is needed
                InstrBase* lastInstrOfPrevBB = prevBB->value_->end_->prev()->value_;

                if ( typeid(*lastInstrOfPrevBB) == typeid(AssignInstr) )
                    prevBB->link(currentBB);
            }
        }
        else if ( typeid(*instr) == typeid(GotoInstr) )
        {
            GotoInstr* gi = (GotoInstr*) instr;

            gi->succBB_ = labelNode2BBNode_[ gi->labelNode_ ];
            currentBB->link(gi->succBB_);
        }
        else if ( typeid(*instr) == typeid(BranchInstr) )
        {
            BranchInstr* bi = (BranchInstr*) instr;

            bi->falseBB_ = labelNode2BBNode_[bi->falseLabelNode_];
            currentBB->link(bi->falseBB_);

            bi->trueBB_ = labelNode2BBNode_[bi->trueLabelNode_];
            currentBB->link(bi->trueBB_);
        }
        else if ( typeid(*instr) == typeid(AssignInstr) )
        {
            // if we have an assignment to a var update this in the map
            AssignInstr* ai = (AssignInstr*) instr;

            if ( ai->result_->isVar() )
            {
                int resultNr = ai->result_->regNr_;
                currentBB->value_->vars_[resultNr] = ai->result_;

                if ( firstOccurance_.find(resultNr) == firstOccurance_.end() )
                    firstOccurance_[resultNr] = currentBB;
            }
        }
    }

    /*
        Create the two additional blocks ENTRY and EXIT.
    */
    entry_ = insert( new BasicBlock(0, instrList_.first()) );
    exit_  = insert( new BasicBlock(instrList_.last(), 0) );

    // connect entry with first regular basic block
    BBNode* firstBB = labelNode2BBNode_[instrList_.first()];
    entry_->link(firstBB);

    // connect exit with last regular basic block
    BBNode* lastBB = prevBB; // prevBB holds the last basic block
    lastBB->link(exit_);

    calcPostOrder(entry_);
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
        Now we can walk the idom array to get each dominator.
        Let's compute the reserve set, too, so we can easily access
        children of basic blocks and not only their parents.
    */

    for (size_t i = 0; i < size(); ++i)
    {
        BBNode* bb = postOrder_[i];
        swiftAssert(i == bb->postOrderIndex_, "i and postOrderIndex_ bust be consistent");
        idoms_[i]->value_->domChildren_.append(bb); // append child
    }

    // remove the entry -> entry cycle from the domChildren_
    entry_->value_->domChildren_.erase( entry_->value_->domChildren_.search(entry_) );
    swiftAssert(entry_->value_->domChildren_.size() == 1, "the entry must have exactly 1 child");
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

void CFG::placePhiFunctions()
{
    /*
        hasAlready[bb->postOrderIndex_] knows
        whether an phi-function in bb has already been put
    */
    int hasAlready[size()];

    /*
        hasBeenAdded[bb->postOrderIndex_] knows
        whether bb has already been added to the work list
    */
    int hasBeenAdded[size()];

    // set all blocks to "not added" and "not already"
    memset(hasAlready, 0, sizeof(int) * size()); // init to 0
    memset(hasBeenAdded, 0, sizeof(int) * size()); // init to 0

    int iterCount = 0;

    // for each var
    REGMAP_EACH(iter, function_->vars_)
    {
        ++iterCount;

        PseudoReg* var = iter->second;

        // if it is a temp continue
        if ( !var->isVar() )
            continue;

        swiftAssert(var->regNr_, "this is not a var");

        BBList work;

        // init work list with all basic blocks which assign to var
        for (size_t i = 0; i < size(); ++i)
        {
            BBNode* bb = postOrder_[i];

            if ( bb->value_->vars_.find(var->regNr_) != bb->value_->vars_.end() )
            {
                hasBeenAdded[bb->postOrderIndex_] = iterCount;
                work.append(bb);
            }

        }

        /*
            mark dominance frontier of the basic block with the first occurance
            of var as hasAlready
        */
        BBNode* firstBB = firstOccurance_.find(var->regNr_)->second;

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
                instrList_.insert(df->value_->begin_, phiInstr);

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

    std::vector< std::stack<PseudoReg*> > names;
    // since index 0 is reserved for literals the stack-array's size must be increased by one
    names.resize( -vars.begin()->first + 1 );

    // start with the first real basic block
    rename( entry_->value_->domChildren_.first()->value_, names );

    /*
        all vars i.e. regNr < 0 are not needed anymore from this point
    */

    // for each basic block
    for (size_t i = 0; i < size(); ++i)
        postOrder_[i]->value_->vars_.clear();

    // for all vars, no temps
    REGMAP_EACH(iter, vars)
    {
        delete iter->second;
        vars.erase(iter);
    }
}

void CFG::rename(BBNode* bb, std::vector< std::stack<PseudoReg*> >& names)
{
    // for each instruction -> start with the first instruction which is followed by the leading LabelInstr
    for (InstrList::Node* iter = bb->value_->begin_->next(); iter != bb->value_->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(PhiInstr) )
        {
            PhiInstr* phi = (PhiInstr*) instr;

#ifdef SWIFT_DEBUG
            PseudoReg* reg = function_->newTemp(phi->result_->regType_, &phi->result_->id_);
#else // SWIFT_DEBUG
            PseudoReg* reg = function_->newTemp(phi->result_->regType_);
#endif // SWIFT_DEBUG

            swiftAssert(size_t(-phi->oldResultVar_) < names.size(), "index out of bounds");
            names[ -phi->oldResultVar_ ].push(reg);
            phi->result_ = reg;
            continue;
        }
        else if ( typeid(*instr) == typeid(AssignInstr) )
        {
            AssignInstr* ai = (AssignInstr*) instr;

            // replace vars on the right hand side
            if ( ai->op1_->isVar() )
            {
                swiftAssert( !names[ ai->op1_->var2Index() ].empty(), "stack is empty here");
                ai->op1_ = names[ ai->op1_->var2Index() ].top();
            }
            if ( ai->op2_ && ai->op2_->isVar() )
            {
                swiftAssert( !names[ ai->op2_->var2Index() ].empty(), "stack is empty here");
                ai->op2_ = names[ ai->op2_->var2Index() ].top();
            }

            // replace var on the left hand side
            if ( ai->result_->isVar() )
            {
#ifdef SWIFT_DEBUG
                PseudoReg* reg = function_->newTemp(ai->result_->regType_, &ai->result_->id_);
#else // SWIFT_DEBUG
                PseudoReg* reg = function_->newTemp(ai->result_->regType_);
#endif // SWIFT_DEBUG

                swiftAssert(size_t(-ai->oldResultVar_) < names.size(), "index out of bounds");
                names[ -ai->oldResultVar_ ].push(reg);
                ai->result_ = reg;
            }
        }
        else if ( typeid(*instr) == typeid(BranchInstr) )
        {
            BranchInstr* bi = (BranchInstr*) instr;

            // replace boolean var
            if ( bi->boolReg_->isVar() )
            {
                swiftAssert( !names[ bi->boolReg_->var2Index() ].empty(), "stack is empty here");
                bi->boolReg_ = names[ bi->boolReg_->var2Index() ].top();
            }
        }
    }

    // for each successor of bb
    RELATIVES_EACH(iter, bb->succ_)
    {
        BBNode* succ = iter->value_;
        size_t j = succ->whichPred(bb);

        // for each phi function in succ -> start with the first instruction which is followed by the leading LabelInstr
        for (InstrList::Node* iter = succ->value_->begin_->next(); iter != succ->value_->end_; iter = iter->next())
        {
            PhiInstr* phi = dynamic_cast<PhiInstr*>(iter->value_);

            if (!phi)
                break; // no further phi functions in this basic block

            if ( names[ -phi->oldResultVar_ ].empty() )
                continue; // var not found

            phi->args_[j] = names[ -phi->oldResultVar_ ].top();
            phi->sourceBBs_[j] = bb;
        }
    }

    // for each child of bb in the dominator tree
    BBLIST_EACH(iter, bb->value_->domChildren_)
    {
        BBNode* domChild = iter->value_;

        // omit special exit node
        if ( domChild->value_->isExit() )
            continue;

        rename(domChild, names);
    }

    // for each AssignInstr in bb
    for (InstrList::Node* iter = bb->value_->begin_->next(); iter != bb->value_->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(PhiInstr) )
        {
            PhiInstr* phi = (PhiInstr*) instr;

            swiftAssert(phi->oldResultVar_ < 0, "this should be a var");
            swiftAssert( names[-phi->oldResultVar_].size() > 0, "cannot pop here");
            names[- phi->oldResultVar_].pop();
        }
        else if ( typeid(*instr) == typeid(AssignInstr) )
        {
            AssignInstr* ai = dynamic_cast<AssignInstr*>(instr);

            if (ai->oldResultVar_ < 0) // if this is a var
            {
                swiftAssert( names[- ai->oldResultVar_].size() > 0, "cannot pop here");
                names[- ai->oldResultVar_].pop();
            }
        }
    }
}

void CFG::calcDef()
{
    // knows the current BB in the iteration
    BBNode* currentBB;

    // iterate over the instruction list and find all definitions
    INSTRLIST_EACH(iter, instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(AssignInstr) )
            ((AssignInstr*) instr)->res_->def_.set(instr, currentBB);
        else if (typeid(*instr) == typeid(LabelInstr) )
            currentBB == labelNode2BBNode_( (LabelInstr*) instr );
    }
}

void CFG::calcUse()
{
    REGMAP_EACH(iter, function->vars_)
    {
        PseudoReg* var = iter->value_;

        calcUse(var, var->def.bbNode_);
    }
}

void CFG::calcUse(PseudoReg* var, BBNode* bb)
{
    // iterate over the instruction list in this bb and find all uses
    for (InstrList::Node* iter = bb->begin_; iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(AssignInstr) )
        {
            AssignInstr* ai = (AssignInstr*) instr;

            // note that a = b + b can cause to a double entry in the list
            if (ai->op1_ == var)
                var->uses_.append( DefUse(instr, bb) );
            if (ai->op1_ == var)
                var->uses_.append( DefUse(instr, bb) );
        }
    }

    // for each child of bb in the dominator tree
    BBLIST_EACH(iter, bb->value_->domChildren_)
        calcUse(var, iter->value_);
}

/*
    dump methods
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
