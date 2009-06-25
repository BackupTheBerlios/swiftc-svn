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
#include "me/cfg.h"
#include "me/struct.h"
#include "me/stacklayout.h"

using namespace std;

namespace me {

FuncTab* functab = 0;

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Function::Function(std::string* id, size_t stackPlaces, bool ignore)
    : id_(id)
    , ssaCounter_(0)
    , varCounter_(-1) // >= 0 is reserved for vars already in SSA form
    , cfg_( new CFG(this) )
    , firstLiveness_(false)
    , firstDefUse_(false)
    , lastLabelNode_( new InstrNode(new LabelInstr()) )
    , stackLayout_( new StackLayout(stackPlaces) )
    , ignore_(ignore)
    , isMain_(false)
{}

Function::~Function()
{
    // delete all instructions
    for (InstrNode* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
        delete iter->value_;

    for (VarMap::iterator iter = vars_.begin(); iter != vars_.end(); ++iter)
        delete iter->second;

    for (size_t i = 0; i < consts_.size(); ++i)
        delete consts_[i];
    for (size_t i = 0; i < undefs_.size(); ++i)
        delete undefs_[i];

    delete id_;
    delete stackLayout_;
    delete cfg_;
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

Reg* Function::newReg(Op::Type type)
{
    Reg* reg = new Reg(type, varCounter_--);
    insert(reg);

    return reg;
}

Reg* Function::newSSAReg(Op::Type type)
{
    Reg* reg = new Reg(type, ssaCounter_++);
    insert(reg);

    return reg;
}

Reg* Function::newSpilledSSAReg(Op::Type type)
{
    Reg* reg = new Reg(type, ssaCounter_++);
    reg->isSpilled_ = true;
    insert(reg);

    return reg;
}

MemVar* Function::newMemVar(Member* memory)
{
    MemVar* var = new MemVar(memory, varCounter_--);
    insert(var);

    return var;
}

MemVar* Function::newSSAMemVar(Member* memory)
{
    MemVar* var = new MemVar(memory, ssaCounter_++);
    insert(var);

    return var;
}


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

bool Function::ignore() const
{
    return ignore_;
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

#if 0
    // print idoms
    ofs << endl << "IDOMS:" << endl;
    ofs << cfg_->dumpIdoms() << endl;

    // print domChildren
    ofs << endl << "DOM CHILDREN:" << endl;
    ofs << cfg_->dumpDomChildren() << endl;

    // print dominance frontier
    ofs << endl << "DOMINANCE FRONTIER:" << endl;
    ofs << cfg_->dumpDomFrontier() << endl;
#endif
}

void Function::dumpDot(const string& baseFilename)
{
    cfg_->dumpDot(baseFilename);
}

void Function::appendArg(Var* arg)
{
    arg_.push_back(arg);
}

void Function::appendRes(Var* res)
{
    res_.push_back(res);
}

void Function::buildFunctionEntry()
{
    if ( !arg_.empty() )
    {
        SetParams* setParams = new SetParams( arg_.size() );

        // for each ingoing param
        for (size_t i = 0; i < arg_.size(); ++i) 
        {
            Var* var = arg_[i];
            setParams->res_[i] = Res(var);
        }

        functab->appendInstr(setParams);
    }

    if ( !res_.empty() )
    {
        // this one will init the return values of the function
        AssignInstr* returnValues = new AssignInstr('=');
        returnValues->arg_.push_back( Arg(newUndef(Op::R_INT32)) ); // simply use a dummy type

        // for each result
        for (size_t i = 0; i < res_.size(); ++i) 
        {
            Var* var = res_[i];
            returnValues->res_.push_back( Res(var, var->varNr_) );
        }

        functab->appendInstr(returnValues);
    }
}

bool Function::isMain() const
{
    return isMain_;
}

void Function::setAsMain()
{
    swiftAssert(!mainSet_, "already one main function set");
    isMain_ = true;
}

void Function::buildFunctionExit()
{
    // is there at least one result?
    if ( res_.empty() > 0 )
        return;

    SetResults* setResults = new SetResults( res_.size() );
    
    for (size_t i = 0; i < res_.size(); ++i)
    {
        Var* var = res_[i];
        setResults->arg_[i] = Arg(var);
    }

    functab->appendInstr(setResults);
}

std::string Function::getId() const
{
    return *id_;
}

//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

/*
 * init statics
 */

bool Function::mainSet_ = false;

#endif // SWIFT_DEBUG

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
        iter->second->destroyNonStructMembers();

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

Function* FunctionTable::insertFunction(string* id, bool ignore)
{
    currentFunction_ = new Function( id, arch->getNumStackPlaces(), ignore );
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

Reg* FunctionTable::newReg(Op::Type type)
{
    return currentFunction_->newReg(type);
}

Reg* FunctionTable::newSSAReg(Op::Type type)
{
    return currentFunction_->newSSAReg(type);
}

Reg* FunctionTable::newSpilledSSAReg(Op::Type type)
{
    return currentFunction_->newSpilledSSAReg(type);
}

MemVar* FunctionTable::newMemVar(Member* memory)
{
    return currentFunction_->newMemVar(memory);
}

MemVar* FunctionTable::newSSAMemVar(Member* memory)
{
    return currentFunction_->newSSAMemVar(memory);
}

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

void FunctionTable::analyzeStructs()
{
    for (StructMap::iterator iter = structs_.begin(); iter != structs_.end(); ++iter)
        iter->second->analyze();
}

void FunctionTable::buildUpME()
{
    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
    {
        CFG* cfg = iter->second->cfg_;
        cfg->constructSSAForm();
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

void FunctionTable::appendArg(Var* arg)
{
    currentFunction_->arg_.push_back(arg);
}

void FunctionTable::appendRes(Var* res)
{
    currentFunction_->res_.push_back(res);
}

void FunctionTable::buildFunctionEntry()
{
    currentFunction_->buildFunctionEntry();
}

void FunctionTable::buildFunctionExit()
{
    currentFunction_->buildFunctionExit();
}

std::string FunctionTable::getId() const
{
    return *currentFunction_->id_;
}

void FunctionTable::setAsMain()
{
    currentFunction_->setAsMain();
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
    structs_[ _struct->getNr() ] = _struct;

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
