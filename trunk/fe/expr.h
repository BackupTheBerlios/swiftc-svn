#ifndef SWIFT_EXPR_H
#define SWIFT_EXPR_H

#include "fe/syntaxtree.h"

// forward declarations
struct Expr;
struct PseudoReg;

//------------------------------------------------------------------------------

struct Expr : public Node
{
    bool        lvalue_;
    Type*       type_;

    /// this var holds the PseudoReg where the result is stored
    PseudoReg*  reg_;

    Expr(int line)
        : Node(line)
        , lvalue_(false)
        , type_(0)
    {}
    ~Expr();

    std::string toString() const { return ""; }

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

        bool        bool_;

        void*       ptr_;
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

    std::string toString() const { return *id_; }

    bool analyze();
    void genSSA();
};

//------------------------------------------------------------------------------

struct UnExpr : public Expr
{
    enum
    {
        NOT
    };

    union
    {
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
    bool analyze();
    void genSSA();
};

//------------------------------------------------------------------------------

/* replaced by AssignStatement

struct AssignExpr : public Expr
{
    union
    {
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

    bool analyze();
    void genSSA();
};

*/

//------------------------------------------------------------------------------

/// Actually not a Expr, but belongs to expressions
struct ExprList : public Node
{
    Expr*       expr_;  ///< the Expr owned by this instance
    ExprList*   next_;  ///< next element in the list

    ExprList(Expr* expr, ExprList* next = 0, int line = NO_LINE)
        : Node(line)
        , expr_(expr)
        , next_(next)
    {}
    ~ExprList()
    {
        delete expr_;
        if (next_)
            delete next_;
    }

    std::string toString() const { return "TODO"; }
    bool analyze();
    void genSSA();
};

//------------------------------------------------------------------------------

struct FunctionCall : public Expr
{
    std::string*    id_;
    ExprList*       exprList_;

    FunctionCall(std::string* id, ExprList* exprList, int line = NO_LINE)
        : Expr(line)
        , id_(id)
        , exprList_(exprList)
    {}
    ~FunctionCall()
    {
        delete exprList_;
    }

    bool analyze();
    void genSSA();
};

#endif // SWIFT_EXPR_H
