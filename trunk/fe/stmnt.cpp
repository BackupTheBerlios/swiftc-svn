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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNUssi
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "fe/stmnt.h"

#include <sstream>
#include <typeinfo>

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/tnlist.h"
#include "fe/scope.h"
#include "fe/sig.h"
#include "fe/type.h"
#include "fe/typenode.h"
#include "fe/var.h"


namespace swift {

//------------------------------------------------------------------------------

Stmnt::Stmnt(const Location& loc, Scope* parent)
    : Node(loc, parent)
{}

//------------------------------------------------------------------------------

DeclStmnt::DeclStmnt(const Location& loc, Scope* parent, Decl* decl)
    : Stmnt(loc, parent)
    , decl_(decl)
{}

DeclStmnt::~DeclStmnt()
{
    delete decl_;
}

void DeclStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

ErrorStmnt::ErrorStmnt(const Location& loc, Scope* parent)
    : Stmnt(loc, parent)
{}

void ErrorStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

ExprStmnt::ExprStmnt(const Location& loc, Scope* parent, Expr* expr)
    : Stmnt(loc, parent)
    , expr_(expr)
{}

ExprStmnt::~ExprStmnt()
{
    delete expr_;
}

void ExprStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

AssignStmnt::AssignStmnt(
        const Location& loc,
        Scope* parent,
        std::string* id,
        TNList* tuple, 
        TNList* exprList)
    : Stmnt(loc, parent)
    , id_(id)
    , tuple_(tuple)
    , exprList_(exprList)
{}

AssignStmnt::~AssignStmnt()
{
    delete id_;
    delete tuple_;
    delete exprList_;
}

void AssignStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//----------------------------------------------------------------------

LoopStmnt::LoopStmnt(const Location& loc, Scope* parent)
    : Stmnt(loc, parent)
    , scope_( new Scope(loc, this, parent) )
{}

LoopStmnt::~LoopStmnt()
{
    delete scope_;
}

//------------------------------------------------------------------------------

WhileLoop::WhileLoop(const Location& loc, Scope* parent, Expr* expr)
    : LoopStmnt(loc, parent)
    , expr_(expr)
{}

WhileLoop::~WhileLoop()
{
    delete expr_;
}

void WhileLoop::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

RepeatUntilLoop::RepeatUntilLoop(const Location& loc, Scope* parent, Expr* expr)
    : LoopStmnt(loc, parent)
    , expr_(expr)
{}

RepeatUntilLoop::~RepeatUntilLoop()
{
    delete expr_;
}

void RepeatUntilLoop::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

SimdLoop::SimdLoop(const Location& loc, Scope* parent, std::string* id, Expr* lExpr, Expr* rExpr)
    : LoopStmnt(loc, parent)
    , id_(id)
    , lExpr_(lExpr)
    , rExpr_(rExpr)
{}

SimdLoop::~SimdLoop()
{
    delete id_;
    delete lExpr_;
    delete rExpr_;
}

void SimdLoop::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

ScopeStmnt::ScopeStmnt(const Location& loc, Scope* parent)
    : Stmnt(loc, parent)
    , scope_( new Scope(loc, this, parent) )
{}

ScopeStmnt::~ScopeStmnt()
{
    delete scope_;
}

void ScopeStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

IfElStmnt::IfElStmnt(const Location& loc, Scope* parent, Expr* expr)
    : Stmnt(loc, parent)
    , expr_(expr)
    , ifScope_( new Scope(loc, this, parent) )
    , elScope_( new Scope(loc, this, parent) )
{}

IfElStmnt::~IfElStmnt()
{
    delete expr_;
    delete ifScope_;
    delete elScope_;
}

void IfElStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

CFStmnt::CFStmnt(const Location& loc, Scope* parent, TokenType token)
    : Stmnt(loc, parent)
    , token_(token)
{}

void CFStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

StmntVisitorBase::StmntVisitorBase(Context* ctxt)
    : ctxt_(ctxt)
{}

//------------------------------------------------------------------------------

} // namespace swift
