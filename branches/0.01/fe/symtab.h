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

#ifndef SWIFT_SYMBOLTABLE_H
#define SWIFT_SYMBOLTABLE_H

#include <string>
#include <vector>
#include <stack>
#include <map>

#include "fe/typelist.h"
#include "fe/var.h"

namespace swift {

// forward declarations
struct Assign;
struct Create;
struct MemberVar;
struct MemberFunction;
struct Method;
struct Module;
struct Scope;
struct Signature;
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
 * Scope. Other pointers point to current Module, Class, or MemberFunction.
 */
struct SymbolTable
{
    typedef std::stack<Scope*> ScopeStack;

    Module* rootModule_;            ///< The root of the syntax tree.
    Module* module_;                ///< Current \a Module.
    Class*  class_;                 ///< Current \a Class.
    MemberFunction* memberFunction_;///< Current \a MemberFunction.
    Signature* sig_;                ///< Current \a Signature.
    ScopeStack scopeStack_;         ///< Top of stack knows the current Scope.

    enum
    {
        NO_LINE = -1
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
    void insert(MemberFunction* memberFunction);
    bool insert(MemberVar* memberVar);
    void insertInParam(InParam* param);
    void insertOutParam(OutParam* param);
    bool insert(Local* local);

    /*
     * enter and leave methods
     */

    void enterModule();
    void leaveModule();
    void enterClass(Class* _class);
    void leaveClass();
    void enterMemberFunction(MemberFunction* memberFunction);
    void leaveMemberFunction();
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
    Var* lookupVar(const std::string* id);

    /**
     * Lookups a Class by name.
     *
     * @return The Var or 0 if not found.
     */
    Class* lookupClass(const std::string* id);

    /**
     * Looks up a member function.
     *
     * @param _class The member function's class.
     * @param id The identifier of the member function to be lookuped.
     * @param methodQualifier Either READER, WRITER, ROUTINE, CREATE or OPERATOR.
     * @param in in-signature the member function should have.
     * @param line The line number of the MemberFunction call.
     *          Use 0 if you want to omit error output.
     */
    MemberFunction* lookupMemberFunction(Class* _class,
                                         const std::string* id,
                                         const TypeList& in,
                                         int line);

    /**
     * Looks up a constructor.
     *
     * @param _class The contructor's class.
     * @param in The in-signature the method should have.
     * @param line The line number of the method call.
     *          Use 0 if you want to omit error output.
     */
    Create* lookupCreate(Class* _class,
                         const TypeList& in,
                         int line);

    /**
     * Looks up the assign operator.
     *
     * @param _class The assign operator's class.
     * @param in The in-signature the method should have.
     * @param line The line number of the method call.
     *          Use 0 if you want to omit error output.
     */
    Assign* lookupAssign(Class* _class, const TypeList& in, int line);

    /**
     * Looks up the assign constructor.
     *
     * @param _class The method's class.
     * @param in The in-signature the method should have.
     * @param line The line number of the method call.
     *          Use 0 if you want to omit error output.
     */
    Method* lookupAssignCreate(Class* _class, const TypeList& in, bool create, int line);

    /*
     * current getters
     */

    /// Returns the current Scope.
    Scope* currentScope();

    Class* currentClass();
    MemberFunction* currentMemberFunction();


    /*
     * further methods
     */

    /**
     * Creates a new Local with Type \p type, \p id and an optional \p line
     * number.
     *
     * @param type Type the new Local should have. It will be cloned via Type::clone().
     * @param id Identifier the new Local should have.
     * @param line An optional line number the Local should have.
     */
    std::pair<Local*, bool> createNewLocal(const Type* type, 
                                           std::string* id, 
                                           int line = NO_LINE);
};

typedef SymbolTable SymTab;
extern SymTab* symtab;

} // namespace swift

#endif // SWIFT_SYMBOLTABLE_H
