#ifndef SWIFT_SYMBOLTABLE_H
#define SWIFT_SYMBOLTABLE_H

#include <string>
#include <vector>
#include <stack>
#include <map>

#include "fe/var.h"

namespace swift {

// forward declarations
struct MemberVar;
struct Method;
struct Module;
struct Scope;
struct Sig;
struct Type;
struct Var;

/**
 * @brief This is the symbol table which is used in the front-end. 
 *
 * It is a global which can be accessed via
@verbatim
symtab->foo();
@endverbatim
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
    Sig*    sig_;        ///< Current Sig.

    ScopeStack scopeStack_; ///< Top of stack knows the current Scope.

    enum
    {
        NO_LINE = -1
    };

    enum SigCheckingStyle
    {
        CHECK_JUST_INGOING,
        CHECK_ALL
    };

    /*
     * constructor and init stuff
     */

    SymbolTable();
    void reset();

    /*
     * insert methods
     */

    bool insert(Module* module);
    bool insert(Class* _class);
    void insert(Method* method);
    bool insert(MemberVar* memberVar);
    bool insert(Param* param);
    bool insert(Local* local);

    void insertLocalByVarNr(Local* local);

    /*
     * enter and leave methods
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
     * lookup methods
     */

    /**
     * Lookups a Var -- either a Local or a Param -- by name.
     *
     * @return The Var or 0 if not found.
     */
    Var* lookupVar(std::string* id);

    /**
     * Lookups a Class by name.
     *
     * @return The Var or 0 if not found.
     */
    Class* lookupClass(std::string* id);

    /**
     * Looks up a method.
     *
     * @param classId The identifier of the method's class.
     * @param methodId The identifier of the method to be lookuped.
     * @param methodQualifier Either READER, WRITER, ROUTINE, CREATE or OPERATOR.
     * @param sig Signature the method should have.
     * @param line The line number of the method call.
     * @param sigCheckingStyle Which params exactly should be checked?
     */
    Method* lookupMethod(std::string* classId,
                         std::string* methodId,
                         int methodQualifier,
                         Sig& sig,
                         int line,
                         SigCheckingStyle sigCheckingStyle);

    /*
     * further methods
     */

    /// Returns the current Scope.
    Scope* currentScope();

    /**
     * Creates a new Local with Type \p type, \p id and an optional \p line
     * number.
     *
     * @param type Type the new Local should have. It will be cloned via Type::clone().
     * @param id Identifier the new Local should have.
     * @param line An optional line number the Local should have.
     */
    Local* createNewLocal(const Type* type, std::string* id, int line = NO_LINE);
};

typedef SymbolTable SymTab;
extern SymTab* symtab;

} // namespace swift

#endif // SWIFT_SYMBOLTABLE_H
