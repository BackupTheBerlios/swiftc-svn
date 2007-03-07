#ifndef SWIFT_STATEMENT_H
#define SWIFT_STATEMENT_H

#include "syntaxtree.h"

namespace swift {

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

    Declaration(Type* type, std::string* id, int line = -1)
        : Statement(line)
        , type_(type)
        , id_(id)
    {}
    ~Declaration();

    std::string toString() const;
    bool analyze();
};

//------------------------------------------------------------------------------

struct ExprStatement : public Statement
{
    Expr* expr_;

    ExprStatement(Expr* expr, int line = -1)
        : Statement(line)
        , expr_(expr)
    {}
    ~ExprStatement();

    std::string toString() const { return std::string(""); }
    bool analyze();
};

} // namespace swift

#endif // SWIFT_STATEMENT_H
