#ifndef SWIFT_SYMBOLTABLE_H
#define SWIFT_SYMBOLTABLE_H

#include <string>
#include <vector>
#include <stack>
#include <map>

#include "fe/class.h"
#include "fe/syntaxtree.h"
#include "fe/type.h"

struct SymbolTable
{
    Module* rootModule_;

    Module* module_;
    Class*  class_;
    Method* method_;

    typedef std::stack<Scope*> ScopeStack;
    ScopeStack scopeStack_;

    SymbolTable()
    {
        reset();
    }

    void reset()
    {
        module_ = 0;
        class_  = 0;
        method_ = 0;
    }

    bool insert(Module* module);
    bool insert(Class* _class);
    bool insert(Method* method);
    bool insert(MemberVar* memberVar);
    bool insert(Parameter* parameter);
    bool insert(Local* local);
    void insertLocalByRegNr(Local* local);

    void enterModule();
    void leaveModule();
    void enterClass(std::string* id);
    void leaveClass();
    void enterMethod(std::string* id);
    void leaveMethod();
    void enterScope(Scope* scope);
    void leaveScope();

    Scope* currentScope()
    {
        return scopeStack_.top();
    }

    Type* lookupType(std::string* id);
    SymTabEntry* lookupVar(std::string* id);
    SymTabEntry* lookupVar(int regNr)
    {
        return currentScope()->lookupLocal(regNr);
    }
    void replaceRegNr(int oldNr, int newNr)
    {
        return currentScope()->replaceRegNr(oldNr, newNr);
    }
    Class* lookupClass(std::string* id);
};

typedef SymbolTable SymTab;
extern SymTab* symtab;

#endif // SWIFT_SYMBOLTABLE_H
