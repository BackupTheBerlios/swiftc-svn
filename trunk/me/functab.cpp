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
    delete id_;
    delete[] doms_;

    // delete all pseudo regs
    for (RegMap::iterator iter = in_    .begin(); iter != in_   .end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = inout_ .begin(); iter != inout_.end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = out_   .begin(); iter != out_  .end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = vars_  .begin(); iter != vars_ .end(); ++iter)
        delete iter->second;

    // delete all instructions
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
        delete iter->value_;

    // delete all basic blocks
    for (BBList::Node* iter = bbList_.first(); iter != bbList_.sentinel(); iter = iter->next())
        delete iter->value_;
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
                bbList_.append( bb );

                // connect new found basic block with the previous one if it exists
                if (prevBB)
                    prevBB->connectBB(bb);

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

    // iterate once again over the instruction list
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        if ( typeid(*iter->value_) == typeid(LabelInstr) )
            currentBB = labelNode2BB_[iter]; // we have found a new basic block
        else if ( typeid(*iter->value_) == typeid(GotoInstr) )
        {
            BasicBlock* succ = labelNode2BB_[ ((GotoInstr*) iter->value_)->labelNode_ ];
            currentBB->connectBB(succ);
        }
        else if ( typeid(*iter->value_) == typeid(BranchInstr) )
        {
            /*
                do not connect the current basic block with the one of the
                trueLabel since they were already connected in the first pass
            */
            BasicBlock* succ = labelNode2BB_[ ((BranchInstr*) iter->value_)->falseLabelNode_ ];
            currentBB->connectBB(succ);
        }
        else if ( typeid(*iter->value_) == typeid(AssignInstr) )
        {
            // if we have an assignment to a real var update this in the map
            AssignInstr* ai = (AssignInstr*) iter->value_;
            if ( ai->result_->isVar() )
                currentBB->varNr_[ai->result_->varNr_] = ai->result_;
        }
    }

    /*
        Create the two additional blocks ENTRY and EXIT.
    */
    BasicBlock* entry = new BasicBlock( 0, instrList_.first() );
    BasicBlock* exit  = new BasicBlock( instrList_.last(), 0 );
    bbList_.prepend(entry);
    bbList_.append(exit);

    // connect entry with exit
    entry->connectBB(exit);

    // connect entry with first regular basic block
    BasicBlock* firstBB = labelNode2BB_[instrList_.first()];
    entry->connectBB(firstBB);

    // connect exit with last regular basic block
    BasicBlock* lastBB = prevBB; // prevBB holds the last basic block
    lastBB->connectBB(exit);

    // store number of basic blocks
    numBBs_ = bbList_.size();
}

void Function::r_reversePostOrderWalk(ProcessBBFunc process, BasicBlock* bb)
{
    bb->reached_ = reachedValue_;
    process(bb);

    for (BBSet::iterator iter = bb->pred_.begin(); iter != bb->pred_.end(); ++iter)
    {
        if ( (*iter)->reached_ == reachedValue_ )
            continue;

        r_reversePostOrderWalk(process, (*iter));
    }
}

void assignPostOrderNr(BasicBlock* bb)
{
    // HACK
    static int counter = 0;
    bb->postOrderNr_ = counter;
    ++counter;
}

void calcDoms(BasicBlock* bb)
{
// TODO
}

void Function::calcDomTree()
{
return;
    // assign post order nr to all basic blocks
    reversePostOrderWalk(assignPostOrderNr);

    // init dom array
    doms_ = new BasicBlock*[numBBs_];
    memset(doms_, 0, sizeof(BasicBlock*) * numBBs_);

    BasicBlock* entry = getEntry();
    doms_[entry->postOrderNr_] = entry;

    bool changed = true;

    while (changed)
    {
        changed = false;

        // iterate over the CFG in reverse post-order
        reversePostOrderWalk(calcDoms);
    }
}

BasicBlock* Function::intersect(BasicBlock* b1, BasicBlock* b2)
{
    BasicBlock* finger1 = b1;
    BasicBlock* finger2 = b2;

    while (finger1->postOrderNr_ != finger2->postOrderNr_)
    {
        while (finger1->postOrderNr_ < finger2->postOrderNr_)
            finger1 = doms_[finger1->postOrderNr_];
        while (finger2->postOrderNr_ < finger1->postOrderNr_)
            finger2 = doms_[finger2->postOrderNr_];
    }

    return finger1;
}

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
}

void Function::dumpDot(const string& baseFilename)
{
    ostringstream oss;
    oss << baseFilename << '.' << *id_ << ".dot";

    ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...

    // prepare graphviz dot file
    ofs << "digraph " << *id_ << " {" << endl << endl;

    // iterate over all basic blocks
    for (BBList::Node* bbIter = bbList_.first(); bbIter != bbList_.sentinel(); bbIter = bbIter->next())
    {
        // start a new node
        ofs << bbIter->value_->toString() << " [shape=box, label=\"\\" << endl;

        if (bbIter->value_->begin_ == 0)
            ofs << "\tENTRY\\n\\" << endl;
        else if (bbIter->value_->end_ == 0)
            ofs << "\tEXIT\\n\\" << endl;
        else
        {
            // for all instructions in this basic block except the last LabelInstr
            for (InstrList::Node* instrIter = bbIter->value_->begin_; instrIter != bbIter->value_->end_; instrIter = instrIter->next())
                ofs << '\t' << instrIter->value_->toString() << "\\n\\" << endl; // print instruction
        }
        // close this node
        ofs << "\"]" << endl << endl;
    }

    // iterate over all basic blocks in order to print connections
    for (BBList::Node* iter = bbList_.first(); iter != bbList_.sentinel(); iter = iter->next())
    {

// use this switch to print predecessors instead of successors
#if 0
        // for all predecessors
        for (BBSet::iterator pred = iter->value_->pred_.begin(); pred != iter->value_->pred_.end(); ++pred)
            ofs << iter->value_->toString() << " -> " << (*pred)->toString() << std::endl;
#else
        // for all successors
        for (BBSet::iterator succ = iter->value_->succ_.begin(); succ != iter->value_->succ_.end(); ++succ)
            ofs << iter->value_->toString() << " -> " << (*succ)->toString() << std::endl;
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

inline void FunctionTable::insert(PseudoReg* reg)
{
    swiftAssert(reg->regNr_ != PseudoReg::LITERAL, "reg is a LITERAL");

    pair<RegMap::iterator, bool> p
        = current_->vars_.insert( make_pair(reg->regNr_, reg) );

    swiftAssert(p.second, "there is already a reg with this regNr in the map");
}

PseudoReg* FunctionTable::newTemp(PseudoReg::RegType regType, int varNr)
{
    PseudoReg* reg = new PseudoReg(regType, current_->counter_, varNr);
    insert(reg);

    ++current_->counter_;

    return reg;
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

void FunctionTable::calcCFG()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->calcCFG();
}

void FunctionTable::calcDomTree()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->calcDomTree();
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

