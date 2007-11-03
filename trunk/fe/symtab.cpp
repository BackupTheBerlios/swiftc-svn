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

/*
    TODO remove all error handling here
*/

using namespace std;

// init global
SymTab* symtab = 0;

/*
    constructor and init stuff
*/

SymbolTable::SymbolTable()
    : varCounter_(-1) // 0 is reserved for literals
{
    reset();
}

void SymbolTable::reset()
{
//     module_ = 0; TODO
    class_  = 0;
    method_ = 0;
}

/*
    insert methods
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

//         // build full class name with module names: module1.module2.Class1.Class2 etc
//         stack<string> idStack;
//         idStack.push(*_class->id_);
//
//         for (Node* iter = class_->parent_; iter != 0; iter = iter->parent_)
//             idStack.push( iter->toString() );
//
//         ostringstream oss;
//
//         while ( !idStack.empty() )
//         {
//             oss << idStack.top();
//             idStack.pop();
//
//             if ( !idStack.empty() )
//                 oss << '.';
//         }
// TODO

        // give proper error
        errorf(_class->line_, "there there is already a class '%s' defined in line %i", p.first->second->id_->c_str(), p.first->second->line_);

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

//         // build full class name with module names: module1.module2.Class1.Class2 etc
//         stack<string> idStack;
//
//         for (Node* iter = class_->parent_; iter != 0; iter = iter->parent_)
//             idStack.push( iter->toString() );
//
//         ostringstream oss;
//
//         while ( !idStack.empty() )
//         {
//             oss << idStack.top();
//             idStack.pop();
//
//             if ( !idStack.empty() )
//                 oss << '.';
//         }
// TODO
        errorf(memberVar->line_, "there is already a member '%s' defined in '%s' line %i", memberVar->id_->c_str(), p.first->second->id_, p.first->second->line_);

        return false;
    }

    return true;
}

void SymbolTable::insert(Method* method)
{
    Class::MethodMap::iterator iter
        = class_->methods_.insert( std::make_pair(method->proc_.id_, method) );

    // set current method scope
    method_ = method;
}

bool SymbolTable::insert(Param* param)
{
    PARAMS_EACH(iter, method_->proc_.sig_.params_)
    {
        if (*param->id_ == *iter->value_->id_)
        {
            errorf(param->line_, "there is already a parameter '%s' defined in this procedure",
                param->id_->c_str());

            return false;
        }
    }

    method_->appendParam(param);

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

    PARAMS_EACH(iter, method_->proc_.sig_.params_)
    {
        if (*local->id_ == *iter->value_->id_)
        {
            errorf(local->line_, "local '%s' shadows a parameter", local->id_->c_str());
            return false;
        }
    }

    return true;
}

void SymbolTable::insertLocalByVarNr(Local* local)
{
    pair<Scope::VarNrMap::iterator, bool> p
        = currentScope()->varNrs_.insert( std::make_pair(local->varNr_, local) );
    swiftAssert(p.second, "there is already a local with this varNr in the map");
}

/*
    enter and leave methods
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

    scopeStack_.push(method_->proc_.rootScope_);
}

void SymbolTable::leaveMethod()
{
    method_ = 0;
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
    symtab->enterScope(newScope);

    return newScope;
}

/*
    lookup methods
*/

Var* SymbolTable::lookupVar(string* id)
{
    // is it a local?
    Local* local = currentScope()->lookupLocal(id);
    if (local)
        return local;

    // no - perhaps a parameter?
    return method_->proc_.findParam(id); // will return 0, if not found
}

Var* SymbolTable::lookupVar(int varNr)
{
    return currentScope()->lookupLocal(varNr);
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

Method* SymbolTable::lookupMethod(std::string* classId,
                                  std::string* methodId,
                                  int methodQualifier,
                                  Sig& sig,
                                  int line,
                                  SigCheckingStyle sigCheckingStyle)
{
    // lookup class
    Class* _class = symtab->lookupClass(classId);

    // lookup method
    Class::MethodIter iter = _class->methods_.find(methodId);
    if (iter == _class->methods_.end())
    {
        errorf(line, "there is no method %s defined in class %s",
            methodId->c_str(), classId->c_str());

        return 0;
    }

    // get iterator to the first method, which has not methodId as identifier
    Class::MethodIter last = _class->methods_.upper_bound(methodId);

    // current method in loop below
    Method* method = 0;

    for (; iter != last; ++iter)
    {
        method = iter->second;

        bool sigCheck;

        switch (sigCheckingStyle)
        {
            case CHECK_JUST_INGOING:
                sigCheck = method->proc_.sig_.checkIngoing(sig);
                break;
            case CHECK_ALL:
                sigCheck = Sig::check(method->proc_.sig_, sig);
                break;
        }

        if (sigCheck)
            break;
        else
            method = 0; // mark as not found
    }

    if ( !method )
        errorf(line, "no method found for this class with the given arguments"); // TODO better error handling

    return method;
}

/*
    further methods
*/

Scope* SymbolTable::currentScope()
{
    return scopeStack_.top();
}

int SymbolTable::newVarNr()
{
    return varCounter_--;
}

Local* SymbolTable::createNewLocal(const Type* type, std::string* id, int line /*= NO_LINE*/)
{
    // create Local
    Local* local = new Local(type->clone(), id, newVarNr(), line);

    // insert into both maps
    symtab->insert(local);
    symtab->insertLocalByVarNr(local);

    return local;
}

