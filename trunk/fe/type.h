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

#include <utility>

#include <llvm/Support/IRBuilder.h>

#include "fe/location.h"
#include "fe/node.h"
#include "fe/token.h"
#include "fe/typelist.h"

namespace llvm {
    class Type;
    class LLVMContext;
    class OpaqueType;
    class Value;
}

namespace swift {

class Class;
class UserType;

//------------------------------------------------------------------------------

struct MemberFctInfo
{
    enum Kind
    {
        FALSE,
        COPY,
        USER,
        CONTAINER_COPY,
        CONTAINER_CREATE
    };

    MemberFct* fct_;
    Kind kind_;
    bool simd_;

    MemberFctInfo() {}
    MemberFctInfo(Kind kind, bool simd = false, MemberFct* fct = 0)
        : fct_(fct)
        , kind_(kind)
        , simd_(simd)
    {}
};

//------------------------------------------------------------------------------

class Type : public Node
{
public:

    Type(const Location& loc, TokenType modifier, bool isRef, bool isSimd);
    virtual ~Type() {}

    virtual Type* clone() const = 0;
    Type* simdClone() const;
    virtual bool check(const Type* t, Module* m) const = 0;
    virtual const Type* derefPtr() const { return this; };
    virtual bool isBool() const { return false; }
    virtual bool isIndex() const { return false; }
    virtual bool isInt() const { return false; }
    virtual bool perRef() const = 0;
    virtual bool isValid() const { return true; }
    virtual bool validate(Module* m) const = 0;
    virtual std::string toString() const = 0;
    const llvm::Type* getLLVMType(Module* m, int simdLength = 0) const;
    virtual const llvm::Type* getRawLLVMType(Module* m, int simdLength = 0) const = 0;
    virtual const llvm::Type* defineLLVMType(
            llvm::OpaqueType*& opaque, 
            const UserType*& missing,
            Module* m) const = 0;

    typedef std::pair<const llvm::Type*, int> VecType;
    virtual const llvm::Type* getVecLLVMType(Module* m, int& simdLength) const;
    virtual const llvm::Type* getRawVecLLVMType(Module* m, int& simdLength) const = 0;

    TokenType getModifier() const;
    bool isVar() const;
    bool isRef() const;

    template<class T>
    const T* cast() const
    {
        return dynamic_cast<const T*>(this);
    }

    bool isSimd() const { return simd_; }

    virtual MemberFctInfo hasMemberFct(const std::string* id, const TypeList& in, Module* m) const = 0;
    MemberFctInfo hasMemberFct(const std::string& id, const TypeList& in, Module* m) const;

protected:

    TokenType modifier_;
    bool isRef_;
    bool simd_;
};

//------------------------------------------------------------------------------

class ErrorType : public Type
{
public:

    ErrorType(bool isSimd = false);

    virtual Type* clone() const;
    virtual bool check(const Type* t, Module* m) const;
    virtual bool isBool() const { return true; }
    virtual bool isIndex() const { return true; }
    virtual bool isInt() const { return true; }
    virtual bool isValid() const { return false; }
    virtual bool validate(Module* m) const;
    virtual std::string toString() const;
    virtual bool perRef() const;
    virtual const llvm::Type* getRawLLVMType(Module* m, int simdLength = 0) const;
    virtual const llvm::Type* defineLLVMType(
            llvm::OpaqueType*& opaque, 
            const UserType*& missing,
            Module* m) const;
    virtual const llvm::Type* getRawVecLLVMType(Module* m, int& simdLength) const;
    virtual MemberFctInfo hasMemberFct(const std::string* id, const TypeList& in, Module* m) const;
};

//------------------------------------------------------------------------------

class BaseType : public Type
{
public:

    BaseType(const Location& loc, TokenType modifier, std::string* id, bool isInOut, bool isSimd);
    virtual ~BaseType();

    static BaseType* create(
            const Location& loc, 
            TokenType modifier, 
            std::string* id, 
            bool isInOut = false);

    virtual bool check(const Type* t, Module* m) const;
    virtual std::string toString() const;
    virtual MemberFctInfo hasMemberFct(const std::string* id, const TypeList& in, Module* m) const;

    Class* lookupClass(Module* m) const;
    const std::string* id() const;
    const char* cid() const;

    static void initTypeMap(llvm::LLVMContext* lctxt);
    static void destroyTypeMap();

protected:

    typedef std::map<std::string, const llvm::Type*> TypeMap;
    typedef std::map<std::string, int> SizeMap;
    static TypeMap* typeMap_; 
    static SizeMap* sizeMap_; 

private:

    std::string* id_;
};

//------------------------------------------------------------------------------

class ScalarType : public BaseType
{
public:

    ScalarType(const Location& loc, TokenType modifier, std::string* id, bool isSimd = false);

    virtual ScalarType* clone() const;
    virtual bool isBool() const;
    virtual bool isIndex() const;
    virtual bool isInt() const;
    virtual bool perRef() const;
    virtual bool validate(Module* m) const;
    virtual const llvm::Type* getRawLLVMType(Module* m, int simdLength = 0) const;
    virtual const llvm::Type* defineLLVMType(
            llvm::OpaqueType*& opaque, 
            const UserType*& missing,
            Module* m) const;
    virtual const llvm::Type* getRawVecLLVMType(Module* m, int& simdLength) const;

    bool isFloat() const;
    bool isInteger() const;
    bool isSigned() const;
    bool isUnsigned() const;
    int sizeOf() const;

    static bool isScalar(const std::string* id);
};

//------------------------------------------------------------------------------

class UserType : public BaseType
{
public:

    UserType(const Location& loc, TokenType modifier, std::string* id, bool isInOut = false, bool isSimd = false);

    virtual UserType* clone() const;
    virtual bool perRef() const;
    virtual bool validate(Module* m) const;
    virtual const llvm::Type* getRawLLVMType(Module* m, int simdLength = 0) const;
    virtual const llvm::Type* defineLLVMType(
            llvm::OpaqueType*& opaque, 
            const UserType*& missing,
            Module* m) const;
    virtual const llvm::Type* getRawVecLLVMType(Module* m, int& simdLength) const;
};

//------------------------------------------------------------------------------

class NestedType : public Type
{
public:

    NestedType(const Location& loc, TokenType modifier, bool isRef, Type* innerType);
    virtual ~NestedType();

    virtual bool validate(Module* m) const;
    virtual bool check(const Type* t, Module* m) const;
    virtual bool perRef() const;
    virtual const llvm::Type* getRawVecLLVMType(Module* m, int& simdLength) const;

    Type* getInnerType();
    const Type* getInnerType() const;

protected:

    Type* innerType_;
};

//------------------------------------------------------------------------------

class Ptr : public NestedType
{
public:

    Ptr(const Location& loc, TokenType modifier, Type* innerType);

    virtual Ptr* clone() const;
    virtual std::string toString() const;
    virtual const llvm::Type* getRawLLVMType(Module* m, int simdLength = 0) const;
    virtual const llvm::Type* defineLLVMType(
            llvm::OpaqueType*& opaque, 
            const UserType*& missing,
            Module* m) const;
    virtual MemberFctInfo hasMemberFct(const std::string* id, const TypeList& in, Module* m) const;

    const Type* derefPtr() const;
    llvm::Value* recDeref(llvm::IRBuilder<>& builder, llvm::Value* value) const;
    llvm::Value* recDerefAddr(llvm::IRBuilder<>& builder, llvm::Value* value) const;
};

//------------------------------------------------------------------------------

class Container : public NestedType
{
public:

    enum StructLayout
    {
        POINTER, 
        SIZE
    };

    Container(const Location& loc, TokenType modifier, Type* innerType);

    virtual std::string toString() const;
    virtual std::string containerStr() const = 0;

    virtual const llvm::Type* defineLLVMType(
            llvm::OpaqueType*& opaque, 
            const UserType*& missing,
            Module* m) const;

    virtual void emitCreate(Context* ctxt, llvm::Value* aggPtr, llvm::Value* size) const = 0;
    virtual void emitCopy(Context* ctxt, llvm::Value* dst, llvm::Value* src) const = 0;
    virtual MemberFctInfo hasMemberFct(const std::string* id, const TypeList& in, Module* m) const;

    static void emitCreate(Context* ctxt, 
                           const llvm::Type* allocType, 
                           llvm::Value* aggPtr,
                           llvm::Value* size,
                           int simdLength);
    static void emitCopy(Context* ctxt, 
                         const llvm::Type* allocType, 
                         llvm::Value* dst, 
                         llvm::Value* src,
                         int simdLength);

    static llvm::Value* adjustSize(Context* ctxt, 
                                   llvm::Value* size, 
                                   int simdLength);
};

//------------------------------------------------------------------------------

class Array : public Container
{
public:

    Array(const Location& loc, TokenType modifier, Type* innerType);

    virtual Array* clone() const;
    virtual std::string containerStr() const;
    virtual const llvm::Type* getRawLLVMType(Module* m, int simdLength = 0) const;
    virtual void emitCreate(Context* ctxt, llvm::Value* aggPtr, llvm::Value* size) const;
    virtual void emitCopy(Context* ctxt, llvm::Value* dst, llvm::Value* src) const;
};

//------------------------------------------------------------------------------

class Simd : public Container
{
public:

    Simd(const Location& loc, TokenType modifier, Type* innerType);

    virtual Simd* clone() const;
    virtual std::string containerStr() const;
    virtual const llvm::Type* getRawLLVMType(Module* m, int simdLength = 0) const;
    virtual void emitCreate(Context* ctxt, llvm::Value* aggPtr, llvm::Value* size) const;
    virtual void emitCopy(Context* ctxt, llvm::Value* dst, llvm::Value* src) const;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPE_H
