#include "symboltable.h"

#include <iostream>
#include <sstream>
#include <algorithm>

#include "utils/assert.h"

#include "error.h"
#include "syntaxtree.h"
#include "class.h"

using namespace std;

SymTab symtab;

// -----------------------------------------------------------------------------

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

        // build full class name with module names: module1.module2.Class1.Class2 etc
        stack<string> idStack;
        idStack.push(*_class->id_);

        for (Node* iter = class_->parent_; iter != 0; iter = iter->parent_)
            idStack.push( iter->toString() );

        ostringstream oss;

        while ( !idStack.empty() )
        {
            oss << idStack.top();
            idStack.pop();

            if ( !idStack.empty() )
                oss << '.';
        }

        // give proper error
        errorf(_class->line_, "there there is already a class '%s' defined in line %i", oss.str().c_str(), p.first->second->line_);

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

        // build full class name with module names: module1.module2.Class1.Class2 etc
        stack<string> idStack;

        for (Node* iter = class_->parent_; iter != 0; iter = iter->parent_)
            idStack.push( iter->toString() );

        ostringstream oss;

        while ( !idStack.empty() )
        {
            oss << idStack.top();
            idStack.pop();

            if ( !idStack.empty() )
                oss << '.';
        }

        errorf(memberVar->line_, "there is already a member '%s' defined in '%s' line %i", memberVar->id_->c_str(), oss.str().c_str(), p.first->second->line_);

        return false;
    }

    return true;
}

bool SymbolTable::insert(Method* method)
{
    pair<Class::MethodMap::iterator, bool> p
        = class_->methods_.insert( std::make_pair(method->id_, method) );

    if ( !p.second )
    {
        stack<string> idStack;

        for (Node* iter = method_->parent_; iter != 0; iter = iter->parent_)
            idStack.push( iter->toString() );

        ostringstream oss;

        while ( !idStack.empty() )
        {
            oss << idStack.top();
            idStack.pop();

            if ( !idStack.empty() )
                oss << '.';
        }

        errorf(method->line_, "there is already a method '%s' defined in '%s' line %i", method_->toString().c_str(), oss.str().c_str(), p.first->second->line_);

        return false;
    }

    // set current method scope
    method_ = p.first->second;

    return true;
}

bool SymbolTable::insert(Parameter* parameter)
{
    for (size_t i = 0; i < method_->params_.size(); ++i)
    {
        if (*parameter->id_ == *method_->params_[i]->id_)
        {
            errorf(parameter->line_, "there is already a parameter '%s' defined in this procedure", parameter->id_->c_str());
            return false;
        }
    }

    method_->params_.push_back(parameter);

    return true;
}

bool SymbolTable::insert(Local* local)
{
    pair<Method::LocalMap::iterator, bool> p
        = method_->locals_.insert( std::make_pair(local->id_, local) );

    if ( !p.second )
    {
        errorf(local->line_, "there is already a local '%s' defined in this scope in line %i", local->id_->c_str(), p.first->second->line_);

        return false;
    }

    for (size_t i = 0; i < method_->params_.size(); ++i)
    {
        if (*local->id_ == *method_->params_[i]->id_)
        {
            errorf(local->line_, "local '%s' shadows a parameter", local->id_->c_str());
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------

void SymbolTable::enterModule()
{
    module_ = rootModule_;
}

void SymbolTable::leaveModule()
{
    module_ = 0;
}

void SymbolTable::enterClass(std::string* id)
{
    Module::ClassMap::iterator iter = module_->classes_.find(id);
    swiftAssert(iter != module_->classes_.end(), "class not found");
    class_ = iter->second;;
}

void SymbolTable::leaveClass()
{
    class_ = 0;
}

void SymbolTable::enterMethod(std::string* id)
{
    Class::MethodMap::iterator iter = class_->methods_.find(id);
    swiftAssert(iter != class_->methods_.end(), "method not found");
    method_ = iter->second;
}

void SymbolTable::leaveMethod()
{
    method_ = 0;
}

// -----------------------------------------------------------------------------

SymTabEntry* SymbolTable::lookupVar(string* id)
{
    {
        // is it a local?
        Method::LocalMap::iterator iter = method_->locals_.find(id);
        if ( iter != method_->locals_.end() )
            return iter->second;
    }

    // no - perhaps a parameter?
    for (size_t i = 0; i < method_->params_.size(); ++i)
    {
        if (*method_->params_[i]->id_ == *id)
            return method_->params_[i];
    }

    {
        // no - perhaps a member?
        Class::MemberVarMap::iterator iter = class_->memberVars_.find(id);
        if ( iter != class_->memberVars_.end() )
            return iter->second;

        // id has not been found -- so return NULL
        return 0;
    }
}

Type* SymbolTable::lookupType(string* id)
{
    // FIXME merge this with lookupType
    {
        // is it a local?
        Method::LocalMap::iterator iter = method_->locals_.find(id);
        if ( iter != method_->locals_.end() )
            return iter->second->type_;
    }

    // no - perhaps a parameter?
    for (size_t i = 0; i < method_->params_.size(); ++i)
    {
        if (*method_->params_[i]->id_ == *id)
            return method_->params_[i]->type_;
    }

    {
        // no - perhaps a member?
        Class::MemberVarMap::iterator iter = class_->memberVars_.find(id);
        if ( iter != class_->memberVars_.end() )
            return iter->second->type_;

        // id has not been found -- so return NULL
        return 0;
    }
}

Class* SymbolTable::lookupClass(string* id)
{
    // currently only one module - the default module - is supported.
    std::map<std::string*, Class*, StringPtrCmp>::iterator iter = rootModule_->classes_.find(id);
    if ( iter != rootModule_->classes_.end() )
        return iter->second;

    // class not found -- so return NULL
    return 0;
}
