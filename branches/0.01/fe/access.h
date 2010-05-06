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

#include "fe/expr.h"

namespace swift {

//------------------------------------------------------------------------------

class Access : public Expr
{
public:

    /*
     * constructor and destructor
     */

    Access(Expr* postfixExpr, int line);
    virtual ~Access();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual bool analyzeAccess() = 0;
    virtual bool needsNewChain() const = 0;

    /*
     * further methods
     */

    void genSSA();
    void emitStoreIfApplicable(Expr* expr);

protected:

    /*
     * data
     */

    Expr* postfixExpr_;
    Access* nextAccess_;
    bool firstInAChain_;
    bool lastInAChain_;

    /// The Offset of the current access.
    me::Offset* offset_;
    /// The StructOffset of the left most place in each chain.
    me::Offset* rootOffset_;
    /// The place of the left most place in each chain.
    me::Var* rootVar_; 
    me::Reg* index_;
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
class MemberAccess : public Access
{
public:

    /*
     * constructor and destructor
     */

    MemberAccess(Expr* expr_, std::string* id, int line = NO_LINE);
    virtual ~MemberAccess();

    /*
     * virtual methods
     */

    virtual bool analyzeAccess();
    virtual bool needsNewChain() const;
    virtual std::string toString() const;

private:

    /*
     * data
     */

    std::string* id_;
};

//------------------------------------------------------------------------------

class IndexExpr : public Access
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

    virtual bool analyzeAccess();
    virtual bool needsNewChain() const;
    virtual std::string toString() const;

private:

    /*
     * data
     */

    Expr* indexExpr_;
};

//------------------------------------------------------------------------------

} // namespace swift
