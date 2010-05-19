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
class StmntVisitorBase;

//------------------------------------------------------------------------------

class Stmnt : public Node
{
public:

    Stmnt(location loc);

    virtual void accept(StmntVisitorBase* s) = 0;
};

//------------------------------------------------------------------------------

class CFStmnt : public Stmnt
{
public:


    CFStmnt(location loc, TokenType token);

    virtual void accept(StmntVisitorBase* s);

private:

    TokenType token_;
};

//------------------------------------------------------------------------------

class DeclStmnt : public Stmnt
{
public:

    DeclStmnt(location loc, Decl* decl);
    virtual ~DeclStmnt();

    virtual void accept(StmntVisitorBase* s);

protected:

    Decl* decl_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class IfElStmnt : public Stmnt
{
public:

    IfElStmnt(location loc, Expr* expr, Scope* ifScope, Scope* elScope);
    virtual ~IfElStmnt();

    virtual void accept(StmntVisitorBase* s);

protected:

    Expr* expr_;
    Scope* ifScope_;
    Scope* elScope_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class RepeatUntilStmnt : public Stmnt
{
public:

    RepeatUntilStmnt(location loc, Expr* expr, Scope* scope);
    virtual ~RepeatUntilStmnt();

    virtual void accept(StmntVisitorBase* s);

protected:

    Expr* expr_;
    Scope* scope_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class ScopeStmnt : public Stmnt
{
public:

    ScopeStmnt(location loc, Scope* scope);
    virtual ~ScopeStmnt();

    virtual void accept(StmntVisitorBase* s);

protected:

    Scope* scope_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class WhileStmnt : public Stmnt
{
public:

    WhileStmnt(location loc, Expr* expr, Scope* scope);
    virtual ~WhileStmnt();

    virtual void accept(StmntVisitorBase* s);

protected:

    Expr* expr_;
    Scope* scope_;

    template<class T> friend class StmntVisitor;
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

    virtual void accept(StmntVisitorBase* s);

protected:

    Expr* expr_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class AssignStmnt : public ActionStmnt
{
public:

    AssignStmnt(location loc, SimdPrefix* simdPrefix, TokenType token, Tuple* tuple, ExprList* exprList);
    virtual ~AssignStmnt();

    virtual void accept(StmntVisitorBase* s);

protected:

    TokenType token_;

    Tuple* tuple_;       ///< The lvalue.
    ExprList* exprList_; ///< The rvalue.

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class StmntVisitorBase
{
public:
    
    StmntVisitorBase(Context* ctxt);

    virtual void visit(CFStmnt* s) = 0;
    virtual void visit(DeclStmnt* s) = 0;
    virtual void visit(IfElStmnt* s) = 0;
    virtual void visit(RepeatUntilStmnt* s) = 0;
    virtual void visit(ScopeStmnt* s) = 0;
    virtual void visit(WhileStmnt* s) = 0;

    // Stmnt -> ActionStmnt
    virtual void visit(AssignStmnt* s) = 0;
    virtual void visit(ExprStmnt* s) = 0;

    friend void AssignStmnt::accept(StmntVisitorBase* s);
    friend void IfElStmnt::accept(StmntVisitorBase* s);
    friend void RepeatUntilStmnt::accept(StmntVisitorBase* s);
    friend void ScopeStmnt::accept(StmntVisitorBase* s);
    friend void WhileStmnt::accept(StmntVisitorBase* s);

protected:
    
    Context* ctxt_;
};

//------------------------------------------------------------------------------

template<class T> class StmntVisitor; 

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_STATEMENT_H
