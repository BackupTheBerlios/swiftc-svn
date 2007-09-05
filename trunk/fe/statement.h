#ifndef SWIFT_STATEMENT_H
#define SWIFT_STATEMENT_H

#include "utils/assert.h"
#include "fe/syntaxtree.h"

// forward declarations
struct Expr;
struct Local;

//------------------------------------------------------------------------------

struct Statement : public Node
{
    Statement* next_;

    Statement(int line)
        : Node(line)
        , next_(0)
    {}
    ~Statement()
    {
        if (next_)
            delete next_;
    };

    virtual bool analyze() = 0;
};

//------------------------------------------------------------------------------

struct Declaration : public Statement
{
    Type*           type_;
    std::string*    id_;
    Local*          local_; // in order to delete those locals again
    InitList*       initList_;

    Declaration(Type* type, std::string* id, InitList* initList, int line = NO_LINE)
        : Statement(line)
        , type_(type)
        , id_(id)
        , local_(0)
        , initList_(initList)
    {}
    ~Declaration();

    std::string toString() const;
    bool analyze();
};

//------------------------------------------------------------------------------

struct ExprStatement : public Statement
{
    Expr* expr_;

    ExprStatement(Expr* expr, int line = NO_LINE)
        : Statement(line)
        , expr_(expr)
    {}
    ~ExprStatement();

    std::string toString() const { return std::string(""); }
    bool analyze();
};

/**
 * @brief Holds either an if, an if-else or an if-elif statement.
 */
struct IfElStatement : public Statement
{
    int kind_;
    Expr* expr_;

    Statement* ifBranch_;
    Statement* elBranch_;

    IfElStatement(int kind_, Expr* expr, Statement* ifBranch, Statement* elBranch, int line = NO_LINE)
        : Statement(line)
        , expr_(expr)
        , ifBranch_(ifBranch)
        , elBranch_(elBranch)
    {
        swiftAssert( kind_ == 0 || kind_ == ELSE || kind_ == ELIF, "kind_ must be 0, ELSE or ELIF" );
    }
    ~IfElStatement();

    std::string toString() const { return std::string(""); }
    /// SSA code will be generated here, too
    bool analyze();
};


//------------------------------------------------------------------------------

struct AssignStatement : public Statement
{
    union
    {
        int kind_;
        char c_;
    };

    Expr* expr_;
    InitList* initList_;

    AssignStatement(int kind, Expr* expr, InitList* initList, int line = NO_LINE)
        : Statement(line)
        , expr_(expr)
        , initList_(initList)
    {}

    ~AssignStatement();

    std::string toString() const;

    bool analyze();
};

//------------------------------------------------------------------------------

struct InitList : public Node
{
    InitList*   child_;
    InitList*   next_;
    Expr*       expr_;

    InitList(InitList* child, Expr* expr, int line = NO_LINE)
        : Node(line)
        , child_(child)
        , expr_(expr)
    {}
    ~InitList();

    std::string toString() const;

    bool analyze();
};

#endif // SWIFT_STATEMENT_H
