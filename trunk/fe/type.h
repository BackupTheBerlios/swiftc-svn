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

/*
 * forward declarations
 */

namespace me {
    class Struct;
    class StructOffset;
}

namespace swift {

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
 * - \em atomic types: int, uint, real, ... and all ptr types, i.e. all types
 *   which can be represented with \a me::Op::Type <br>
 * - \em builtin types: int, uint, real, all ptr, array and simd types, i.e. all
 *   types where the compiler must provide the implementation <br><br>
 *
 * Theses combinations are useful: <br>
 * - \em inner \em atomic types: int, uint, real, ..., i.e all builtin types 
 * known by the \a symtab
 * - \em internal \em atomic types: types which are internally represented by
 *   an atomic value, i.e. all atomic types and other types which are internally
 *   a reference
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

    /// Is this a type which can be represented as an atomic value?
    virtual bool isAtomic() const = 0;

    /// Is this a type which is built into swift?
    virtual bool isBuiltin() const = 0;

    virtual bool isInternalAtomic() const = 0;

    virtual bool isActuallyPtr() const = 0;

    /**
     * @brief Returns 'this' if this a a BaseType.
     *
     * @return 'this' or 0.
     */
    virtual const BaseType* isInner() const = 0;

    /// Checks whether this Type is the builtin bool type.
    virtual bool isBool() const;

    virtual bool isIndex() const;

    virtual bool isInt() const;

    virtual size_t sizeOf() const = 0;

    virtual me::Reg* derefToInnerstPtr(me::Var* var) const = 0;

    virtual const BaseType* unnestPtr() const = 0;
    virtual const Ptr* unnestInnerstPtr() const = 0;

    virtual bool hasAssignCreate(const TypeList& in, 
                                 bool hasCreate, 
                                 int line) const = 0;

    virtual std::string toString() const = 0;

    /*
     * further methods
     */

    Type* constClone() const;
    Type* varClone() const;

    const BaseType* isInnerAtomic() const;
    bool isNonInnerBuiltin() const;

    int& modifier();
    const int& modifier() const;

    bool isReadOnly() const;


protected:

    me::Reg* loadPtr(me::Var* var) const;

    /*
     * data
     */

    /**
     * Modifier of this type.
     *
     * One of: <br>
     * - VAR: an ordinary variable with read and write access <br>
     * - CONST: a constant with read-only access <br>
     * - REF: an internal pointer to location with read and write access <br>
     * - CONST_REF: an internal pointer to a location with read and write access
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
    BaseType(int modifier, const Class* _class);
    ~BaseType();

    /*
     * virtual methods
     */

    virtual BaseType* clone() const;
    virtual bool validate() const;
    virtual bool check(const Type* type) const;
    virtual me::Op::Type toMeType() const;
    virtual bool isAtomic() const;
    virtual bool isInternalAtomic() const;
    virtual bool isBuiltin() const;
    virtual const BaseType* isInner() const;
    virtual bool isBool() const;
    virtual bool isIndex() const;
    virtual bool isInt() const;
    virtual bool isActuallyPtr() const;
    virtual size_t sizeOf() const;
    virtual me::Var* createVar(const std::string* id = 0) const;
    virtual me::Reg* derefToInnerstPtr(me::Var* var) const;
    virtual const BaseType* unnestPtr() const;
    virtual const Ptr* unnestInnerstPtr() const;

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

class NestedType : public Type
{
public:

    /*
     * constructor and destructor
     */
     
    NestedType(int modifier, Type* innerType, int line = NO_LINE);
    ~NestedType();

    /*
     * virtual methods
     */

    virtual bool validate() const;
    virtual bool isBuiltin() const;
    virtual const BaseType* isInner() const;
    virtual bool check(const Type* type) const;

    /*
     * further methods
     */

    Type* getInnerType();
    const Type* getInnerType() const;

protected:

    /*
     * data
     */

    Type* innerType_;
};

//------------------------------------------------------------------------------

class Ptr : public NestedType
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
    virtual bool isAtomic() const;
    virtual bool isInternalAtomic() const;
    virtual bool isActuallyPtr() const;
    virtual size_t sizeOf() const;
    virtual const BaseType* unnestPtr() const;
    virtual const Ptr* unnestInnerstPtr() const;

    virtual me::Op::Type toMeType() const;
    virtual me::Var* createVar(const std::string* id = 0) const;
    virtual me::Reg* derefToInnerstPtr(me::Var* var) const;
    virtual bool hasAssignCreate(const TypeList& in, 
                                 bool hasCreate, 
                                 int line) const;

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

class Container : public NestedType
{
public:

    /*
     * constructor
     */
     
    Container(int modifier, Type* innerType, int line = NO_LINE);

    /*
     * virtual methods
     */

    virtual bool isAtomic() const;
    virtual bool isInternalAtomic() const;
    virtual bool isActuallyPtr() const;
    virtual size_t sizeOf() const;

    virtual me::Op::Type toMeType() const;
    virtual me::Var* createVar(const std::string* id = 0) const;
    virtual me::Reg* derefToInnerstPtr(me::Var* var) const;
    virtual bool hasAssignCreate(const TypeList& in, 
                                 bool hasCreate, 
                                 int line) const;

    virtual const BaseType* unnestPtr() const;
    virtual const Ptr* unnestInnerstPtr() const;
    virtual std::string toString() const;
    virtual std::string containerStr() const = 0;

    /*
     * static methods
     */

    static void initMeContainer();
    static me::Struct* getMeStruct();
    static me::StructOffset* createContainerPtrOffset();
    static me::StructOffset* createContainerSizeOffset();
    static size_t getContainerSize();

protected:

    /*
     * data
     */

    static me::Struct* meContainer_;
    static me::Member* meContainerPtr_;
    static me::Member* meContainerSize_;
};

//------------------------------------------------------------------------------

class Array : public Container
{
public:

    /*
     * constructor and destructor
     */

    Array(int modifier, Type* innerType, int line = NO_LINE);

    /* 
     * virtual methods
     */

    virtual Array* clone() const;
    virtual std::string containerStr() const;
};

//------------------------------------------------------------------------------

class Simd : public Container
{
public:

    /*
     * constructor and destructor
     */

    Simd(int modifier, Type* innerType, int line = NO_LINE);

    /* 
     * virtual methods
     */

    virtual Simd* clone() const;
    virtual std::string containerStr() const;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPE_H
