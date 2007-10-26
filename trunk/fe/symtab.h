#ifndef SWIFT_SYMBOLTABLE_H
#define SWIFT_SYMBOLTABLE_H

#include <string>
#include <vector>
#include <stack>
#include <map>

// forward declarations
struct Class;
struct Method;
struct Module;
struct Scope;

/**
 * This is the symbol table which is used in the front-end. It is a global
 * which can be accessed via
 * \verbatim
    symtab->foo();
 * \endverbatim
 * A stack of scope manages scoping and thus the top of stack is the current
 * Scope. Other pointers point to current Module, Class, or Method.
 */
struct SymbolTable
{
    typedef std::stack<Scope*> ScopeStack;

    Module* rootModule_; ///< The root of the syntax tree.
    Module* module_;     ///< Current Module.
    Class*  class_;      ///< Current Class.
    Method* method_;     ///< Current Method.

    ScopeStack scopeStack_; ///< Top of stack knows the current Scope.

    int varCounter_; ///< Counter which gives new var numbers.

/*
    constructor and init stuff
*/

    SymbolTable();
    void reset();

/*
    insert methods
*/

    bool insert(Module* module);
    bool insert(Class* _class);
    void insert(Method* method);
    bool insert(MemberVar* memberVar);
    bool insert(Parameter* parameter);
    bool insert(Local* local);

    void insertLocalByRegNr(Local* local);

/*
    enter and leave methods
*/

    void enterModule();
    void leaveModule();
    void enterClass(Class* _class);
    void leaveClass();
    void enterMethod(Method* method);
    void leaveMethod();
    void enterScope(Scope* scope);
    void leaveScope();

    /**
     * creates a new scope with the current scope as parent one and enters it
     * @return the new created scope
    */
    Scope* createAndEnterNewScope();

/*
    lookup methods
*/

    /**
     * Lookups a Var -- either a Local or a Param -- by name.
     *
     * @return The Var or 0 if not found.
     */
    Var* lookupVar(std::string* id);

    /**
     * Lookups a Var -- either a Local or a Param -- by varNr.
     *
     * @return The Var or 0 if not found.
     */
    Var* lookupVar(int varNr);

    /**
     * Lookups a Class by name.
     *
     * @return The Var or 0 if not found.
     */
    Class* lookupClass(std::string* id);

    /**
     * @param inSig ingoing parameters
     */
    Method* lookupMethod(std::string* classId,
                         std::string* methodId,
                         int methodQualifier,
                         Sig& sig,
                         int line);

/*
    further methods
*/

    /// Returns the current Scope.
    Scope* currentScope();

    /// Returns a new int which can be used as VarNr.
    int newVarNr();
};

typedef SymbolTable SymTab;
extern SymTab* symtab;

#endif // SWIFT_SYMBOLTABLE_H
