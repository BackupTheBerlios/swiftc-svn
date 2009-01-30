#include "me/functab.h"

#include <algorithm>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

#include "me/struct.h"

using namespace std;

namespace me {

FuncTab* functab = 0;

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Function::Function(std::string* id)
    : id_(id)
    , regCounter_(0)
    , varCounter_(-1) // >= 0 is reserved for vars already in SSA form
    , cfg_(this)
    , firstLiveness_(false)
    , firstDefUse_(false)
    , lastLabelNode_( new InstrNode(new LabelInstr()) )
    , spillSlots_(-1)
{}

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

/*
 * further methods
 */

inline void Function::insert(Reg* reg)
{
    pair<RegMap::iterator, bool> p
        = vars_.insert( make_pair(reg->varNr_, reg) );

    swiftAssert(p.second, "there is already a reg with this varNr in the map");
}

#ifdef SWIFT_DEBUG

Reg* Function::newSSA(Op::Type type, std::string* id /*= 0*/)
{
    Reg* reg = new Reg(type, regCounter_++, id);
    insert(reg);

    return reg;
}

Reg* Function::newVar(Op::Type type, std::string* id)
{
    Reg* reg = new Reg(type, varCounter_--, id);
    insert(reg);

    return reg;
}

Reg* Function::newMemSSA(Op::Type type, std::string* id /*= 0*/)
{
    Reg* reg = new Reg(type, regCounter_++, id);
    reg->isMem_ = true;
    insert(reg);

    return reg;
}

#else // SWIFT_DEBUG

Reg* Function::newSSA(Op::Type type)
{
    Reg* reg = new Reg(type, regCounter_++);
    insert(reg);

    return reg;
}

Reg* Function::newVar(Op::Type type)
{
    Reg* reg = new Reg(type, varCounter_--);
    insert(reg);

    return reg;
}

Reg* Function::newMemSSA(Op::Type type)
{
    Reg* reg = new Reg(type, regCounter_++);
    reg->isMem_ = true;
    insert(reg);

    return reg;
}

#endif // SWIFT_DEBUG

/*
 * dump methods
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

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

FunctionTable::FunctionTable(const std::string& filename)
    : filename_(filename)
{}

FunctionTable::~FunctionTable()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        delete iter->second;

    for (StructMap::iterator iter = structs_.begin(); iter != structs_.end(); ++iter)
        delete iter->second;
}

/*
 * further methods
 */

Function* FunctionTable::insertFunction(string* id)
{
    currentFunction_ = new Function(id);
    functions_.insert( make_pair(id, currentFunction_) );

    return currentFunction_;
}

#ifdef SWIFT_DEBUG

Reg* FunctionTable::newSSA(Op::Type type, std::string* id /*= 0*/)
{
    return currentFunction_->newSSA(type, id);
}

Reg* FunctionTable::newVar(Op::Type type, std::string* id)
{
    return currentFunction_->newVar(type, id);
}

Reg* FunctionTable::newMemSSA(Op::Type type, std::string* id /*= 0*/)
{
    return currentFunction_->newMemSSA(type, id);
}

#else // SWIFT_DEBUG

Reg* FunctionTable::newSSA(Op::Type type)
{
    return currentFunction_->newSSA(type);
}

Reg* FunctionTable::newVar(Op::Type type)
{
    return currentFunction_->newVar(type);
}

Reg* FunctionTable::newMemSSA(Op::Type type)
{
    return currentFunction_->newMemSSA(type);
}

#endif // SWIFT_DEBUG

Reg* FunctionTable::lookupReg(int id)
{
    RegMap::iterator regIter = currentFunction_->vars_.find(id);

    if ( regIter == currentFunction_->vars_.end() )
        return 0;
    else
        return regIter->second;
}

void FunctionTable::appendInstr(InstrBase* instr)
{
    currentFunction_->instrList_.append(instr);
}

void FunctionTable::appendInstrNode(InstrNode* node)
{
    currentFunction_->instrList_.append(node);
}

void FunctionTable::buildUpME()
{
    for (StructMap::iterator iter = structs_.begin(); iter != structs_.end(); ++iter)
        iter->second->analyze();

    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
    {
        CFG& cfg = iter->second->cfg_;
        cfg.constructSSAForm();
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

InstrNode* FunctionTable::getLastLabelNode()
{
    return currentFunction_->lastLabelNode_;
}

/*
 * struct handling
 */

void FunctionTable::enterStruct(Struct* _struct)
{
    structStack_.push(_struct);
}

void FunctionTable::leaveStruct()
{
    structStack_.pop();
}

Struct* FunctionTable::currentStruct()
{
    return structStack_.top();
}

#ifdef SWIFT_DEBUG

Struct* FunctionTable::newStruct(const std::string& id)
{
    Struct* _struct = new Struct(id);
    structs_[_struct->nr_] = _struct;

    return _struct;
}

Member* FunctionTable::appendMember(Op::Type type, const std::string& id)
{
    return structStack_.top()->append(type,id);
}

Member* FunctionTable::appendMember(Struct* _struct, const std::string& id)
{
    return structStack_.top()->append(_struct, id);
}

#else // SWIFT_DEBUG

Struct* FunctionTable::newStruct()
{
    Struct* _struct = new Struct();
    structs_[_struct->nr_] = _struct;

    return str;
}

Member* FunctionTable::appendMember(Op::Type type)
{
    return structStack_.top()->append(type);
}

Member* FunctionTable::appendMember(Struct* _struct)
{
    return structStack_.top()->append(_struct);
}

#endif // SWIFT_DEBUG

} // namespace me
