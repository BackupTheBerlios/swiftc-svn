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

#ifndef SWIFT_EXPR_H
#define SWIFT_EXPR_H

#include "fe/syntaxtree.h"

#include "me/op.h"

/*
 * forward declarations
 */

namespace me {
    class Struct;
    class StructOffset;
    class Member;
    class MemVar;
    class Var;
}

namespace swift {

class Expr;
class Var;

//------------------------------------------------------------------------------

/** 
 * @brief Base class for all expressions.
 *
 * All expressions inherit from this class. Implement \a analyze in order to
 * implement the syntax checking.
 */
class Expr : public TypeNode
{
public:

    /*
     * constructor
     */

    Expr(int line);

    /*
     * virtual methods
     */

    virtual void genSSA() = 0;

    /*
     * further methods
     */

    void neededAsLValue();
    bool isNeededAsLValue() const;

protected:

    /*
     * data
     */

private:

    bool  neededAsLValue_;
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

    /*
     * virtual methods
     */

    virtual std::string toString() const;

    /*
     * static methods
     */

    static void initTypeMap();
    static void destroyTypeMap();

private:

    /*
     * data
     */

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
    Var* var_;

    /*
     * constructor and destructor
     */

    Id(std::string* id, int line = NO_LINE);
    virtual ~Id();

    /*
     * virtual methods
     */

    virtual std::string toString() const;
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
     * virtual methods
     */

    virtual std::string toString() const;
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
     * virtual methods
     */

    virtual std::string toString() const;
    virtual bool analyze();
    virtual void genSSA();

    /*
     * further methods
     */

    std::string getExprName() const;
    std::string getOpString() const;
};

//------------------------------------------------------------------------------

class IndexExpr : public Expr
{
public:

    /*
     * constructor and destructor
     */

    IndexExpr(Expr* postfixExpr, Expr* indexExpr, int line);
    virtual ~IndexExpr();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual void genSSA();
    virtual std::string toString() const;

private:

    /*
     * data
     */

    Expr* postfixExpr_;
    Expr* indexExpr_;
};

//------------------------------------------------------------------------------

/**
 * @brief Represents an access of a class member.
 *
 * A chain of member accesses can be calcuated in one offset
 * if all accesses are compile time known offsets. Otherwise the accesses must
 * be split. \a rootVar_ holds the place of the left most place of each chain
 * which holds the root of each access.
 */
struct MemberAccess : public Expr
{
    Expr* expr_;
    std::string* id_;
    bool right_;

    /// The StructOffset of the current access.
    me::StructOffset* structOffset_;
    /// The StructOffset of the left most place in each chain.
    me::StructOffset* rootStructOffset_;
    /// The place of the left most place in each chain.
    me::Var* rootVar_; 

    bool storeNecessary_;

    /*
     * constructor and destructor
     */

    MemberAccess(Expr* expr_, std::string* id, int line = NO_LINE);
    virtual ~MemberAccess();

    /*
     * virtual methods
     */

    virtual std::string toString() const;
    virtual bool analyze();
    virtual void genSSA();

    /*
     * further methods
     */

    void emitStoreIfApplicable();
};

//------------------------------------------------------------------------------

class Nil : public Expr
{
public:

    /*
     * constructor and destructor
     */

    Nil(Type* innerType, int line);
    virtual ~Nil();

    /*
     * virtual methods
     */

    virtual std::string toString() const;
    virtual bool analyze();
    virtual void genSSA();

private:

    /*
     * data
     */

    Type* innerType_;
};

//------------------------------------------------------------------------------

class Self : public Expr
{
public:

    /*
     * constructor
     */

    Self(int line);

    /*
     * virtual methods
     */

    virtual std::string toString() const;
    virtual bool analyze();
    virtual void genSSA();
};

//------------------------------------------------------------------------------

class SimdIndex : public Expr
{
public:

    /*
     * constructor
     */

    SimdIndex(int line);

    /*
     * virtual methods
     */

    virtual std::string toString() const;
    virtual bool analyze();
    virtual void genSSA();
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_EXPR_H
