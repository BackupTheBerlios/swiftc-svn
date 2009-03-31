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

#ifndef SWIFT_METHOD_H
#define SWIFT_METHOD_H

#include <map>
#include <string>

#include "fe/class.h"

namespace swift {

// forward declaration
class Param;
class Signature;
class Scope;

//------------------------------------------------------------------------------

/**
 * This class represents a Method of a Class. 
 */
struct Method : public ClassMember
{
    int methodQualifier_;   ///< Either READER, WRITER, ROUTINE, CREATE or OPERATOR.
    Statement* statements_; ///< The statements_ inside this Proc.
    Scope* rootScope_;      ///< The root Scope where vars of this Proc are stored.
    Signature* sig_;        ///< The signature of this Method.

    /*
     * constructor and destructor
     */

    Method(int methodQualifier, std::string* id, Symbol* parent, int line = NO_LINE);
    virtual ~Method();

    /*
     * virtual methods
     */

    virtual bool analyze();

    /*
     * further methods
     */

    bool isTrivial() const;
};

} // namespace swift

#endif // SWIFT_METHOD_H
