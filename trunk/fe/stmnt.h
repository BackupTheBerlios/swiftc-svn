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

#include "fe/auto.h"
#include "fe/node.h"
#include "fe/typelist.h"

namespace swift {

//------------------------------------------------------------------------------

class Decl;
class Expr;
class ExprList;
class SimdPrefix;
class Tuple;
class StmntVisitor;

//------------------------------------------------------------------------------

class Stmnt : public Node
{
public:

    Stmnt(location loc);

    virtual void accept(StmntVisitor* s) = 0;
};

//------------------------------------------------------------------------------

class CFStmnt : public Stmnt
{
public:


    CFStmnt(location loc, TokenType token);

    virtual void accept(StmntVisitor* s);

private:

    TokenType token_;
};

//------------------------------------------------------------------------------

class DeclStmnt : public Stmnt
{
public:

    DeclStmnt(location loc, Decl* decl);
    virtual ~DeclStmnt();

    virtual void accept(StmntVisitor* s);

    Decl* decl_;
};

//------------------------------------------------------------------------------

class IfElStmnt : public Stmnt
{
public:

    IfElStmnt(location loc, Expr* expr, Scope* ifScope, Scope* elScope);
    virtual ~IfElStmnt();

    virtual void accept(StmntVisitor* s);

    Expr* expr_;
    Scope* ifScope_;
    Scope* elScope_;
};

//------------------------------------------------------------------------------

class RepeatUntilStmnt : public Stmnt
{
public:

    RepeatUntilStmnt(location loc, Expr* expr, Scope* scope);
    virtual ~RepeatUntilStmnt();

    virtual void accept(StmntVisitor* s);

    Expr* expr_;
    Scope* scope_;
};

//------------------------------------------------------------------------------

class ScopeStmnt : public Stmnt
{
public:

    ScopeStmnt(location loc, Scope* scope);
    virtual ~ScopeStmnt();

    virtual void accept(StmntVisitor* s);

    Scope* scope_;
};

//------------------------------------------------------------------------------

class WhileStmnt : public Stmnt
{
public:

    WhileStmnt(location loc, Expr* expr, Scope* scope);
    virtual ~WhileStmnt();

    virtual void accept(StmntVisitor* s);

    Expr* expr_;
    Scope* scope_;
};

//------------------------------------------------------------------------------

class ActionStmnt : public Stmnt
{
public:

    ActionStmnt(location loc, SimdPrefix* simd);
    virtual ~ActionStmnt();

protected:

    SimdPrefix* simdPrefix_;
};

//------------------------------------------------------------------------------

class ExprStmnt : public ActionStmnt
{
public:

    ExprStmnt(location loc, SimdPrefix* simd, Expr* expr);
    virtual ~ExprStmnt();

    virtual void accept(StmntVisitor* s);

    Expr* expr_;
};

//------------------------------------------------------------------------------

class AssignStmnt : public ActionStmnt
{
public:

    AssignStmnt(location loc, SimdPrefix* simdPrefix, TokenType token, Tuple* tuple, ExprList* exprList);
    virtual ~AssignStmnt();

    virtual void accept(StmntVisitor* s);

    TokenType token_;

    Tuple* tuple_;       ///< The lvalue.
    ExprList* exprList_; ///< The rvalue.
};

//------------------------------------------------------------------------------

class StmntVisitor
{
public:
    
    StmntVisitor(Context& ctxt);

    virtual void visit(CFStmnt* s) = 0;
    virtual void visit(DeclStmnt* s) = 0;
    virtual void visit(IfElStmnt* s) = 0;
    virtual void visit(RepeatUntilStmnt* s) = 0;
    virtual void visit(ScopeStmnt* s) = 0;
    virtual void visit(WhileStmnt* s) = 0;

    // Stmnt -> ActionStmnt
    virtual void visit(AssignStmnt* s) = 0;
    virtual void visit(ExprStmnt* s) = 0;

    friend void AssignStmnt::accept(StmntVisitor* s);
    friend void IfElStmnt::accept(StmntVisitor* s);
    friend void RepeatUntilStmnt::accept(StmntVisitor* s);
    friend void ScopeStmnt::accept(StmntVisitor* s);
    friend void WhileStmnt::accept(StmntVisitor* s);

protected:
    
    Context& ctxt_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_STATEMENT_H
