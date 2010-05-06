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
#include "fe/signature.h"
#include "fe/var.h"

namespace swift {

/*
 * constructor and destructor
 */

Scope::Scope(Scope* parent)
    : parent_(parent)
{}

Scope::~Scope()
{
    // delete each child scope
    for (size_t i = 0; i < childScopes_.size(); ++i)
        delete childScopes_[i];
}

/*
 * further methods
 */

Local* Scope::lookupLocal(const std::string* id)
{
    LocalMap::const_iterator iter = locals_.find(id);
    if ( iter != locals_.end() )
        return iter->second;
    else
    {
        // try to find in parent scope
        if (parent_)
            return parent_->lookupLocal(id);
        else
            return 0;
    }
}

bool Scope::insert(Local* local, const Signature* sig)
{
    std::pair<LocalMap::iterator, bool> p 
        = locals_.insert( std::make_pair(local->id_, local) );

    if ( !p.second )
    {
        errorf(local->loc_, "there is already a local '%s' defined in this scope in line %i",
            local->id_->c_str(), p.first->second->loc_.begin.line);

        return false;
    }

    const Param* found = sig->findParam(local->id_);
    if (found)
    {
        errorf(local->loc_, "local '%s' shadows a parameter", local->id_->c_str());
        return false;
    }

    return true;
}

} // namespace swift
