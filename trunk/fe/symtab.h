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

    typedef std::stack<SwiftScope*> ScopeStack;
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

    void enterModule();
    void leaveModule();
    void enterClass(std::string* id);
    void leaveClass();
    void enterMethod(std::string* id);
    void leaveMethod();
    void enterScope(SwiftScope* scope);
    void leaveScope();

    Type* lookupType(std::string* id);
    SymTabEntry* lookupVar(std::string* id);
    Class* lookupClass(std::string* id);
};

typedef SymbolTable SymTab;
extern SymTab* symtab;

#endif // SWIFT_SYMBOLTABLE_H
