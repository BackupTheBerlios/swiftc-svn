#ifndef SWIFT_SSA_H
#define SWIFT_SSA_H

#include <fstream>

#include "../utils/list.h"

#include "../fe/symboltable.h"

// forward declarations
struct Expr;

//------------------------------------------------------------------------------

struct InstrBase
{
    virtual ~InstrBase() {}

    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------
//TagInstr----------------------------------------------------------------------
//------------------------------------------------------------------------------

struct TagInstr : public InstrBase
{
    std::string* id_;
    bool enter_;

    TagInstr(std::string* id, bool enter)
        : id_(id)
        , enter_(enter)
    {}

    /// calls the appropriate enter method of the symtab
    virtual void enter() = 0;
    /// calls the appropriate leave method of the symtab
    virtual void leave() = 0;
};

//------------------------------------------------------------------------------

struct ModuleTagInstr : public TagInstr
{
    ModuleTagInstr(std::string* id, bool enter)
        : TagInstr(id, enter)
    {}

    std::string toString() const;
    void enter();
    void leave();
};

//------------------------------------------------------------------------------

struct ClassTagInstr : public TagInstr
{
    ClassTagInstr(std::string* id, bool enter)
        : TagInstr(id, enter)
    {}

    std::string toString() const;
    void enter();
    void leave();
};

//------------------------------------------------------------------------------

struct MethodTagInstr : public TagInstr
{
    MethodTagInstr(std::string* id, bool enter)
        : TagInstr(id, enter)
    {}

    std::string toString() const;
    void enter();
    void leave();
};

//------------------------------------------------------------------------------
//ExprInstr---------------------------------------------------------------------
//------------------------------------------------------------------------------

struct ExprInstr : public InstrBase
{
    virtual void genCode(std::ofstream& ofs) = 0;
};

struct AssignInstr : public ExprInstr
{
    union
    {
        int kind_;
        char c_;
    };

    Expr* result_;
    Expr* expr_;

    AssignInstr(int kind, Expr* result, Expr* expr)
        : kind_(kind)
        , result_(result)
        , expr_(expr)
    {}

    std::string toString() const;
    void genCode(std::ofstream& ofs);
};

//------------------------------------------------------------------------------

/**
 *
 */
struct UnInstr : public ExprInstr
{
    union
    {
        int kind_;
        char c_;
    };

    Expr* result_;
    Expr* op_;

    UnInstr(int kind, Expr* result, Expr* op)
        : kind_(kind)
        , result_(result)
        , op_(op)
    {}

    std::string toString() const;
    void genCode(std::ofstream& ofs);
};

//------------------------------------------------------------------------------

/**
 *  result = op1 + op2 <br>
 *  result = op1 - op2 <br>
 *  result = op1 * op2 <br>
 *  result = op1 / op2 <br>
 *  result = op1 % op2 <br>
 *  <br>
 *  result = op1 and op2 <br>
 *  result = op1 or  op2 <br>
 *  result = op1 xor op2 <br>
 *  <br>
 *  result = op1 l_and op2 <br>
 *  result = op1 l_or  op2 <br>
 *  result = op1 l_xor op2 <br>
 *  <br>
 *  result = op1 == op2 <br>
 *  result = op1 != op2 <br>
 *  result = op1 \< op2 <br>
 *  result = op1 >  op2 <br>
 *  result = op1 <= op2 <br>
 *  result = op1 >= op2 <br>
*/
struct BinInstr : public ExprInstr
{
    union
    {
        int kind_;
        char c_;
    };

    Expr* result_;
    Expr* op1_;
    Expr* op2_;

    BinInstr(int kind, Expr* result, Expr* op1, Expr* op2)
        : kind_(kind)
        , result_(result)
        , op1_(op1)
        , op2_(op2)
    {}

    std::string toString() const;
    void genCode(std::ofstream& ofs);
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//BranchInstr-------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * if a goto b
*/

// struct GotoInstr
// {
// };

/**
 *  result =  expr <br>
 *  result <- expr <br>
 *  result <-> expr <br>
 *  <br>
 *  result += expr <br>
 *  result -= expr <br>
 *  result *= expr <br>
 *  result /= expr <br>
 *  result %= expr <br>
 *
 *  result and= expr <br>
 *  result or=  expr <br>
 *  result xor= expr <br>
*/


typedef List<InstrBase*> InstrList;
extern InstrList instrlist;

#endif // SWIFT_SSA_H
