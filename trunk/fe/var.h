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

#ifndef SWIFT_VAR_H
#define SWIFT_VAR_H

#include <string>

#include "fe/syntaxtree.h"

#include "me/functab.h"

namespace me {
    struct Var;
}

namespace swift {

// forward declaration
struct Type;

//------------------------------------------------------------------------------

/**
 * This class is the base for a Local or a Param. It is the return value
 * for symtab lookups.
 */
class Var : public Symbol
{
public:

    /*
     * constructor and destructor
     */

    Var(Type* type, me::Var* var, std::string* id, int line = NO_LINE);
    virtual ~Var();

    /*
     * further methods
     */

    const Type* getType() const;
    me::Var* getMeVar();

protected:

    /*
     * data
     */

    Type* type_;
    me::Var* meVar_;
};

//------------------------------------------------------------------------------

/**
 * This class represents either an ordinary local variable used by the
 * programmer or a compiler generated variable used to store a temporary value.
 */
class Local : public Var
{
public:

    /*
     * constructor
     */

    Local(Type* type, me::Var* var, std::string* id, int line = NO_LINE);
};

//------------------------------------------------------------------------------

/**
 * This class abstracts an parameter of a method, routine etc. 
 *
 * It knows its Kind and Type. Optionally it may know its identifier. So when
 * the Parser sees a parameter a Param with \a id_ will be created. If just a
 * Param is needed to check whether a signature fits a Param without \a id_ can
 * be used.
 */
class Param : public Var
{
public:

    /*
     * constructor
     */

    Param(Type* type, std::string* id, int line = NO_LINE);

    /*
     * further methods
     */

    /// Check whether the type of both Param objects fit.
    static bool check(const Param* param1, const Param* param2);

    /// Check whether this Param has a correct Type.
    bool validateAndCreateVar(); 

    int getModifier() const;
};

} // namespace swift

#endif // SWIFT_VAR_H
