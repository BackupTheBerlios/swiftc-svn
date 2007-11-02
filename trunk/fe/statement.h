#ifndef SWIFT_STATEMENT_H
#define SWIFT_STATEMENT_H

#include "utils/assert.h"
#include "fe/syntaxtree.h"

// forward declarations
struct Expr;
struct Local;

//------------------------------------------------------------------------------

/**
 * This class represents a Statement. It is either a Declaration, an
 * ExprStatement, an IfElStatement or an AssignStatement.d
 */
struct Statement : public Node
{
    Statement* next_; ///< Linked List of statements.

/*
    constructor and destructor
*/

    Statement(int line);
    virtual ~Statement();

/*
    further methods
*/

    virtual bool analyze() = 0;
};

//------------------------------------------------------------------------------

/**
 * This is Declaration, consisting of a Type, an Identifier and an ExprList.
 * Furthermore it will create a Local which will be inserted in the SymbolTable.
 */
struct Declaration : public Statement
{
    Type*           type_;
    std::string*    id_;
    ExprList*       exprList_;

    /// Since this class created the Local it is responsible to delete it again.
    Local*          local_;

/*
    constructor and destructor
*/

    Declaration(Type* type, std::string* id, ExprList* exprList, int line = NO_LINE);
    virtual ~Declaration();

/*
    further methods
*/

    virtual bool analyze();
};

//------------------------------------------------------------------------------

/**
 * An ExprStatement is an Statement which holds an Expr.
 */
struct ExprStatement : public Statement
{
    Expr* expr_;

/*
    constructor and destructor
*/

    ExprStatement(Expr* expr, int line = NO_LINE)
        : Statement(line)
        , expr_(expr)
    {}
    virtual ~ExprStatement();

/*
    further methods
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
    constructor and destructor
*/

    IfElStatement(Expr* expr, Statement* ifBranch, Statement* elBranch, int line = NO_LINE)
        : Statement(line)
        , expr_(expr)
        , ifBranch_(ifBranch)
        , elBranch_(elBranch)
    {}
    virtual ~IfElStatement();

/*
    further methods
*/

    virtual bool analyze();
    virtual std::string toString() const { return std::string(""); }
};


//------------------------------------------------------------------------------

/**
 * This is an ordinary assignment. In contrast to other languages assignments in
 * Swift are no expresions but statements.
 */
struct AssignStatement : public Statement
{
    union
    {
        int kind_;
        char c_; ///< '=', others will follow.
    };

    Expr*       expr_;      ///< The lvalue.
    ExprList*   exprList_;  ///< The rvalue.

/*
    constructor and destructor
*/

    AssignStatement(int kind, Expr* expr, ExprList* exprList, int line = NO_LINE)
        : Statement(line)
        , expr_(expr)
        , exprList_(exprList)
    {}
    virtual ~AssignStatement();

/*
    further methods
*/

    virtual bool analyze();
};

#endif // SWIFT_STATEMENT_H
