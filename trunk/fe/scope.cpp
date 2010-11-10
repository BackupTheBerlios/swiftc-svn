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

#include "fe/scope.h"

#include "fe/context.h"
#include "fe/error.h"
#include "fe/sig.h"
#include "fe/stmnt.h"
#include "fe/var.h"

namespace swift {

Scope::Scope(const Location& loc, Node* parent, Scope* pScope)
    : Node(loc, parent)
    , pScope_(pScope)
{}

Scope::~Scope()
{
    for (size_t i = 0; i < stmnts_.size(); ++i)
        delete stmnts_[i];
}

Var* Scope::lookupVarOneLevelOnly(const std::string* id)
{
    VarMap::const_iterator iter = vars_.find(id);
    if ( iter != vars_.end() )
        return iter->second;
    else
        return 0;
}

Var* Scope::lookupVar(const std::string* id)
{
    if ( Var* var = lookupVarOneLevelOnly(id) )
        return var;
    else if ( pScope_ ) // try to find in parent scope
        return pScope_->lookupVar(id);

    // indicate "not found"
    return 0;
}

void Scope::insert(Var* var)
{
    std::pair<VarMap::iterator, bool> p 
        = vars_.insert( std::make_pair(var->id(), var) );

    swiftAssert(p.second, "already inserted");
}

void Scope::appendStmnt(Stmnt* stmnt)
{
    stmnts_.push_back(stmnt);
}

void Scope::accept(StmntVisitorBase* s)
{
    s->getCtxt()->enterScope(this);

    for (size_t i = 0; i < stmnts_.size(); ++i)
        stmnts_[i]->accept(s);

    s->getCtxt()->leaveScope();
}

bool Scope::isEmpty() const
{
    return stmnts_.empty();
}

} // namespace swift
