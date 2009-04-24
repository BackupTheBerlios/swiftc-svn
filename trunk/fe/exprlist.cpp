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

#include "fe/exprlist.h"

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/functioncall.h"

namespace swift {

/*
 * constructor and destructor
 */

ExprList::ExprList(int modifier, Expr* expr, ExprList* next, int line /*= NO_LINE*/)
    : Node(line)
    , modifier_(modifier)
    , expr_(expr)
    , next_(next)
{}

ExprList::~ExprList()
{
    delete expr_;
    if (next_)
        delete next_;
}

/*
 * virtual methods
 */

bool ExprList::analyze()
{
    bool result = true;

    // for each expr in this list
    for (ExprList* iter = this; iter != 0; iter = iter->next_)
        result &= iter->expr_->analyze();

    return result;
}

/*
 * further methods
 */

TypeList ExprList::getTypeList() const
{
    TypeList result;

    for (const ExprList* iter = this; iter != 0; iter = iter->next_)
        result.push_back( iter->expr_->getType() );

    return result;
}

PlaceList ExprList::getPlaceList()
{
    PlaceList result;

    for (ExprList* iter = this; iter != 0; iter = iter->next_)
        result.push_back( iter->expr_->getPlace() );

    return result;
}

FunctionCall* ExprList::getFunctionCall()
{
    swiftAssert(!next_, "more than one item in the ExprList");
    return dynamic_cast<FunctionCall*>(expr_);
}

std::string ExprList::toString() const
{
    std::string result;

    for (const ExprList* iter = this; iter != 0; iter = iter->next_)
        result += modifier_ ? "inout " : "" + iter->expr_->toString() + ", ";

    if ( !result.empty() )
        result = result.substr( 0, result.size() - 2 );

    return result;
}

bool ExprList::moreThanOne() const
{
    return next_;
}

} // namespace swift
