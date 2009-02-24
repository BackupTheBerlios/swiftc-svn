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
#include "fe/method.h"
#include "fe/var.h"
#include "fe/syntaxtree.h"
#include "fe/type.h"

using namespace std;

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
    method_ = 0;
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
    pair<Module::ClassMap::iterator, bool> p
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
    pair<Class::MemberVarMap::iterator, bool> p
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

void SymbolTable::insert(Method* method)
{
    Class::MethodMap::iterator iter
        = class_->methods_.insert( std::make_pair(method->id_, method) );

    // set current method scope
    method_ = method;

    // set current signature scope
    sig_ = &method->sig_;
}

bool SymbolTable::insert(Param* param)
{
    PARAMS_EACH(iter, sig_->params_)
    {
        if (*param->id_ == *iter->value_->id_)
        {
            errorf(param->line_, "there is already a parameter '%s' defined in this procedure",
                param->id_->c_str());

            return false;
        }
    }

    sig_->params_.append(param);

    return true;
}

bool SymbolTable::insert(Local* local)
{
    pair<Scope::LocalMap::iterator, bool> p
        = currentScope()->locals_.insert( std::make_pair(local->id_, local) );

    if ( !p.second )
    {
        errorf(local->line_, "there is already a local '%s' defined in this scope in line %i",
            local->id_->c_str(), p.first->second->line_);

        return false;
    }

    PARAMS_EACH(iter, sig_->params_)
    {
        if (*local->id_ == *iter->value_->id_)
        {
            errorf(local->line_, "local '%s' shadows a parameter", local->id_->c_str());
            return false;
        }
    }

    return true;
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

void SymbolTable::enterMethod(Method* method)
{
    method_ = method;
    sig_ = &method->sig_;

    scopeStack_.push(method_->rootScope_);
}

void SymbolTable::leaveMethod()
{
    method_ = 0;
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
    currentScope()->childScopes_.append(newScope);
    enterScope(newScope);

    return newScope;
}

/*
 * lookup methods
 */

Var* SymbolTable::lookupVar(const string* id)
{
    // is it a local?
    Local* local = currentScope()->lookupLocal(id);
    if (local)
        return local;

    // no - perhaps a parameter?
    return sig_->findParam(id); // will return 0, if not found
}

Class* SymbolTable::lookupClass(const string* id)
{
    // currently only one module - the default module - is supported.
    Module::ClassMap::const_iterator iter = rootModule_->classes_.find(id);
    if ( iter != rootModule_->classes_.end() )
        return iter->second;

    // class not found -- so return NULL
    return 0;
}

Method* SymbolTable::lookupMethod(const std::string* classId,
                                  const std::string* methodId,
                                  int methodQualifier,
                                  const Sig& sig,
                                  int line,
                                  SigCheckingStyle sigCheckingStyle)
{
    // lookup class
    Class* _class = symtab->lookupClass(classId);

    // lookup method
    Class::MethodMap::const_iterator iter = _class->methods_.find(methodId);
    if (iter == _class->methods_.end())
    {
        errorf(line, "there is no method %s defined in class %s",
            methodId->c_str(), classId->c_str());

        return 0;
    }

    // get iterator to the first method, which has not methodId as identifier
    Class::MethodMap::const_iterator last = _class->methods_.upper_bound(methodId);

    // current method in loop below
    Method* method = 0;

    for (; iter != last; ++iter)
    {
        method = iter->second;

        bool sigCheck;

        switch (sigCheckingStyle)
        {
            case CHECK_JUST_INGOING:
                sigCheck = method->sig_.checkIngoing(sig);
                break;
            case CHECK_ALL:
                sigCheck = Sig::check(method->sig_, sig);
                break;
        }

        if (sigCheck)
            break;
        else
            method = 0; // mark as not found
    }

    return method;
}

/*
    further methods
*/

Scope* SymbolTable::currentScope()
{
    Scope* s = scopeStack_.top();
    return s;
}

Local* SymbolTable::createNewLocal(const Type* type, std::string* id, int line /*= NO_LINE*/)
{
    Local* local = new Local(type->clone(), type->baseType_->createVar(id), id, line);
    symtab->insert(local);

    return local;
}

} // namespace swift
