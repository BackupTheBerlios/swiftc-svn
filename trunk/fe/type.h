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

class BaseType;
class Class;
class Ptr;

//------------------------------------------------------------------------------

class Type : public Node
{
public:

    /*
     * constructor and destructor
     */

    Type(int modifier, int line = NO_LINE);
    virtual ~Type() {}

    /*
     * virtual methods
     */

    /// Creates a copy of this Type
    virtual Type* clone() const = 0;

    /// Checks whether a given type exists
    virtual bool validate() const = 0;

    virtual bool check(const Type* type) const = 0;

    virtual me::Op::Type toMeType() const = 0;

    virtual me::Var* createVar(const std::string* id = 0) const = 0;

    /**
     * Checks whether this is an atomic builtin type.
     *
     * @return True if this is a built in type, false otherwise
     */
    virtual bool isAtomic() const;

    /// Checks whether this Type is the builtin bool Type
    virtual bool isBool() const;

    virtual const BaseType* getFirstBaseType() const = 0;
    virtual const Ptr* derefToInnerstPtr() const = 0;

    virtual std::string toString() const = 0;

protected:

    /*
     * data
     */

    int modifier_;
};

//------------------------------------------------------------------------------

class BaseType : public Type
{
public:

    /*
     * constructor and destructor
     */
    
    BaseType(int modifier, std::string* id, int line = NO_LINE);
    ~BaseType();

    /*
     * virtual methods
     */

    virtual BaseType* clone() const;
    virtual bool validate() const;
    virtual bool check(const Type* type) const;
    virtual me::Op::Type toMeType() const;
    virtual bool isAtomic() const;
    virtual bool isBool() const;
    virtual me::Var* createVar(const std::string* id = 0) const;
    virtual const BaseType* getFirstBaseType() const;
    virtual const Ptr* derefToInnerstPtr() const;
    virtual std::string toString() const;

    /*
     * further methods
     */

    Class* lookupClass() const;
    const std::string* getId() const;

    /*
     * static methods
     */

    static bool isBuiltin(const std::string* id);
    static void initTypeMap();
    static void destroyTypeMap();

private:

    /*
     * data
     */

    std::string* id_;
    bool builtin_;

    typedef std::map<std::string, me::Op::Type> TypeMap;
    static TypeMap* typeMap_; 
};

//------------------------------------------------------------------------------

class Container : public Type
{
public:

    /*
     * constructor and destructor
     */
     
    Container(int modifier, Type* innerType, int line = NO_LINE);
    ~Container();

    /*
     * virtual methods
     */

    virtual bool validate() const;

    /*
     * further methods
     */

    Type* getInnerType();

protected:

    /*
     * data
     */

    Type* innerType_;
};


//------------------------------------------------------------------------------

class Ptr : public Container
{
public:

    /*
     * constructor and destructor
     */

    Ptr(int modifier, Type* innerType, int line = NO_LINE);

    /* 
     * virtual methods
     */

    virtual Ptr* clone() const;
    virtual bool check(const Type* type) const;
    virtual me::Op::Type toMeType() const;
    virtual me::Var* createVar(const std::string* id = 0) const;
    virtual const BaseType* getFirstBaseType() const;
    virtual const Ptr* derefToInnerstPtr() const;
    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPE_H
