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
    for (InstrNode* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
        delete iter->value_;

    // delete all pseudo regs
    for (RegMap::iterator iter = in_   .begin(); iter != in_   .end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = inout_.begin(); iter != inout_.end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = out_  .begin(); iter != out_  .end(); ++iter)
        delete iter->second;
    for (RegMap::iterator iter = vars_ .begin(); iter != vars_ .end(); ++iter)
        delete iter->second;

    delete id_;
}

inline void Function::insert(Reg* reg)
{
    pair<RegMap::iterator, bool> p
        = vars_.insert( make_pair(reg->varNr_, reg) );

    swiftAssert(p.second, "there is already a reg with this varNr in the map");
}

Reg* Function::newSSA(Op::Type type)
{
    Reg* reg = new Reg(type, regCounter_);
    insert(reg);

    ++regCounter_;

    return reg;
}

Reg* Function::newVar(Op::Type type, int varNr, std::string* id)
{
    swiftAssert(varNr < 0, "varNr must be less than zero");
    Reg* reg = new Reg(type, varNr, id);
    insert(reg);

    return reg;
}

#ifdef SWIFT_DEBUG

Reg* Function::newSSA(Op::Type type, std::string* id)
{
    Reg* reg = new Reg(type, regCounter_, id);
    insert(reg);

    ++regCounter_;

    return reg;
}

Reg* Function::newVar(Op::Type type, int varNr, std::string* id)
{
    swiftAssert(varNr < 0, "varNr must be less than zero");
    Reg* reg = new Reg(type, varNr, id);
    insert(reg);

    return reg;
}

#else // SWIFT_DEBUG

Reg* Function::newVar(Op::Type type, int varNr)
{
    swiftAssert(varNr < 0, "varNr must be less than zero");
    Reg* reg = new Reg(type, varNr);
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
    for (InstrNode* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
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

Reg* FunctionTable::newSSA(Op::Type type)
{
    return current_->newSSA(type);
}

#ifdef SWIFT_DEBUG

Reg* FunctionTable::newSSA(Op::Type type, std::string* id)
{
    return current_->newSSA(type, id);
}

Reg* FunctionTable::newVar(Op::Type type, int varNr, std::string* id)
{
    return current_->newVar(type, varNr, id);
}

#else // SWIFT_DEBUG

Reg* FunctionTable::newVar(Op::Type type, int varNr)
{
    return current_->newVar(type, varNr);
}

#endif // SWIFT_DEBUG

Reg* FunctionTable::lookupReg(int varNr)
{
    RegMap::iterator regIter = current_->vars_.find(varNr);

    if ( regIter == current_->vars_.end() )
        return 0;
    else
        return regIter->second;
}

void FunctionTable::appendInstr(InstrBase* instr)
{
    current_->instrList_.append(instr);
}

void FunctionTable::appendInstrNode(InstrNode* node)
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
