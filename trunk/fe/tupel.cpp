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

#include "fe/tupel.h"

#include "fe/expr.h"

namespace swift {

/*
 * constructor and destructor
 */

Tupel::Tupel(TypeNode* typeNode, Tupel* next, int line /*= NO_LINE*/)
    : Node(line)
    , typeNode_(typeNode)
    , next_(next)
{}

Tupel::~Tupel()
{
    delete typeNode_;

    if (next_)
        delete next_;
}

/*
 * virtual methods
 */

bool Tupel::analyze()
{
    Expr* expr = dynamic_cast<Expr*>(typeNode_);
    if (expr)
        expr->neededAsLValue();

    bool result = typeNode_->analyze();

    if (next_)
        result &= next_->analyze();

    return result;
}

/*
 * further methods
 */

TypeList Tupel::getTypeList() const
{
    TypeList result;

    for (const Tupel* iter = this; iter != 0; iter = iter->next_)
        result.push_back( iter->typeNode_->getType() );

    return result;
}

PlaceList Tupel::getPlaceList()
{
    PlaceList result;

    for (Tupel* iter = this; iter != 0; iter = iter->next_)
        result.push_back( iter->typeNode_->getPlace() );

    return result;
}

const Tupel* Tupel::next() const
{
    return next_;
}

const TypeNode* Tupel::getTypeNode() const
{
    return typeNode_;
}

std::string Tupel::toString() const
{
    std::string result;

    for (const Tupel* iter = this; iter != 0; iter = iter->next_)
        result += iter->typeNode_->toString() + ", ";

    if ( !result.empty() )
        result = result.substr( 0, result.size() - 2 );

    return result;
}

} // namespace swift
