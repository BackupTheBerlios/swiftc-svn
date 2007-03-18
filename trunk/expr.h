#ifndef SWIFT_EXPR_H
#define SWIFT_EXPR_H

#include "syntaxtree.h"

namespace swift {

// forward declarations
struct Expr;

//------------------------------------------------------------------------------

struct Expr : public Node
{
    bool            lvalue_;
    Type*           type_;
    SymTabEntry*    place_;

    Expr(int line)
        : Node(line)
        , lvalue_(false)
        , type_(0)
    {}
    ~Expr();

    virtual bool analyze() = 0;
    virtual void genSSA() = 0;
};

//------------------------------------------------------------------------------

struct Literal : public Expr
{
    int kind_;

    union {
        size_t      index_;

        int         int_;
        int8_t      int8_;
        int16_t     int16_;
        int32_t     int32_;
        int64_t     int64_;
        int8_t      sat8_;
        int16_t     sat16_;

        uint        uint_;
        uint8_t     uint8_;
        uint16_t    uint16_;
        uint32_t    uint32_;
        uint64_t    uint64_;
        uint8_t     usat8_;
        uint16_t    usat16_;

        float       real_;
        float       real32_;
        double      real64_;
    };

    Literal(int kind, int line = NO_LINE)
        : Expr(line)
        , kind_(kind)
    {}

    std::string toString() const;
    bool analyze();
    void genSSA();
};

//------------------------------------------------------------------------------

struct Id : public Expr
{
    std::string* id_;

    Id(std::string* id, int line = NO_LINE)
        : Expr(line)
        , id_(id)
    {}
    ~Id()
    {
        delete id_;
    }

    std::string toString() const
    {
        return *place_->id_;
    }

    bool analyze();
    void genSSA();
};

//------------------------------------------------------------------------------

struct UnExpr : public Expr
{
    union {
        int kind_;
        char c_;
    };

    Expr* op_;

    UnExpr(int kind, Expr* op, int line = NO_LINE)
        : Expr(line)
        , kind_(kind)
        , op_(op)
    {}
    ~UnExpr()
    {
        delete op_;
    }

    std::string toString() const;
    bool analyze();
    void genSSA();
};

//------------------------------------------------------------------------------

struct BinExpr : public Expr
{
    union {
        int kind_;
        char c_;
    };

    Expr* op1_;
    Expr* op2_;

    BinExpr(int kind, Expr* op1, Expr* op2, int line = NO_LINE)
        : Expr(line)
        , kind_(kind)
        , op1_(op1)
        , op2_(op2)
    {}
    ~BinExpr()
    {
        delete op1_;
        delete op2_;
    }

    std::string getExprName() const;
    std::string getOpString() const;
    std::string toString() const;
    bool analyze();
    void genSSA();
};

//------------------------------------------------------------------------------

struct AssignExpr : public Expr
{
    union {
        int kind_;
        char c_;
    };

    Expr* result_;
    Expr* expr_;

    AssignExpr(int kind, Expr* result, Expr* expr, int line = NO_LINE)
        : Expr(line)
        , kind_(kind)
        , result_(result)
        , expr_(expr)
    {}
    ~AssignExpr()
    {
        delete result_;
        delete expr_;
    }

    std::string toString() const;
    bool analyze();
    void genSSA();
};

//------------------------------------------------------------------------------

// actually not a Expr, but belongs to expressions
struct Arg : public Node
{
    Expr*   expr_;
    Arg*    next_;

    Arg(Expr* expr, Arg* next = 0, int line = NO_LINE)
        : Node(line)
        , expr_(expr)
        , next_(next)
    {}
    ~Arg()
    {
        delete expr_;
        if (next_)
            delete next_;
    }

    std::string toString() const { return std::string(""); }
    void genSSA();
};

//------------------------------------------------------------------------------

struct FunctionCall : public Expr
{
    std::string* id_;
    Arg* args_;

    FunctionCall(std::string* id, Arg* args, int line = NO_LINE)
        : Expr(line)
        , id_(id)
        , args_(args)
    {}
    ~FunctionCall()
    {
        delete id_;
        delete args_;
    }

    std::string toString() const;
    bool analyze();
    void genSSA();
};

} // namespace swift

#endif // SWIFT_EXPR_H
