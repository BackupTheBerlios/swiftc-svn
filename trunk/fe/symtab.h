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

    /// destroys all contents of this class
    void destroy();

    /**
     * Creates a new revision of either an original variable or an already revised one
     * @param local the original variable
     * @return the new created revision
     */
//     Local* newRevision(Local* local);

    Type* lookupType(std::string* id);
    SymTabEntry* lookupVar(std::string* id);
    Class* lookupClass(std::string* id);
};

typedef SymbolTable SymTab;
extern SymTab symtab;

#endif // SWIFT_SYMBOLTABLE_H