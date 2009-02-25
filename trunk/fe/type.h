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

#ifndef SWIFT_TYPE_H
#define SWIFT_TYPE_H

#include "fe/syntaxtree.h"

#include "me/op.h"

namespace swift {

/*
 * forward declarations
 */

struct Class;

//------------------------------------------------------------------------------

struct BaseType : public Node
{
    std::string* id_;
    bool builtin_;

    /*
     * constructors and destructor
     */

    BaseType(std::string* id, int line = NO_LINE);
    ~BaseType();

    /*
     * further methods
     */
    BaseType* clone() const;
    me::Op::Type toMeType() const;

    /**
     * Checks whether this is a builtin type.
     *
     * @return True if this is a builtin type, false otherwise
     */
    bool isBuiltin() const;

    Class* lookupClass() const;
    me::Var* createVar(std::string* id = 0) const;

    typedef std::map<std::string, me::Op::Type> TypeMap;
    static TypeMap* typeMap_; 
};

//------------------------------------------------------------------------------

struct Type : public Node
{
    BaseType*   baseType_;
    int         pointerCount_;

    /*
     * constructor and destructor
     */

    Type(BaseType* baseType, int pointerCount, int line = NO_LINE);
    virtual ~Type();

    /*
     * further methods
     */

    /// Creates a copy of this Type
    Type* clone() const;

    /**
     * Check Type \p t1 and Type \p t2 for consistency
     *
     * @param t1 first type to be checked
     * @param t2 second type to be checked
    */
    static bool check(Type* t1, Type* t2);

    /// Checks whether a given type exists
    bool validate() const;

    /// Checks whether this Type is the builtin bool Type
    bool isBool() const;

    /**
     * Checks whether this is a builtin type.
     *
     * @return True if this is a built in type, false otherwise
     */
    bool isBuiltin() const;

    std::string toString() const;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPE_H
