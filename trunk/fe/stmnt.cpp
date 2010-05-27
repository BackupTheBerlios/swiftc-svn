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

#include "fe/stmnt.h"

#include <sstream>
#include <typeinfo>

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/scope.h"
#include "fe/simdprefix.h"
#include "fe/sig.h"
#include "fe/type.h"
#include "fe/typenode.h"
#include "fe/var.h"

namespace swift {

//------------------------------------------------------------------------------

Stmnt::Stmnt(location loc)
    : Node(loc)
{}

//------------------------------------------------------------------------------

DeclStmnt::DeclStmnt(location loc, Decl* decl)
    : Stmnt(loc)
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

ActionStmnt::ActionStmnt(location loc, SimdPrefix* simdPrefix)
    : Stmnt(loc)
    , simdPrefix_(simdPrefix)
{}

ActionStmnt::~ActionStmnt()
{
    delete simdPrefix_;
}

//------------------------------------------------------------------------------

ErrorStmnt::ErrorStmnt(location loc)
    : Stmnt(loc)
{}

void ErrorStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

ExprStmnt::ExprStmnt(location loc, SimdPrefix* simdPrefix, Expr* expr)
    : ActionStmnt(loc, simdPrefix)
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
        location loc,
        SimdPrefix* simdPrefix, 
        TokenType token,
        Tuple* tuple, 
        ExprList* exprList)
    : ActionStmnt(loc, simdPrefix)
    , token_(token)
    , tuple_(tuple)
    , exprList_(exprList)
{}

AssignStmnt::~AssignStmnt()
{
    delete tuple_;
    delete exprList_;
}

void AssignStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

WhileStmnt::WhileStmnt(location loc, Expr* expr, Scope* scope)
    : Stmnt(loc)
    , expr_(expr)
    , scope_(scope)
{}

WhileStmnt::~WhileStmnt()
{
    delete expr_;
    delete scope_;
}

void WhileStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

RepeatUntilStmnt::RepeatUntilStmnt(location loc, Expr* expr, Scope* scope)
    : Stmnt(loc)
    , expr_(expr)
    , scope_(scope)
{}

RepeatUntilStmnt::~RepeatUntilStmnt()
{
    delete expr_;
    delete scope_;
}

void RepeatUntilStmnt::accept(StmntVisitorBase* s)
{
    s->visit(this);
}

//------------------------------------------------------------------------------

ScopeStmnt::ScopeStmnt(location loc, Scope* scope)
    : Stmnt(loc)
    , scope_(scope)
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

IfElStmnt::IfElStmnt(location loc, Expr* expr, Scope* ifScope, Scope* elScope)
    : Stmnt(loc)
    , expr_(expr)
    , ifScope_(ifScope)
    , elScope_(elScope)
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

CFStmnt::CFStmnt(location loc, TokenType token)
    : Stmnt(loc)
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
