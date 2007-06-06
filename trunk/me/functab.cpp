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

    // append and prepend artificially two starting and one ending label
//     LabelInstr* entryLabel = new LabelInstr();
    LabelInstr* exitLabel  = new LabelInstr();
    instrList_.append(exitLabel);           // this will become the EXIT block
//     instrList_.prepend(entryLabel);         // this will become the ENTRY block

    LabelInstr* end = 0;
    LabelInstr* begin = 0;
    BasicBlock* lastBB = 0;

    // iterate over the instruction list and find basic blocks
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        if ( typeid(*iter->value_) == typeid(LabelInstr) )
        {
            begin = end;
            end = (LabelInstr*) iter->value_;

            if (begin) // we have found the next basic block
            {
                BasicBlock* bb = new BasicBlock(begin, end);
                label2BB_[begin] = bb; // keep acount of the label and the basic block in the map
                bbList_.append( bb );

                // connect new found basic block with the last one if it exists
                if (lastBB)
                {
                    lastBB->succ_.append(bb);   // append successor
                    bb->pred_.append(lastBB);   // append predecessor
                }

                lastBB = bb;                // now this is the last basic block
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

    /*
        now we know of all starting labels and their associated basic block
        so let us calculate the CFG
    */

    BasicBlock* currentBB = 0;

    // iterate once again over the instruction list
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        if ( typeid(*iter->value_) == typeid(LabelInstr) )
            currentBB = label2BB_[ ((LabelInstr*) iter->value_) ]; // we have found a new basic block

        if ( typeid(*iter->value_) == typeid(GotoInstr) )
        {
            BasicBlock* succ = label2BB_[ ((GotoInstr*) iter->value_)->label_ ];
            currentBB->succ_.append(succ);  // append successor
            succ->pred_.append(succ);       // append predecessor
        }
        else if ( typeid(*iter->value_) == typeid(BranchInstr) )
        {
            /*
                do not connect the current basic block with the one of the
                trueLabel since they were already connected in the first pass
            */
            BasicBlock* succ = label2BB_[ ((BranchInstr*) iter->value_)->falseLabel_ ];
            currentBB->succ_.append(succ); // append successor
            succ->pred_.append(currentBB); // append predecessor
        }
    }

    /*
        Create the two additional blocks ENTRY and EXIT.
    */
//     BasicBlock* entry = label2BB_[entryLabel];
//     BasicBlock* exit  = label2BB_[exitLabel];

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

    // now print the basic blocks of this function
    ofs << "---" << endl;

    for (BBList::Node* iter = bbList_.first(); iter != bbList_.sentinel(); iter = iter->next())
        ofs << '\t' << iter->value_->toString() << endl;
}

void Function::dumpDot(const string& baseFilename)
{
    ostringstream oss;
    oss << baseFilename << '.' << *id_ << ".dot";

    ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...

    // prepare graphviz dot file
    ofs << "digraph " << *id_ << " {" << endl;

    // for all instructions in this function
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        // start a new node if this is a label
        if ( typeid(*iter->value_) == typeid(LabelInstr) )
        {
            // close this node when this is not the first instruction
            if ( iter != instrList_.first() )
                ofs << "\t\"]" << endl << endl;

            ofs << '\t' << iter->value_->toString() << " [shape=box, label=\"\\" << endl;
        }

        ofs << "\t\t" << iter->value_->toString() << "\\n\\" << endl;
    }

    // close last node
    ofs << "\t\"]" << endl << endl;

    // iterate over all basic blocks
    for (BBList::Node* iter = bbList_.first(); iter != bbList_.sentinel(); iter = iter->next())
        iter->value_->toDot(ofs); // print connections

    // end graphviz dot file
    ofs << '}' << endl;
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

PseudoReg* FunctionTable::newTemp(PseudoReg::RegType regType)
{
    PseudoReg* reg = new PseudoReg(current_->counter_, regType);
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

void FunctionTable::calcCFG()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->calcCFG();
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
