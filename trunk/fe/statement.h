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

#ifndef SWIFT_STATEMENT_H
#define SWIFT_STATEMENT_H

#include "utils/assert.h"

#include "fe/syntaxtree.h"
#include "fe/typelist.h"

namespace swift {

/*
 * forward declarations
 */

class Expr;
class ExprList;
class Local;
class Tupel;

//------------------------------------------------------------------------------

/**
 * @brief This class represents a Statement. 
 *
 * It is either a DeclStatement, an ExprStatement, an IfElStatement 
 * or an AssignStatement.
 */
struct Statement : public Node
{
    Statement* next_; ///< Linked List of statements.

    /*
     * constructor and destructor
     */

    Statement(int line);
    virtual ~Statement();

    /*
     * virtual methods
     */

    virtual bool analyze() = 0;
};

//------------------------------------------------------------------------------

/**
 * An ExprStatement is an Statement which holds an Expr.
 */
struct ExprStatement : public Statement
{
    Expr* expr_;

    /*
     * constructor and destructor
     */

    ExprStatement(Expr* expr, int line)
        : Statement(line)
        , expr_(expr)
    {}
    virtual ~ExprStatement();

    /*
     * further methods
     */

    virtual bool analyze();
    virtual std::string toString() const { return std::string(""); }
};

//------------------------------------------------------------------------------

/**
 * @brief This is DeclStatement, consisting of a Type, an Identifier and an ExprList.
 *
 * Furthermore it will create a Local which will be inserted in the SymbolTable.
 */
class DeclStatement : public Statement
{
public:
    /*
     * constructor and destructor
     */

    DeclStatement(Decl* decl, int line);
    virtual ~DeclStatement();

    /*
     * virtual methods
     */

    virtual std::string toString() const;

    /*
     * further methods
     */

    virtual bool analyze();

private:

    /*
     * data
     */

    Decl* decl_;
};

//------------------------------------------------------------------------------

/**
 * @brief This is an ordinary assignment. 
 *
 * In contrast to other languages assignments in Swift are not expresions 
 * but statements.
 */
struct AssignStatement : public Statement
{
    union
    {
        int kind_;
        char c_; ///< '=', others will follow.
    };

    Tupel* tupel_;       ///< The lvalue.
    ExprList* exprList_; ///< The rvalue.

    /*
     * constructor and destructor
     */

    AssignStatement(int kind, Tupel* tupel, ExprList* exprList, int line);
    virtual ~AssignStatement();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const;

    /*
     * further methods
     */

    void genMove();
    //void genConstructorCall(Class* _class, Method* method);
    //void genAssignCall(Class* _class, Method* method);
    //void genSSA();

    bool analyzeFunctionCall();
    bool analyzeAssignCreate();
    bool constCheck();
};

//------------------------------------------------------------------------------

/**
 * Represents a while statement.
 */
struct ScopeStatement : public Statement
{
    Statement* statements_; ///< Linked List of statements of the while-loop.

    /*
     * constructor and destructor
     */

    ScopeStatement(Statement* statements, int line);
    virtual ~ScopeStatement();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const { return std::string(""); }
};

//------------------------------------------------------------------------------

/**
 * Represents a while statement.
 */
struct WhileStatement : public Statement
{
    Expr* expr_;

    Statement* statements_; ///< Linked List of statements of the while-loop.

    /*
     * constructor and destructor
     */

    WhileStatement(Expr* expr, Statement* statements, int line);
    virtual ~WhileStatement();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const { return std::string(""); }
};

//------------------------------------------------------------------------------

/**
 * Holds either an if, an if-else or an if-elif statement.
 */
struct IfElStatement : public Statement
{
    Expr* expr_;

    Statement* ifBranch_; ///< Linked List of statements of the if-branch.
    Statement* elBranch_; ///< Linked List of statements of the else-branch, 0 if there is no else-branch.

    /*
     * constructor and destructor
     */

    IfElStatement(Expr* expr, Statement* ifBranch, Statement* elBranch, int line);
    virtual ~IfElStatement();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const { return std::string(""); }
};

//------------------------------------------------------------------------------

struct CFStatement : public Statement
{
    int kind_;

    /*
     * constructor
     */

    CFStatement(int kind, int line);

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const { return std::string(""); }
};

} // namespace swift

#endif // SWIFT_STATEMENT_H
