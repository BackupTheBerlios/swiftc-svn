#ifndef SWIFT_EXPR_H
#define SWIFT_EXPR_H

#include "fe/syntaxtree.h"

#include "me/op.h"

namespace swift {

// forward declarations
struct Expr;

//------------------------------------------------------------------------------

/** 
 * @brief Base class for all expressions.
 *
 * All expressions inherit from this class. Implement \a analyze in order to
 * implement the syntax checking.
 */
struct Expr : public Node
{
    bool    lvalue_;
    Type*   type_;

    /// This me::Op knows where the result is stored.
    me::Op* place_;

    /*
     * constructor and destructor
     */

    Expr(int line);
    virtual ~Expr();

    /*
     * further methods
     */

    /** 
     * @brief Implement this in order to implenent syntax checking.
     * 
     * @return true -> syntax corrent, false oterwise
     */
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

    /*
     * constructor
     */

    Literal(int kind, int line = NO_LINE);

    /*
     * further methods
     */

    me::Op::Type toType() const;
    virtual bool analyze();
    virtual void genSSA();
    std::string toString() const;

    typedef std::map<int, me::Op::Type> TypeMap;
    static TypeMap* typeMap_;
};

//------------------------------------------------------------------------------

/**
 * This class represents an identifier.
 */
struct Id : public Expr
{
    std::string* id_;

    /*
     * constructor and destructor
     */

    Id(std::string* id, int line = NO_LINE);
    virtual ~Id();

    /*
     * further methods
     */

    virtual bool analyze();
    virtual void genSSA();
};

//------------------------------------------------------------------------------

/**
 * This class represents an unary Expression. This is either
 * unary minus or NOT.
 */
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

    /*
     * constructor and destructor
     */

    UnExpr(int kind, Expr* op, int line = NO_LINE);
    virtual ~UnExpr();

    /*
     * further methods
     */

    virtual bool analyze();
    virtual void genSSA();
};

//------------------------------------------------------------------------------

/**
 * @brief This class represents a binary expression. 
 *
 * This is either 
 * * +, -, *, /, MOD, DIV, AND, OR, XOR, <, >, <=, >=, ==, or <>. TODO
 */
struct BinExpr : public Expr
{
    union
    {
        int kind_;
        char c_;
    };

    Expr* op1_;
    Expr* op2_;

    /*
     * constructor and destructor
     */

    BinExpr(int kind, Expr* op1, Expr* op2, int line = NO_LINE);
    virtual ~BinExpr();

    /*
     * further methods
     */

    std::string getExprName() const;
    std::string getOpString() const;
    virtual bool analyze();
    virtual void genSSA();
};

//------------------------------------------------------------------------------

/**
 * @brief This class represents a comma sperated list of Expr instances used in
 * function/method/contructor calls.
 *
 * This is actually not a Expr, but belongs to expressions
 * so it is in this file.
 */
struct ExprList : public Node
{
    Expr*       expr_;  ///< the Expr owned by this instance
    ExprList*   next_;  ///< next element in the list

    /*
     * constructor and destructor
     */

    ExprList(Expr* expr, ExprList* next = 0, int line = NO_LINE);
    virtual ~ExprList();

    /*
     * further methods
     */

    virtual bool analyze();
};

//------------------------------------------------------------------------------

// TODO
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
    virtual void genSSA();
};

} // namespace swift

#endif // SWIFT_EXPR_H
