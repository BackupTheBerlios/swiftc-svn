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

#include "fe/decl.h"

#include "fe/symtab.h"
#include "fe/type.h"

namespace swift {

/*
 * constructor and destructor
 */

Decl::Decl(Type* type, std::string* id, int line /*= NO_LINE*/)
    : TypeNode(type, line)
    , id_(id)
    , local_(0) // This will be created in analyze
{}

Decl::~Decl()
{
    delete local_;
}

/*
 * further methods
 */

me::Op* Decl::getPlace()
{
    return local_->getMeVar();
}

bool Decl::analyze()
{
    // check whether this type exists
    bool result = type_->validate();

    // insert the local in every case otherwise memory leaks can occur
    local_ = symtab->createNewLocal(type_, id_, line_);

    return result;
}


} // namespace swift
