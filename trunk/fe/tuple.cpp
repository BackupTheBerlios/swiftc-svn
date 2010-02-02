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

#include "fe/tuple.h"

#include "fe/access.h"
#include "fe/expr.h"
#include "fe/exprlist.h"
#include "fe/type.h"

namespace swift {

/*
 * constructor and destructor
 */

Tuple::Tuple(TypeNode* typeNode, Tuple* next, int line /*= NO_LINE*/)
    : Node(line)
    , typeNode_(typeNode)
    , next_(next)
{}

Tuple::~Tuple()
{
    delete typeNode_;

    if (next_)
        delete next_;
}

/*
 * virtual methods
 */

bool Tuple::analyze()
{
    bool result = true;

    for (const Tuple* iter = this; iter != 0; iter = iter->next_)
    {
        Expr* expr = dynamic_cast<Expr*>(iter->typeNode_);
        if (expr)
            expr->neededAsLValue();

        result &= iter->typeNode_->analyze();
    }

    return result;
}

/*
 * further methods
 */

TypeList Tuple::getTypeList(bool simd) const
{
    TypeList result;

    for (const Tuple* iter = this; iter != 0; iter = iter->next_)
    {
        const Type* type = iter->typeNode_->getType();
        if ( simd && typeid(*type) == typeid(Simd))
            type = ((const NestedType*) type)->getInnerType();

        result.push_back(type);
    }

    return result;
}

PlaceList Tuple::getPlaceList()
{
    PlaceList result;

    for (Tuple* iter = this; iter != 0; iter = iter->next_)
        result.push_back( iter->typeNode_->getPlace() );

    return result;
}

const Tuple* Tuple::next() const
{
    return next_;
}

Tuple* Tuple::next()
{
    return next_;
}

const TypeNode* Tuple::typeNode() const
{
    return typeNode_;
}

TypeNode* Tuple::typeNode()
{
    return typeNode_;
}

std::string Tuple::toString() const
{
    std::string result;

    for (const Tuple* iter = this; iter != 0; iter = iter->next_)
        result += iter->typeNode_->toString() + ", ";

    if ( !result.empty() )
        result = result.substr( 0, result.size() - 2 );

    return result;
}

bool Tuple::moreThanOne() const
{
    return next_;
}

void Tuple::emitStoreIfApplicable(Expr* expr)
{
    Access* access = dynamic_cast<Access*>( typeNode() );
    if (access)
        access->emitStoreIfApplicable(expr);
}

void Tuple::setSimdLength(int simdLength)
{
    for (Tuple* iter = this; iter != 0; iter = iter->next_)
        iter->typeNode_->setSimdLength(simdLength);
}

void Tuple::simdAnalyze(SimdAnalysis& simdAnalysis) 
{
    for (Tuple* iter = this; iter != 0; iter = iter->next_)
        iter->typeNode_->simdAnalyze(simdAnalysis);
}

} // namespace swift
