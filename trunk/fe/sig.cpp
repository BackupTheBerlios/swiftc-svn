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

#include "fe/sig.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/context.h"
#include "fe/class.h"
#include "fe/stmnt.h"
#include "fe/type.h"
#include "fe/var.h"

namespace swift {

//------------------------------------------------------------------------------

Sig::~Sig()
{
    for (size_t i = 0; i < in_.size(); ++i)
        delete in_[i];

    for (size_t i = 0; i < out_.size(); ++i)
        delete out_[i];
}

bool Sig::checkIn(Module* module, const TypeList& inTypes) const
{
    return inTypes_.check(module, inTypes);
}

bool Sig::checkOut(Module* module, const TypeList& outTypes) const
{
    return outTypes_.check(module, outTypes);
}

InOut* Sig::lookupInOut(const std::string* id) const
{
    // is it a parameter?
    for (size_t i = 0; i < in_.size(); ++i)
    {
        if ( *in_[i]->id() == *id)
            return in_[i];
    }

    // is it a return value?
    for (size_t i = 0; i < out_.size(); ++i)
    {
        if ( *out_[i]->id() == *id)
            return out_[i];
    }

    // -> not found, so return 0
    return 0;
}

void Sig::buildTypeLists()
{
    inTypes_.reserve( in_.size() );
    outTypes_.reserve( out_.size() );

    for (size_t i = 0; i < in_.size(); ++i)
        inTypes_.push_back( in_[i]->getType() );

    for (size_t i = 0; i < out_.size(); ++i)
        outTypes_.push_back( out_[i]->getType() );
}

//------------------------------------------------------------------------------

} // namespace swift
