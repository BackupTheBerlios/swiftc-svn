#include "me/functab.h"

#include <algorithm>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

using namespace std;

namespace me {

FuncTab* functab = 0;

// -----------------------------------------------------------------------------

Function::~Function()
{
    // delete all instructions
    for (InstrNode iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
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

    delete id_;
}

inline void Function::insert(PseudoReg* reg)
{
    swiftAssert(reg->regNr_ != PseudoReg::LITERAL, "reg is a LITERAL");

    pair<RegMap::iterator, bool> p
        = vars_.insert( make_pair(reg->regNr_, reg) );

    swiftAssert(p.second, "there is already a reg with this regNr in the map");
}

PseudoReg* Function::newTemp(PseudoReg::RegType regType)
{
    PseudoReg* reg = new PseudoReg(regType, regCounter_);
    insert(reg);

    ++regCounter_;

    return reg;
}

#ifdef SWIFT_DEBUG

PseudoReg* Function::newTemp(PseudoReg::RegType regType, std::string* id)
{
    PseudoReg* reg = new PseudoReg(regType, regCounter_, id);
    insert(reg);

    ++regCounter_;

    return reg;
}

PseudoReg* Function::newVar(PseudoReg::RegType regType, int varNr, std::string* id)
{
    swiftAssert(varNr < 0, "varNr must be less than zero");
    PseudoReg* reg = new PseudoReg(regType, varNr, id);
    insert(reg);

    return reg;
}

#else // SWIFT_DEBUG

PseudoReg* Function::newVar(PseudoReg::RegType regType, int varNr)
{
    swiftAssert(varNr < 0, "varNr must be less than zero");
    PseudoReg* reg = new PseudoReg(regType, varNr);
    insert(reg);

    return reg;
}

#endif // SWIFT_DEBUG

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
    for (InstrNode iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        // don't print a tab character if this is a label
        if ( typeid(*iter->value_) != typeid(LabelInstr) )
            ofs << '\t';
        ofs << iter->value_->toString() << endl;
    }

    // print idoms
    ofs << endl << "IDOMS:" << endl;
    ofs << cfg_.dumpIdoms() << endl;

    // print domChildren
    ofs << endl << "DOM CHILDREN:" << endl;
    ofs << cfg_.dumpDomChildren() << endl;

    // print dominance frontier
    ofs << endl << "DOMINANCE FRONTIER:" << endl;
    ofs << cfg_.dumpDomFrontier() << endl;
}

void Function::dumpDot(const string& baseFilename)
{
    cfg_.dumpDot(baseFilename);
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

PseudoReg* FunctionTable::newTemp(PseudoReg::RegType regType)
{
    return current_->newTemp(regType);
}

#ifdef SWIFT_DEBUG

PseudoReg* FunctionTable::newTemp(PseudoReg::RegType regType, std::string* id)
{
    return current_->newTemp(regType, id);
}

PseudoReg* FunctionTable::newVar(PseudoReg::RegType regType, int varNr, std::string* id)
{
    return current_->newVar(regType, varNr, id);
}

#else // SWIFT_DEBUG

PseudoReg* FunctionTable::newVar(PseudoReg::RegType regType, int varNr)
{
    return current_->newVar(regType, varNr);
}

#endif // SWIFT_DEBUG

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

void FunctionTable::appendInstrNode(InstrNode node)
{
    current_->instrList_.append(node);
}

void FunctionTable::buildUpME()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
    {
        CFG& cfg = iter->second->cfg_;

        cfg.calcCFG();
        cfg.calcDomTree();
        cfg.calcDomFrontier();
        cfg.placePhiFunctions();
        cfg.renameVars();
        cfg.calcDef();
        cfg.calcUse();
    }
}

void FunctionTable::dumpSSA()
{
    ostringstream oss;
    oss << filename_ << ".ssa";

    ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...
    ofs << oss.str() << ':' << endl;

    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->dumpSSA(ofs);

    // finish
    ofs.close();
}

void FunctionTable::dumpDot()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->dumpDot(filename_ + '.' + *iter->second->id_);
}

} // namespace me
