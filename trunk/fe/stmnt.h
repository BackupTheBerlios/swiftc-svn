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
#include "fe/fct.h"
#include "fe/node.h"
#include "fe/typelist.h"

namespace llvm {
    class BasicBlock;
    class Function;
}

namespace swift {

//------------------------------------------------------------------------------

class Decl;
class Expr;
class Local;
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

    llvm::BasicBlock* getLoopBB() const { return loopBB_; }
    llvm::BasicBlock* getOutBB() const { return outBB_; }

protected:

    Scope* scope_;
    llvm::BasicBlock* loopBB_;
    llvm::BasicBlock* outBB_;
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

//------------------------------------------------------------------------------

class LoopWithHeader : public LoopStmnt
{
public:

    LoopWithHeader(location loc, Scope* scope);

protected:

    llvm::BasicBlock* headerBB_;

    template<class T> friend class StmntVisitor;
};

//------------------------------------------------------------------------------

class WhileLoop : public LoopWithHeader
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

class SimdLoop : public LoopWithHeader
{
public:

    SimdLoop(location loc, Scope* scope, std::string* id, Expr* lExpr, Expr* rExpr);
    virtual ~SimdLoop();

    virtual void accept(StmntVisitorBase* s);

protected:

    std::string* id_;
    Expr* lExpr_;
    Expr* rExpr_;
    Local* index_;

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

    AssignStmnt(location loc, std::string* id, TNList* tuple, TNList* exprList);
    virtual ~AssignStmnt();

    virtual void accept(StmntVisitorBase* s);

protected:

    std::string* id_;

    TNList* tuple_;    ///< The lvalues.
    TNList* exprList_; ///< The rvalues.

    enum Kind
    {
        SINGLE,
        PAIRWISE
    };

    Kind kind_;

    std::vector<AssignCreate> acs_;

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


#define SWIFT_ENTER_LOOP \
    LoopStmnt* oldLoop = ctxt_->currentLoop_; \
    ctxt_->currentLoop_ = l; \
    l->scope_->accept(this); \
    ctxt_->currentLoop_ = oldLoop;

//------------------------------------------------------------------------------

template<class T> class StmntVisitor; 

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_STATEMENT_H
