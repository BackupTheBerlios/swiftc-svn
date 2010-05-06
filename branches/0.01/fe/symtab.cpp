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

#include "symtab.h"

#include <iostream>
#include <sstream>
#include <algorithm>

#include "utils/assert.h"

#include "fe/class.h"
#include "fe/error.h"
#include "fe/memberfunction.h"
#include "fe/var.h"
#include "fe/scope.h"
#include "fe/signature.h"
#include "fe/syntaxtree.h"
#include "fe/type.h"

namespace swift {

/*
 * TODO remove all error handling here
 */

// init global
SymTab* symtab = 0;

/*
 * constructor and init stuff
 */

SymbolTable::SymbolTable()
{
    reset();
}

void SymbolTable::reset()
{
//     module_ = 0; TODO
    class_  = 0;
    memberFunction_ = 0;
    sig_ = 0;
}

/*
 * insert methods
 */

bool SymbolTable::insert(Module* module)
{
    swiftAssert(!module_, "There is already a module");

    rootModule_ = module;
    module_ = rootModule_;

    return true;
}

bool SymbolTable::insert(Class* _class)
{
    std::pair<Module::ClassMap::iterator, bool> p
        = module_->classes_.insert( std::make_pair(_class->id_, _class) );

    if ( !p.second )
    {
        // insertion was not successfull

        // give proper error
        errorf(_class->line_, "there there is already a class '%s' defined in line %i",
            p.first->second->getFullName().c_str(), p.first->second->line_);

        return false;
    }

    // set current class scope
    class_ = p.first->second;

    return true;
}

bool SymbolTable::insert(MemberVar* memberVar)
{
    std::pair<Class::MemberVarMap::iterator, bool> p
        = class_->memberVars_.insert( std::make_pair(memberVar->id_, memberVar) );

    if ( !p.second )
    {
        // insertion was not successfull

        errorf(memberVar->line_, "there is already a member '%s' defined in '%s' line %i",
            p.first->second->id_, memberVar->getFullName().c_str(), p.first->second->line_);

        return false;
    }

    return true;
}

void SymbolTable::insert(MemberFunction* memberFunction)
{
    Class::MemberFunctionMap::iterator iter
        = class_->memberFunctions_.insert( std::make_pair(memberFunction->id_, memberFunction) );

    // set current method scope
    memberFunction_ = memberFunction;

    // set current signature scope
    sig_ = memberFunction_->sig_;

    /*
     * build a function name for the me::functab consisting of the class name,
     * the method name and a counted number to prevent name clashes
     * due to overloading
     */
    static int counter = 0;
    std::ostringstream oss;

    oss << *class_->id_ << '$' << *memberFunction->id_ << '$' << counter++;
    sig_->setMeId( oss.str() );
}

void SymbolTable::insertInParam(InParam* param)
{
    if ( sig_->findParam(param->id_) )
    {
        errorf(param->line_, 
                "there is already a parameter '%s' defined in this procedure",
                param->id_->c_str());
        return;
    }

    sig_->appendInParam(param);
}

void SymbolTable::insertOutParam(OutParam* param)
{
    Param* found = sig_->findParam(param->id_);
    if (found) 
    {
        if ( typeid(*found) == typeid(OutParam) )
        {
            errorf(param->line_, 
                    "there is already a return value '%s' defined for this procedure",
                    param->id_->c_str());
        }
        else
        {
            errorf(param->line_, 
                    "there is already a parameter '%s' defined for this procedure", 
                    param->id_->c_str());
        }

        return;
    }

    sig_->appendOutParam(param);

    return;
}

bool SymbolTable::insert(Local* local)
{
    return currentScope()->insert(local, sig_);
}

/*
 * enter and leave methods
 */

void SymbolTable::enterModule()
{
    module_ = rootModule_;
}

void SymbolTable::leaveModule()
{
    module_ = 0;
}

void SymbolTable::enterClass(Class* _class)
{
    class_ = _class;
}

void SymbolTable::leaveClass()
{
    class_ = 0;
}

void SymbolTable::enterMemberFunction(MemberFunction* memberFunction)
{
    memberFunction_ = memberFunction;
    sig_ = memberFunction->sig_;

    scopeStack_.push(memberFunction->rootScope_);
}

void SymbolTable::leaveMemberFunction()
{
    memberFunction_ = 0;
    sig_ = 0;
    scopeStack_.pop();

    swiftAssert(scopeStack_.empty(), "stack must be empty here");
}

void SymbolTable::enterScope(Scope* scope)
{
    scopeStack_.push(scope);
}

void SymbolTable::leaveScope()
{
    scopeStack_.pop();
}

Scope* SymbolTable::createAndEnterNewScope()
{
    Scope* newScope = new Scope( currentScope() );
    currentScope()->childScopes_.push_back(newScope);
    enterScope(newScope);

    return newScope;
}

/*
 * lookup methods
 */

Var* SymbolTable::lookupVar(const std::string* id)
{
    // is it a local?
    Local* local = currentScope()->lookupLocal(id);
    if (local)
        return local;

    // no - perhaps a parameter?
    return sig_->findParam(id); // will return 0, if not found
}

Class* SymbolTable::lookupClass(const std::string* id)
{
    // currently only one module - the default module - is supported.
    Module::ClassMap::const_iterator iter = rootModule_->classes_.find(id);
    if ( iter != rootModule_->classes_.end() )
        return iter->second;

    // class not found -- so return NULL
    return 0;
}

MemberFunction* SymbolTable::lookupMemberFunction(Class* _class,
                                          const std::string* id,
                                          const TypeList& in,
                                          int line)
{
    // lookup method
    Class::MemberFunctionMap::const_iterator iter = 
        _class->memberFunctions_.find(id);

    if (iter == _class->memberFunctions_.end())
    {
        if (line)
        {
            errorf( line, "there is no method named '%s' in class '%s'",
                id->c_str(), _class->id_->c_str() );
        }

        return 0;
    }

    // get iterator to the first method, which has not methodId as identifier
    Class::MemberFunctionMap::const_iterator last 
        = _class->memberFunctions_.upper_bound(id);

    // current method in loop below
    MemberFunction* memberFunction = 0;
    for (; iter != last; ++iter)
    {
        memberFunction = iter->second;

        if ( memberFunction->sig_->checkIn(in) )
            break;
        else
            memberFunction = 0; // mark as not found
    }

    if (!memberFunction)
    {
        if (line)
        {
            errorf(line, "there is no member function '%s(%s)' defined in class '%s'",
                    //memberFunction->qualifierString().c_str(), 
                    id->c_str(), 
                    in.toString().c_str(), 
                    _class->id_->c_str());
        }

        return 0;
    }

    return memberFunction;
}

Create* SymbolTable::lookupCreate(Class* _class, const TypeList& in, int line)
{
    std::string create("create");
    return (Create*) lookupMemberFunction(_class, &create, in, line);
}

Assign* SymbolTable::lookupAssign(Class* _class, const TypeList& in, int line)
{
    std::string op("assign");
    return (Assign*) lookupMemberFunction(_class, &op, in, line);
}

Method* SymbolTable::lookupAssignCreate(Class* _class, 
                                        const TypeList& in, 
                                        bool create, 
                                        int line)
{
    if (create)
        return lookupCreate(_class, in, line);
    else
        return lookupAssign(_class, in, line);
}

/*
 * current getters
 */

Scope* SymbolTable::currentScope()
{
    return scopeStack_.top();
}

Class* SymbolTable::currentClass()
{
    return class_;
}

MemberFunction* SymbolTable::currentMemberFunction()
{
    return memberFunction_;
}

/*
 * further methods
 */

std::pair<Local*, bool> SymbolTable::createNewLocal(const Type* type, 
                                                    std::string* id, 
                                                    int line /*= NO_LINE*/)
{
    Local* local = new Local(type->clone(), type->createVar(id), id, line);
    bool b = symtab->insert(local);

    return std::make_pair(local, b);
}

} // namespace swift
