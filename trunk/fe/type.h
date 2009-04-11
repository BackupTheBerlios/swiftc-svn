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
#include "fe/typelist.h"

#include "me/op.h"

namespace swift {

/*
 * forward declarations
 */

class BaseType;
class Class;
class Ptr;

//------------------------------------------------------------------------------

/** 
 * @brief Baseclass for all types in swift.
 *
 * Terminology: <br>
 * - \em inner types: int, uint, real, ... and user defined types, i.e. all inner
 *   types are \a BaseType instances 
 * - \em atmoic types: int, uint, real, ... and all ptr types, i.e. all types
 *   which can be represented with \a me::Op::Type <br>
 * - \em builtin types: int, uint, real, all ptr, array and simd types, i.e. all
 *   types where the compiler must provide the implementation <br><br>
 *
 * This combination is useful: <br>
 * \em inner \em atomic types: int, uint, real, ..., i.e all builtin types 
 * known by the \a symtab.
 */
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

    virtual bool isAtomic() const;
    virtual bool isBuiltin() const;
    virtual const BaseType* isInner() const;

    /// Checks whether this Type is the builtin bool Type
    virtual bool isBool() const;

    virtual const BaseType* getFirstBaseType() const = 0;
    virtual const Ptr* derefToInnerstPtr() const = 0;

    virtual const BaseType* unnestPtr() const = 0;

    virtual bool hasAssignCreate(const TypeList& in, 
                                 bool hasCreate, 
                                 int line) const = 0;

    virtual std::string toString() const = 0;

    /*
     * further methods
     */

    Type* constClone() const;
    const BaseType* isInnerAtomic() const;

    int& modifier();
    const int& modifier() const;

    bool isReadOnly() const;

protected:

    /*
     * data
     */

    /**
     * One of: <br>
     * - VAR <br>
     * - CONST <br>
     * - INOUT <br>
     * - CONST_PARAM<br>
     * - RETURN_VALUE <br>
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
    BaseType(int modifier, Class* _class);
    ~BaseType();

    /*
     * virtual methods
     */

    virtual BaseType* clone() const;
    virtual bool validate() const;
    virtual bool check(const Type* type) const;
    virtual me::Op::Type toMeType() const;
    virtual bool isAtomic() const;
    virtual bool isBuiltin() const;
    virtual const BaseType* isInner() const;
    virtual bool isBool() const;
    virtual me::Var* createVar(const std::string* id = 0) const;
    virtual const BaseType* getFirstBaseType() const;
    virtual const Ptr* derefToInnerstPtr() const;
    virtual const BaseType* unnestPtr() const;

    virtual bool hasAssignCreate(const TypeList& in, 
                                 bool hasCreate, 
                                 int line) const;

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
    virtual bool isBuiltin() const;
    virtual const BaseType* isInner() const;

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
    virtual bool isAtomic() const;

    virtual me::Op::Type toMeType() const;
    virtual me::Var* createVar(const std::string* id = 0) const;
    virtual const BaseType* getFirstBaseType() const;
    virtual const Ptr* derefToInnerstPtr() const;
    virtual bool hasAssignCreate(const TypeList& in, 
                                 bool hasCreate, 
                                 int line) const;

    virtual const BaseType* unnestPtr() const;
    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPE_H
