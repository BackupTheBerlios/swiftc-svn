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

#include "var.h"

#include "fe/type.h"

#include "me/functab.h"

namespace swift {

/*
 * constructor and destructor
 */

Var::Var(Type* type, me::Var* var, std::string* id, int line /*= NO_LINE*/)
    : Symbol(id, 0, line) // Vars (Params or Locals) never have parents
    , type_(type)
    , meVar_(var)
{}

Var::~Var()
{
    delete type_;
}

/*
 * further methods
 */

const Type* Var::getType() const
{
    return type_;
}

me::Var* Var::getMeVar()
{
    return meVar_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Local::Local(Type* type, me::Var* var, std::string* id, int line /*= NO_LINE*/)
    : Var(type, var, id, line)
{}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Param::Param(Kind kind, Type* type, std::string* id, int line /*= NO_LINE*/)
    : Var(type, 0, id, line)
    , kind_(kind)
{}

/*
 * further methods
 */

Param::Kind Param::getKind() const
{
    return kind_;
}

bool Param::check(const Param* param1, const Param* param2)
{
    // first check whether kind_ fits
    if (param1->kind_ != param2->kind_)
        return false;

    // check whether type fits
    if ( param1->type_->check(param2->type_) )
        return true;

    // else
    return false;
}

bool Param::validateAndCreateVar()
{
    meVar_ = type_->createVar(id_);

    return type_->validate();
}

} // namespace swift
