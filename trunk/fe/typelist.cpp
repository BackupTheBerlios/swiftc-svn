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

#include "fe/typelist.h"

#include "utils/stringhelper.h"

#include "fe/type.h"

namespace swift {

TypeList::TypeList()
    : std::vector<const Type*>()
{}

TypeList::TypeList(size_t size)
    : std::vector<const Type*>(size)
{}

TypeList::TypeList(const_iterator begin, const_iterator end)
    : std::vector<const Type*>(begin, end)
{}

std::string TypeList::toString() const
{
    std::string result;

    for (size_t i = 0; i < size(); ++i)
    {
        if ( (*this)[i] )
            result += (*this)[i]->toString() + ", ";
        else
            result += "void, ";
    }

    if ( !result.empty() )
        result = result.substr(0, result.size() - 2);

    return result;
}

bool TypeList::check(Module* m, const TypeList& t) const
{
    // if the sizes do not match the type lists are obviously different
    if ( size() != t.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    // check each type
    for (size_t i = 0; result && i < size(); ++i)
        result &= (*this)[i]->check(t[i], m);

    return result;
}

} // namespace swift
