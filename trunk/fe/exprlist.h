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

#ifndef SWIFT_EXPR_LIST_H
#define SWIFT_EXPR_LIST_H

#include <vector>

#include "fe/syntaxtree.h"
#include "fe/typelist.h"

namespace swift {

/*
 * forward declarations
 */

class Expr;
class FunctionCall;

//------------------------------------------------------------------------------

/**
 * @brief This is a list of expresoins.
 *
 * This comma sperated list of \a Expr instances used in 
 * function/method/contructor calls.
 */
class ExprList : public Node
{
public:

    /*
     * constructor and destructor
     */

    ExprList(int modifier, Expr* expr, ExprList* next, int line = NO_LINE);
    virtual ~ExprList();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const;

    /*
     * further methods
     */

    TypeList getTypeList(bool simd) const;
    PlaceList getPlaceList();
    int getModifier() const;
    FunctionCall* getFunctionCall();
    bool moreThanOne() const;
    Expr* getExpr();
    const Expr* getExpr() const;
    ExprList* next();
    const ExprList* next() const;
    void setSimd();

private:

    /*
     * data
     */

    int modifier_;   ///< \a INOUT or 0 if no modifier is used.
    Expr* expr_;     ///< the Expr owned by this instance.
    ExprList* next_; ///< next element in the list, 0 if this is the last one.
};

} // namespace swift

#endif // SWIFT_EXPR_LIST_H
