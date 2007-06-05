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

void Function::findBasicBlocks()
{
    // append and prepend artificially a stating and ending label
    instrList_.append( new LabelInstr() );
    instrList_.prepend( new LabelInstr() );

    LabelInstr* end = 0;
    LabelInstr* begin = 0;

    // iterate backwards through the instruction list
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        if ( typeid(*iter->value_) == typeid(LabelInstr) )
        {
            begin = end;
            end = (LabelInstr*) iter->value_;

            if (begin)
            {
                // we have found the next basic block
                bbList_.prepend( new BasicBlock(begin, end) );
            }
        }
    }

    std::cout << *id_ << std::endl;
    for (BBList::Node* iter = bbList_.first(); iter != bbList_.sentinel(); iter = iter->next())
        std::cout << '\t' << iter->value_->toString() << std::endl;;
}

void Function::dump(ofstream& ofs)
{
    ofs << std::endl << *id_ << ":" << std::endl;
    // for all instructions in this function
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        // don't print a tab character if this is a label
        if ( typeid(*iter->value_) != typeid(LabelInstr) )
            ofs << '\t';
        ofs << iter->value_->toString() << std::endl;
    }
}

// -----------------------------------------------------------------------------

FunctionTable::~FunctionTable()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        delete iter->second;
}

Function* FunctionTable::insertFunction(std::string* id)
{
    current_ = new Function(id);
    functions_.insert( std::make_pair(id, current_) );

    return current_;
}

inline void FunctionTable::insert(PseudoReg* reg)
{
    swiftAssert(reg->regNr_ != PseudoReg::LITERAL, "reg is a LITERAL");

    pair<RegMap::iterator, bool> p
        = current_->vars_.insert( std::make_pair(reg->regNr_, reg) );

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

void FunctionTable::findBasicBlocks()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->findBasicBlocks();
}

void FunctionTable::dump(const std::string& extension)
{
    ostringstream oss;
    oss << filename_ << extension;

    ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...
    ofs << filename_ << extension << ":" << std::endl << std::endl;

    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->dump(ofs);

    // finish
    ofs.close();
}
