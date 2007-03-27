#ifndef SWIFT_SYMBOLTABLE_H
#define SWIFT_SYMBOLTABLE_H

#include <string>
#include <vector>
#include <stack>
#include <map>

#include "syntaxtree.h"
#include "class.h"
#include "type.h"

struct ScopeTable
{
    Scope* rootScope_;

    /**
     * Creates a new temporary variable in order to divide expressions
     * @param type type of the new temp
     * @return new created Local
     */
    Local* newTemp(Type* type);

    /**
     * Creates a new revision of either an original variable or an already revised one
     * @param local the original variable
     * @return the new created revision
     */
    Local* newRevision(Local* local);

    Type* lookupType(std::string* id);
    SymTabEntry* lookupVar(std::string* id);
    Class* lookupClass(std::string* id);
    /// Use only with original variables
    SymTabEntry* lookupLastRevision(Local* local);
};

typedef ScopeTable ScopeTab;
extern SymTab scopetab;

#endif // SWIFT_SYMBOLTABLE_H
