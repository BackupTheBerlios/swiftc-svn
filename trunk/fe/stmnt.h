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

namespace llvm {
    class Function;
}

namespace swift {

//------------------------------------------------------------------------------

class Decl;
class Expr;
class TNList;
class StmntVisitorBase;

//------------------------------------------------------------------------------

class Stmnt : public Node
{
public:

    Stmnt(location loc);

    virtual void accept(StmntVisitorBase* s) = 0;
};

//------------------------------------------------------------------------------

class ErrorStmnt : public Stmnt
{
public:

    ErrorStmnt(location loc);

    virtual void accept(StmntVisitorBase* s);
};

//------------------------------------------------------------------------------

class CFStmnt : public Stmnt
{
public:

    CFStmnt(location loc, TokenType token);

    virtual void accept(StmntVisitorBase* s);

private:

    TokenType token_;

    template<class T> friend class StmntVisitor;
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

class LoopStmnt : public Stmnt
{
public:

    LoopStmnt(location loc, Scope* scope);
    virtual ~LoopStmnt();

protected:

    Scope* scope_;
};

//------------------------------------------------------------------------------

class WhileLoop : public LoopStmnt
{
public:

    WhileLoop(location loc, Scope* scope, Expr* expr);
    virtual ~WhileLoop();

    virtual void accept(StmntVisitorBase* s);

protected:

    Expr* expr_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class RepeatUntilLoop : public LoopStmnt
{
public:

    RepeatUntilLoop(location loc, Scope* scope, Expr* expr);
    virtual ~RepeatUntilLoop();

    virtual void accept(StmntVisitorBase* s);

protected:

    Expr* expr_;

    template<class T> friend class StmntVisitor;
};

//----------------------------------------------------------------------

class SimdLoop : public LoopStmnt
{
public:

    SimdLoop(location loc, Scope* scope, Expr* lExpr, Expr* rExpr);
    virtual ~SimdLoop();

    virtual void accept(StmntVisitorBase* s);

protected:

    Expr* lExpr_;
    Expr* rExpr_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class ActionStmnt : public Stmnt
{
public:

    ActionStmnt(location loc);
    virtual ~ActionStmnt();
};

//------------------------------------------------------------------------------

class ExprStmnt : public ActionStmnt
{
public:

    ExprStmnt(location loc, Expr* expr);
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

    AssignStmnt(location loc, TokenType token, TNList* tuple, TNList* exprList);
    virtual ~AssignStmnt();

    virtual void accept(StmntVisitorBase* s);

protected:

    TokenType token_;

    TNList* tuple_;    ///< The lvalues.
    TNList* exprList_; ///< The rvalues.

    enum Kind
    {
        CREATE,
        ASSIGN,
        PAIRWISE
    };

    Kind kind_;

    struct Call
    {
        enum Kind
        {
            EMPTY,
            COPY,
            USER,
            CONTAINER_COPY,
            CONTAINER_CREATE,
            GETS_INITIALIZED_BY_CALLER
        };

        Call(Kind kind, MemberFct* fct = 0)
            : kind_(kind)
            , fct_(fct)
        {}

        Kind kind_;
        MemberFct* fct_;
    };

    typedef std::vector<Call> Calls;
    Calls calls_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class StmntVisitorBase
{
public:
    
    StmntVisitorBase(Context* ctxt);
    virtual ~StmntVisitorBase() {}

    virtual void visit(ErrorStmnt* s) = 0;
    virtual void visit(CFStmnt* s) = 0;
    virtual void visit(DeclStmnt* s) = 0;
    virtual void visit(IfElStmnt* s) = 0;
    virtual void visit(RepeatUntilLoop* l) = 0;
    virtual void visit(SimdLoop* l) = 0;
    virtual void visit(WhileLoop* l) = 0;
    virtual void visit(ScopeStmnt* s) = 0;

    // Stmnt -> ActionStmnt
    virtual void visit(AssignStmnt* s) = 0;
    virtual void visit(ExprStmnt* s) = 0;

    friend void AssignStmnt::accept(StmntVisitorBase* s);
    friend void IfElStmnt::accept(StmntVisitorBase* s);
    friend void RepeatUntilLoop::accept(StmntVisitorBase* s);
    friend void ScopeStmnt::accept(StmntVisitorBase* s);
    friend void WhileLoop::accept(StmntVisitorBase* s);

    Context* getCtxt() const { return ctxt_; }

protected:
    
    Context* ctxt_;
};

//------------------------------------------------------------------------------

template<class T> class StmntVisitor; 

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_STATEMENT_H
