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

#ifndef SWIFT_TUPEL_H
#define SWIFT_TUPEL_H

#include "fe/syntaxtree.h"
#include "fe/typelist.h"

namespace swift {

/*
 * forward declarations
 */

class Expr;

//------------------------------------------------------------------------------

/**
 * @brief This class represents a comma sperated list of Expr instances used in
 * function/method/constructor calls.
 *
 * This is actually not a Expr, but belongs to expressions
 * so it is in this file.
 */
class Tupel : public Node
{
public:

    /*
     * constructor and destructor
     */

    Tupel(TypeNode* typeNode, Tupel* next, int line = NO_LINE);
    virtual ~Tupel();

    /*
     * virtual methods
     */

    virtual bool analyze();

    /*
     * further methods
     */

    PlaceList getPlaceList();
    TypeList getTypeList() const;

private:

    /*
     * data
     */

    TypeNode* typeNode_; ///< Actual data.
    Tupel* next_;        ///< Next element in the list, 0 if this is the last one.
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TUPEL_H
