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

#include "fe/auto.h"
#include "fe/location.hh"
#include "fe/node.h"
#include "fe/typelist.h"

namespace llvm {
    class Type;
    class LLVMContext;
}

namespace swift {

class BaseType;
class Class;
class Ptr;

//------------------------------------------------------------------------------

class Type : public Node
{
public:

    Type(location loc, TokenType modifier);
    virtual ~Type() {}

    virtual Type* clone() const = 0;
    virtual bool validate(Module* m) const = 0;
    virtual bool check(const Type* type, Module* m) const = 0;
    virtual std::string toString() const = 0;
    virtual const BaseType* isInner() const { return 0; }
    virtual bool isBool() const { return false; }
    virtual bool isIndex() const { return false; }
    virtual bool isInt() const { return false; }

    /// All types which are not user defined types are builtin types.
    virtual bool isBuiltin() const { return true; }

    /**
     * All int, uint, sat, usat, real types and bools, 
     * i.e. all BaseTypes which are builtin.
     */
    virtual bool isSimple() const { return false; }

    /// All simple types and pointers.
    virtual bool isAtomic() const = 0;

    virtual const llvm::Type* getLLVMType(Module* module) const = 0;

protected:

    TokenType modifier_;
};

//------------------------------------------------------------------------------

class BaseType : public Type
{
public:

    BaseType(location loc, TokenType modifier, std::string* id);
    BaseType(TokenType modifier, const Class* _class);
    virtual ~BaseType();

    virtual BaseType* clone() const;
    virtual bool validate(Module* m) const;
    virtual bool check(const Type* type, Module* m) const;
    virtual std::string toString() const;
    virtual const BaseType* isInner() const;
    virtual bool isBool() const;
    virtual bool isIndex() const;
    virtual bool isInt() const;

    virtual bool isBuiltin() const;
    virtual bool isSimple() const;
    virtual bool isAtomic() const;

    virtual const llvm::Type* getLLVMType(Module* module) const;

    Class* lookupClass(Module* m) const;
    const std::string* id() const;
    const char* cid() const;

    static bool isBuiltin(const std::string* id);
    static void initTypeMap(llvm::LLVMContext* llvmCtxt);
    static void destroyTypeMap();

private:

    std::string* id_;
    bool builtin_;

    typedef std::map<std::string, const llvm::Type*> TypeMap;
    static TypeMap* typeMap_; 
};

//------------------------------------------------------------------------------

class NestedType : public Type
{
public:

    NestedType(location loc, TokenType modifier, Type* innerType);
    virtual ~NestedType();

    virtual bool validate(Module* m) const;
    virtual bool check(const Type* type, Module* m) const;

    virtual const llvm::Type* getLLVMType(Module* module) const;

    Type* getInnerType();
    const Type* getInnerType() const;

protected:

    Type* innerType_;
};

//------------------------------------------------------------------------------

class Ptr : public NestedType
{
public:

    Ptr(location loc, TokenType modifier, Type* innerType);

    virtual Ptr* clone() const;
    virtual std::string toString() const;
    virtual bool isAtomic() const;
};

//------------------------------------------------------------------------------

class Container : public NestedType
{
public:

    Container(location loc, TokenType modifier, Type* innerType);

    virtual std::string toString() const;
    virtual std::string containerStr() const = 0;
    virtual bool isAtomic() const;

    static void initMeContainer();
    static size_t getContainerSize();
};

//------------------------------------------------------------------------------

class Array : public Container
{
public:

    Array(location loc, TokenType modifier, Type* innerType);

    virtual Array* clone() const;
    virtual std::string containerStr() const;
};

//------------------------------------------------------------------------------

class Simd : public Container
{
public:

    Simd(location loc, TokenType modifier, Type* innerType);

    virtual Simd* clone() const;
    virtual std::string containerStr() const;

    BaseType* getInnerType();
    const BaseType* getInnerType() const;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPE_H
