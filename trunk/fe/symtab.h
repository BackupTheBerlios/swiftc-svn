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

    int varCounter_;

    SymbolTable()
        : varCounter_(-1) // 0 is reserved for literals
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
    void insert(Method* method);
    bool insert(MemberVar* memberVar);
    bool insert(Parameter* parameter);
    bool insert(Local* local);

    void insertLocalByRegNr(Local* local);

    bool checkSignature();

    void enterModule();
    void leaveModule();
    void enterClass(std::string* id);
    void leaveClass();
    void enterMethod(std::string* id);
    void leaveMethod();
    void enterScope(Scope* scope);
    void leaveScope();

    /**
     * creates a new scope with the current scope as parent one and enters it
     * @return the new created scope
    */
    Scope* createAndEnterNewScope();

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

    Class* lookupClass(std::string* id);

    int newVarNr()
    {
        return varCounter_--;
    }
};

typedef SymbolTable SymTab;
extern SymTab* symtab;

#endif // SWIFT_SYMBOLTABLE_H
