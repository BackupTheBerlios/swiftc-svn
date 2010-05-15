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

#include "fe/error.h"
#include "fe/sig.h"
#include "fe/stmnt.h"
#include "fe/var.h"

namespace swift {

Scope::Scope(Scope* parent)
    : parent_(parent)
{}

Scope::~Scope()
{
    for (size_t i = 0; i < stmnts_.size(); ++i)
        delete stmnts_[i];
}

Local* Scope::lookupLocalOneLevelOnly(const std::string* id)
{
    LocalMap::const_iterator iter = locals_.find(id);
    if ( iter != locals_.end() )
        return iter->second;
    else
        return 0;
}

Local* Scope::lookupLocal(const std::string* id)
{
    if ( Local* local = lookupLocalOneLevelOnly(id) )
        return local;
    else if (parent_) // try to find in parent scope
        return parent_->lookupLocal(id);

    // indicate "not found"
    return 0;
}

bool Scope::insert(Local* local)
{
    std::pair<LocalMap::iterator, bool> p 
        = locals_.insert( std::make_pair(local->id(), local) );

    if ( !p.second )
    {
        errorf(local->loc(), "there is already a local '%s' defined in this scope", local->cid());
        SWIFT_PREV_ERROR( p.first->second->loc() );

        return false;
    }

    return true;
}

void Scope::appendStmnt(Stmnt* stmnt)
{
    stmnts_.push_back(stmnt);
}

void Scope::accept(StmntVisitor* s)
{
    for (size_t i = 0; i < stmnts_.size(); ++i)
        stmnts_[i]->accept(s);
}

} // namespace swift
