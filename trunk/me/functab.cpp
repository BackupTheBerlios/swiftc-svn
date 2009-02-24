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

#include "me/functab.h"

#include <algorithm>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

#include "me/arch.h"
#include "me/struct.h"

using namespace std;

namespace me {

FuncTab* functab = 0;

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Function::Function(std::string* id, size_t stackPlaces)
    : id_(id)
    , ssaCounter_(0)
    , varCounter_(-1) // >= 0 is reserved for vars already in SSA form
    , cfg_(this)
    , firstLiveness_(false)
    , firstDefUse_(false)
    , lastLabelNode_( new InstrNode(new LabelInstr()) )
    , stackLayout_(stackPlaces)
{}

Function::~Function()
{
    // delete all instructions
    for (InstrNode* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
        delete iter->value_;

    // delete all pseudo vars
    for (VarMap::iterator iter = in_   .begin(); iter != in_   .end(); ++iter)
        delete iter->second;
    for (VarMap::iterator iter = inout_.begin(); iter != inout_.end(); ++iter)
        delete iter->second;
    for (VarMap::iterator iter = out_  .begin(); iter != out_  .end(); ++iter)
        delete iter->second;
    for (VarMap::iterator iter = vars_ .begin(); iter != vars_ .end(); ++iter)
        delete iter->second;

    for (size_t i = 0; i < consts_.size(); ++i)
        delete consts_[i];
    for (size_t i = 0; i < undefs_.size(); ++i)
        delete undefs_[i];

    delete id_;
}

/*
 * further methods
 */

inline void Function::insert(Var* var)
{
    pair<VarMap::iterator, bool> p
        = vars_.insert( make_pair(var->varNr_, var) );

    swiftAssert(p.second, "there is already a var with this varNr in the map");
}

#ifdef SWIFT_DEBUG

Reg* Function::newReg(Op::Type type, const std::string* id /*= 0*/)
{
    Reg* reg = new Reg(type, varCounter_--, id);
    insert(reg);

    return reg;
}

Reg* Function::newSSAReg(Op::Type type, const std::string* id /*= 0*/)
{
    Reg* reg = new Reg(type, ssaCounter_++, id);
    insert(reg);

    return reg;
}

Reg* Function::newSpilledSSAReg(Op::Type type, const std::string* id /*= 0*/)
{
    Reg* reg = new Reg(type, ssaCounter_++, id);
    reg->isSpilled_ = true;
    insert(reg);

    return reg;
}

MemVar* Function::newMemVar(Member* memory, const std::string* id /*= 0*/)
{
    MemVar* var = new MemVar(memory, varCounter_--, id);
    insert(var);

    return var;
}

MemVar* Function::newSSAMemVar(Member* memory, const std::string* id /*= 0*/)
{
    MemVar* var = new MemVar(memory, ssaCounter_++, id);
    insert(var);

    return var;
}

#else // SWIFT_DEBUG

#endif // SWIFT_DEBUG

Var* Function::cloneNewSSA(Var* var)
{
    Var* newVar = var->clone(ssaCounter_++);
    insert(newVar);

    return newVar;
}

Const* Function::newConst(Op::Type type)
{
    Const* _const = new Const(type);
    consts_.push_back(_const);

    return _const;
}

Undef* Function::newUndef(Op::Type type)
{
    Undef* undef = new Undef(type);
    undefs_.push_back(undef);

    return undef;
}

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
    // first destroy all ordinary Struct Members
    for (StructMap::iterator iter = structs_.begin(); iter != structs_.end(); ++iter)
    {
        Struct* _struct = iter->second;
        for (size_t i = 0; i < _struct->members_.size(); ++i)
        {
            if ( typeid(*_struct->members_[i]) != typeid(Struct) )
                delete _struct->members_[i];
        }
    }

    // now delete all structs
    for (StructMap::iterator iter = structs_.begin(); iter != structs_.end(); ++iter)
        delete iter->second;

    // finally delete all functions
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        delete iter->second;
}

/*
 * further methods
 */

Function* FunctionTable::insertFunction(string* id)
{
    currentFunction_ = new Function( id, me::arch->getNumStackPlaces() );
    functions_.insert( make_pair(id, currentFunction_) );

    return currentFunction_;
}

#ifdef SWIFT_DEBUG

Reg* FunctionTable::newReg(Op::Type type, const std::string* id /*= 0*/)
{
    return currentFunction_->newReg(type, id);
}

Reg* FunctionTable::newSSAReg(Op::Type type, const std::string* id /*= 0*/)
{
    return currentFunction_->newSSAReg(type, id);
}

Reg* FunctionTable::newSpilledSSAReg(Op::Type type, const std::string* id /*= 0*/)
{
    return currentFunction_->newSpilledSSAReg(type, id);
}

MemVar* FunctionTable::newMemVar(Member* memory, const std::string* id /*= 0*/)
{
    return currentFunction_->newMemVar(memory, id);
}

MemVar* FunctionTable::newSSAMemVar(Member* memory, const std::string* id /*= 0*/)
{
    return currentFunction_->newSSAMemVar(memory, id);
}

#else // SWIFT_DEBUG


#endif // SWIFT_DEBUG

Var* FunctionTable::cloneNewSSA(Var* var)
{
    return currentFunction_->cloneNewSSA(var);
}

Const* FunctionTable::newConst(Op::Type type)
{
    return currentFunction_->newConst(type);
}

Undef* FunctionTable::newUndef(Op::Type type)
{
    return currentFunction_->newUndef(type);
}

Var* FunctionTable::lookupVar(int id)
{
    VarMap::iterator varIter = currentFunction_->vars_.find(id);

    if ( varIter == currentFunction_->vars_.end() )
        return 0;
    else
        return varIter->second;
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

#else // SWIFT_DEBUG

Struct* FunctionTable::newStruct()
{
    Struct* _struct = new Struct();
    structs_[_struct->nr_] = _struct;

    return _struct;
}

#endif // SWIFT_DEBUG

void FunctionTable::appendMember(Member* member)
{
    structStack_.top()->append(member);
}

} // namespace me
