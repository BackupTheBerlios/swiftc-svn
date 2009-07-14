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

#include "fe/simdprefix.h"

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/type.h"

namespace swift {

/*
 * constructor and destructor
 */

SimdPrefix::SimdPrefix(Expr* leftExpr, Expr* rightExpr, int line)
    : Node(line)
    , leftExpr_(leftExpr)
    , rightExpr_(rightExpr)
{
}

SimdPrefix::~SimdPrefix()
{
    delete leftExpr_;
    delete rightExpr_;
}

/*
 * further methods
 */

bool SimdPrefix::analyze()
{
    bool result = true;

    if (leftExpr_)
        result &= leftExpr_->analyze();

    if (rightExpr_)
        result &= rightExpr_->analyze();

    if (!result)
        return false;

    if ( leftExpr_ && !leftExpr_->getType()->isIndex() )
    {
        errorf(line_, "type of the left expression must be of type 'index'");
        return false;
    }

    if ( rightExpr_ && !rightExpr_->getType()->isIndex() )
    {
        errorf(line_, "type of the right expression must be of type 'index'");
        return false;
    }

    // TODO get bounds for 0 left/rigt expressions

    genSSA();

    return true;
}

void SimdPrefix::genSSA()
{
    // TODO
}

/*
 * virtual methods
 */

std::string SimdPrefix::toString() const
{
    return "TODO";
}

} // namespace swift
