#include "functab.h"

#include <algorithm>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

using namespace std;

FuncTab* functab = 0;

// -----------------------------------------------------------------------------

Function::~Function()
{
    // delete all instructions
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
        delete iter->value_;

    // delete all pseudo regs
    for (RegMap::iterator iter = in_    .begin(); iter != in_   .end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = inout_ .begin(); iter != inout_.end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = out_   .begin(); iter != out_  .end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = vars_  .begin(); iter != vars_ .end(); ++iter)
        delete iter->second;

    // delete all basic blocks
    for (size_t i = 0; i < numBBs_; ++i)
        delete bbs_[i];

    delete id_;
    delete[] idoms_;
    delete[] bbs_;
}

inline void Function::insert(PseudoReg* reg)
{
    swiftAssert(reg->regNr_ != PseudoReg::LITERAL, "reg is a LITERAL");

    pair<RegMap::iterator, bool> p
        = vars_.insert( make_pair(reg->regNr_, reg) );

    swiftAssert(p.second, "there is already a reg with this regNr in the map");
}


PseudoReg* Function::newVar(PseudoReg::RegType regType, int varNr)
{
    swiftAssert(varNr < 0, "varNr must be less than zero");
    PseudoReg* reg = new PseudoReg(regType, varNr);
    insert(reg);

    return reg;
}

PseudoReg* Function::newTemp(PseudoReg::RegType regType)
{
    PseudoReg* reg = new PseudoReg(regType, regCounter_);
    insert(reg);

    ++regCounter_;

    return reg;
}

void Function::calcCFG()
{
    swiftAssert( typeid( *instrList_.first()->value_ ) == typeid(LabelInstr),
        "first instruction of a function must be a LabelInstr");
    swiftAssert( typeid( *instrList_.last ()->value_ ) == typeid(LabelInstr),
        "last instruction of a function must be a LabelInstr");

    InstrList::Node* end = 0;
    InstrList::Node* begin = 0;
    BasicBlock* prevBB = 0;

    // iterate over the instruction list and find basic blocks
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        if ( typeid(*iter->value_) == typeid(LabelInstr) )
        {
            begin = end;
            end = iter;

            if (begin) // we have found the next basic block
            {
                BasicBlock* bb = new BasicBlock(begin, end);
                // keep acount of the label node and the basic block in the map
                labelNode2BB_[begin] = bb;
                // and count basic blocks
                ++numBBs_;

                prevBB = bb; // now this is the previous basic block
            }
        }

#ifdef SWIFT_DEBUG
        // check in the debug version whether a GotoInstr or a BranchInstr is followed by a LabelInstr
        if ( typeid(*iter->value_) == typeid(BranchInstr) || typeid(*iter->value_) == typeid(GotoInstr) )
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

    BasicBlock* currentBB = 0;

    // iterate once again over the instruction list in order to connect the blocks properly
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        if ( typeid(*iter->value_) == typeid(LabelInstr) )
        {
            BasicBlock* prevBB = currentBB;
            // we have found a new basic block
            currentBB = labelNode2BB_[iter];

            if (currentBB == 0)
                break; // -> this is the last label

            // check whether there is already a connection between currentBB and prevBB
            if ( prevBB && currentBB->pred_.find(prevBB) == currentBB->pred_.end() )
            {
                // no -> test whether a connection is needed
                InstrBase* lastInstrOfPrevBB = prevBB->end_->prev()->value_;

                if ( typeid(*lastInstrOfPrevBB) == typeid(AssignInstr) )
                    prevBB->connectBB(currentBB);
            }
        }
        else if ( typeid(*iter->value_) == typeid(GotoInstr) )
        {
            BasicBlock* succ = labelNode2BB_[ ((GotoInstr*) iter->value_)->labelNode_ ];
            currentBB->connectBB(succ);
        }
        else if ( typeid(*iter->value_) == typeid(BranchInstr) )
        {
            BasicBlock* falseBlock = labelNode2BB_[ ((BranchInstr*) iter->value_)->falseLabelNode_ ];
            currentBB->connectBB(falseBlock);

            BasicBlock* trueBlock = labelNode2BB_[ ((BranchInstr*) iter->value_)->trueLabelNode_ ];
            currentBB->connectBB(trueBlock);
        }
        else if ( typeid(*iter->value_) == typeid(AssignInstr) )
        {
            // if we have an assignment to a var update this in the map
            AssignInstr* ai = (AssignInstr*) iter->value_;
            if ( ai->result_->isVar() )
                currentBB->vars_[ai->result_->regNr_] = ai->result_;
        }
    }

    /*
        Create the two additional blocks ENTRY and EXIT.
    */
    entry_ = new BasicBlock( 0, instrList_.first() );
    exit_  = new BasicBlock( instrList_.last(), 0 );
    // do not increment numBBs_, it has already been initialized with 2

    // connect entry with exit
// FIXME connect or not?
//     entry_->connectBB(exit_);

    // connect entry with first regular basic block
    BasicBlock* firstBB = labelNode2BB_[instrList_.first()];
    entry_->connectBB(firstBB);

    // connect exit with last regular basic block
    BasicBlock* lastBB = prevBB; // prevBB holds the last basic block
    lastBB->connectBB(exit_);

    // init bbs array
    bbs_ = new BasicBlock*[numBBs_];
    // assign post order nr to all basic blocks
    postOrderWalk(entry_);
}

void Function::postOrderWalk(BasicBlock* bb)
{
    for (BBSet::iterator iter = bb->succ_.begin(); iter != bb->succ_.end(); ++iter)
    {
        if ( (*iter)->isReached() )
            continue;

        postOrderWalk(*iter);
    }

    // process this node
    bbs_[indexCounter_] = bb;
    bb->index_ = indexCounter_;
    ++indexCounter_;
}

void Function::calcDomTree()
{
    // init dom array
    idoms_ = new BasicBlock*[numBBs_];
    memset(idoms_, 0, sizeof(BasicBlock*) * numBBs_);

    BasicBlock* entry = entry_;
    idoms_[entry->index_] = entry;
    bool changed = true;

    while (changed)
    {
        changed = false;

        // iterate over the CFG in reverse post-order
        for (size_t i = numBBs_ - 2; long(i) >= 0; --i) // void entry node
        {
            // current node
            BasicBlock* bb = bbs_[i];
            swiftAssert(bb != entry_, "do not process the entry node");

            // pick one which has been processed
            BasicBlock* newIdom = 0;
            for (BBSet::iterator iter = bb->pred_.begin(); iter != bb->pred_.end(); ++iter)
            {
                if ( (*iter)->index_ > i)
                {
                    newIdom = *iter;
                    // found a processed node
                    break;
                }
            }

            swiftAssert(newIdom, "no processed predecessor found");

            // for all other predecessors
            for (BBSet::iterator iter = bb->pred_.begin(); iter != bb->pred_.end(); ++iter)
            {
                if (bb == newIdom)
                    continue;

                if ( idoms_[(*iter)->index_] != 0 )
                    newIdom = intersect(*iter, newIdom);
            }

            if (idoms_[bb->index_] != newIdom )
            {
                idoms_[bb->index_] = newIdom;
                changed = true;
            }
        } // for
    } // while

    /*
        Now we can walk the idom array to get each dominator.
        Let's compute the reserve set, too, so we can easily access
        children of basic blocks and not only their parents.
    */

    for (size_t i = 0; i < numBBs_; ++i)
    {
        BasicBlock* bb = bbs_[i];
        swiftAssert(i == bb->index_, "i and index_ bust be consistent");
        BasicBlock* idom = idoms_[i];
        idom->domChildren_.insert(bb); // append child
    }

    // remove the entry -> entry cycle from the domChildren_
    entry->domChildren_.erase(entry);
    swiftAssert(entry->domChildren_.size() == 1, "the entry must have exactly 1 child");
}

BasicBlock* Function::intersect(BasicBlock* b1, BasicBlock* b2)
{
    BasicBlock* finger1 = b1;
    BasicBlock* finger2 = b2;

    while (finger1->index_ != finger2->index_)
    {
        while (finger1->index_ < finger2->index_)
            finger1 = idoms_[finger1->index_];
        while (finger2->index_ < finger1->index_)
            finger2 = idoms_[finger2->index_];
    }

    return finger1;
}

void Function::calcDomFrontier()
{
    for (size_t i = 0; i < numBBs_; ++i)
    {
        BasicBlock* bb = bbs_[i];

        // if the number of predecessors >= 2
        if (bb->pred_.size() >= 2)
        {
            // for all predecessors of bb
            for (BBSet::iterator iter = bb->pred_.begin(); iter != bb->pred_.end(); ++iter)
            {
                BasicBlock* runner = *iter;

                while ( runner != idoms_[bb->index_] )
                {
                    // add bb to the runner's dominance frontier
                    runner->domFrontier_.insert(bb);
                    runner = idoms_[runner->index_];
                }
            }
        }
    }
}

void Function::placePhiFunctions()
{
    /*
        hasAlready[bb->index_] knows
        whether an phi-function in bb has already been put
    */
    int hasAlready[numBBs_];

    /*
        hasBeenAdded[bb->index_] knows
        whether bb has already been added to the work list
    */
    int hasBeenAdded[numBBs_];

    // set all blocks to "not added" and "not already"
    memset(hasAlready, 0, sizeof(int) * numBBs_); // init to 0
    memset(hasBeenAdded, 0, sizeof(int) * numBBs_); // init to 0

    int iterCount = 0;

    // for each var
    for (RegMap::iterator iter = vars_.begin(); iter != vars_.end(); ++iter)
    {
        ++iterCount;

        PseudoReg* var = iter->second;
        swiftAssert(var->regNr_, "this is not a var");

        BBList work;

        // init work list with all basic blocks which assign to var
        for (size_t i = 0; i < numBBs_; ++i)
        {
            BasicBlock* bb = bbs_[i];

            if ( bb->vars_.find(var->regNr_) != bb->vars_.end() )
            {
                hasBeenAdded[bb->index_] = iterCount;
                work.append(bb);
            }

        }

        // for each basic block in the work list
        while ( !work.empty() )
        {
            // take a basic block from the list
            BasicBlock* bb = work.first()->value_;
            work.removeFirst();

            // for each basic block from DF(bb)
            for (BBSet::iterator iter = bb->domFrontier_.begin(); iter != bb->domFrontier_.end(); ++iter)
            {
                BasicBlock* df = *iter;

                // do we already have a phi function for this node and this var?
                if ( hasAlready[df->index_] >= iterCount )
                    continue; // yes -> so process next one
                // else

                // place phi function
                PhiInstr* phiInstr = new PhiInstr( var, df->pred_.size() );
                instrList_.insert(df->begin_, phiInstr);

                // update data structures
                hasAlready[df->index_] = iterCount;
                if (hasBeenAdded[df->index_] == false)
                {
                    hasBeenAdded[df->index_] = iterCount;
                    work.append(df);
                }
            }
        } // while
    } // for each var
}

void Function::renameVars()
{
    stack<PseudoReg*>* names = new stack<PseudoReg*>[ vars_.size() ];

    // start with the first real basic block
    rename( *entry_->domChildren_.begin(), names );

    delete[] names;
}

void Function::rename(BasicBlock* bb, stack<PseudoReg*>* names)
{
    // for each phi function in succ -> start with the first instruction which is followed by the leading LabelInstr
    for (InstrList::Node* iter = bb->begin_->next(); iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(PhiInstr) )
        {
            PhiInstr* phi = (PhiInstr*) instr;

            PseudoReg* reg = newTemp(phi->result_->regType_);
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
                PseudoReg* reg = newTemp(ai->result_->regType_);
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
    for (BBSet::iterator iter = bb->succ_.begin(); iter != bb->succ_.end(); ++iter)
    {
        BasicBlock* succ = *iter;
        size_t j = succ->whichPred(bb);

        // for each phi function in succ -> start with the first instruction which is followed by the leading LabelInstr
        for (InstrList::Node* iter = succ->begin_->next(); iter != succ->end_; iter = iter->next())
        {
            PhiInstr* phi = dynamic_cast<PhiInstr*>(iter->value_);

            if (!phi)
                break; // no further phi functions in this basic block

            if ( names[ -phi->oldResultVar_ ].empty() )
                continue; // var not found

            phi->args_[j] = names[ -phi->oldResultVar_ ].top();
        }
    }

    // for each child of bb in the dominator tree
    for (BBSet::iterator iter = bb->domChildren_.begin(); iter != bb->domChildren_.end(); ++iter)
    {
        // omit special exit node
        if ( (*iter)->isExit() )
            continue;

        rename(*iter, names);
    }

    // for each AssignInstr in bb
    for (InstrList::Node* iter = bb->begin_->next(); iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(PhiInstr) )
        {
            PhiInstr* phi = (PhiInstr*) iter->value_;

            swiftAssert(phi->oldResultVar_ < 0, "this should be a var");
            swiftAssert( names[-phi->oldResultVar_].size() > 0, "cannot pop here");
            names[- phi->oldResultVar_].pop();
        }
        else if ( typeid(*instr) == typeid(AssignInstr) )
        {
            AssignInstr* ai = dynamic_cast<AssignInstr*>(iter->value_);

            if (ai->oldResultVar_ < 0) // if this is a var
            {
                swiftAssert( names[- ai->oldResultVar_].size() > 0, "cannot pop here");
                names[- ai->oldResultVar_].pop();
            }
        }
    }
}

/*
    dump functions
*/

void Function::dumpSSA(ofstream& ofs)
{
    ofs << endl
        << "--------------------------------------------------------------------------------"
        << endl;
    ofs << *id_ << ":" << endl;

    // for all instructions in this function
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        // don't print a tab character if this is a label
        if ( typeid(*iter->value_) != typeid(LabelInstr) )
            ofs << '\t';
        ofs << iter->value_->toString() << endl;
    }

    // print idoms
    ofs << endl
        << "IDOMS:" << endl;
    for (size_t i = 0; i < numBBs_; ++i)
        ofs << '\t' << bbs_[i]->toString() << " -> " << idoms_[i]->toString() << endl;

    // print domChildren
    ofs << endl
        << "DOM CHILDREN:" << endl;
    for (size_t i = 0; i < numBBs_; ++i)
    {
        BasicBlock* bb = bbs_[i];

        ofs << '\t' << bb->toString() << ":\t";

        for (BBSet::iterator iter = bb->domChildren_.begin(); iter != bb->domChildren_.end(); ++iter)
             ofs << (*iter)->toString() << ' ';

        ofs << endl;
    }

    // print dominance frontier
    ofs << endl
        << "DOMINANCE FRONTIER:" << endl;
    for (size_t i = 0; i < numBBs_; ++i)
    {
        BasicBlock* bb = bbs_[i];

        ofs << '\t' << bb->toString() << ":\t";

        for (BBSet::iterator iter = bb->domFrontier_.begin(); iter != bb->domFrontier_.end(); ++iter)
            ofs << (*iter)->toString() << " ";

        ofs << endl;
    }
}

void Function::dumpDot(const string& baseFilename)
{
    ostringstream oss;
    oss << baseFilename << '.' << *id_ << ".dot";

    ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...

    // prepare graphviz dot file
    ofs << "digraph " << *id_ << " {" << endl << endl;

    // iterate over all basic blocks
    for (size_t i = 0; i < numBBs_; ++i)
    {
        BasicBlock* bb = bbs_[i];
        // start a new node
        ofs << bb->toString() << " [shape=box, label=\"\\" << endl;

        if (bb->begin_ == 0)
            ofs << "\tENTRY\\n\\" << endl;
        else if (bb->end_ == 0)
            ofs << "\tEXIT\\n\\" << endl;
        else
        {
            // for all instructions in this basic block except the last LabelInstr
            for (InstrList::Node* instrIter = bb->begin_; instrIter != bb->end_; instrIter = instrIter->next())
                ofs << '\t' << instrIter->value_->toString() << "\\n\\" << endl; // print instruction
        }
        // close this node
        ofs << "\"]" << endl << endl;
    }

    // iterate over all basic blocks in order to print connections
    for (size_t i = 0; i < numBBs_; ++i)
    {
        BasicBlock* bb = bbs_[i];
// use this switch to print predecessors instead of successors
#if 0
        // for all predecessors
        for (BBSet::iterator pred = bb->pred_.begin(); pred != bb->pred_.end(); ++pred)
            ofs << bb->toString() << " -> " << (*pred)->toString() << endl;
#else
        // for all successors
        for (BBSet::iterator succ = bb->succ_.begin(); succ != bb->succ_.end(); ++succ)
            ofs << bb->toString() << " -> " << (*succ)->toString() << endl;
#endif
    }

    // end graphviz dot file
    ofs << endl << '}' << endl;
    ofs.close();
}

// -----------------------------------------------------------------------------

FunctionTable::~FunctionTable()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        delete iter->second;
}

Function* FunctionTable::insertFunction(string* id)
{
    current_ = new Function(id);
    functions_.insert( make_pair(id, current_) );

    return current_;
}

PseudoReg* FunctionTable::newVar(PseudoReg::RegType regType, int varNr)
{
    return current_->newVar(regType, varNr);
}

PseudoReg* FunctionTable::newTemp(PseudoReg::RegType regType)
{
    return current_->newTemp(regType);
}

PseudoReg* FunctionTable::lookupReg(int regNr)
{
    RegMap::iterator regIter = current_->vars_.find(regNr);

    if ( regIter == current_->vars_.end() )
        return 0;
    else
        return regIter->second;
}

void FunctionTable::appendInstr(InstrBase* instr)
{
    current_->instrList_.append(instr);
}

void FunctionTable::appendInstrNode(InstrList::Node* node)
{
    current_->instrList_.append(node);
}

void FunctionTable::buildUpME()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
    {
        Function* function = iter->second;
        function->calcCFG();
        function->calcDomTree();
        function->calcDomFrontier();
        function->placePhiFunctions();
        function->renameVars();
    }
}

void FunctionTable::dumpSSA()
{
    ostringstream oss;
    oss << filename_ << ".ssa";

    ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...
    ofs << oss.str() << ":" << endl;

    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->dumpSSA(ofs);

    // finish
    ofs.close();
}

void FunctionTable::dumpDot()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->dumpDot(filename_);
}
