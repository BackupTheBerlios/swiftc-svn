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

#include "fe/type.h"

#include <iostream>
#include <sstream>
#include <typeinfo>
#include <utility>

#include <llvm/Module.h>
#include <llvm/Support/TypeBuilder.h>
#include <llvm/Transforms/Utils/BuildLibCalls.h>
#include <llvm/Transforms/Utils/BuildLibCalls.h>

#include "utils/assert.h"
#include "utils/cast.h"
#include "utils/llvmhelper.h"

#include "vec/typevectorizer.h"

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"

using llvm::Value;

namespace swift {

//------------------------------------------------------------------------------

Type::Type(location loc, TokenType modifier, bool isRef, bool isSimd /*= false*/)
    : Node(loc) 
    , modifier_(modifier)
    , isRef_(isRef)
    , isSimd_(isSimd)
{
    swiftAssert(modifier != Token::VAR || modifier != Token::CONST,
            "illegal modifier value");
}

Type* Type::simdClone() const
{
    Type* result = clone();
    result->isSimd_ = true;

    return result;
}

TokenType Type::getModifier() const
{
    return modifier_;
}

bool Type::isVar() const
{
    return modifier_ == Token::VAR;
}

bool Type::isRef() const
{
    return isRef_;
}

const llvm::Type* Type::getLLVMType(Module* m) const
{
    const llvm::Type* llvmType = getRawLLVMType(m);

    if (isRef_)
        return llvm::PointerType::getUnqual(llvmType);
    else
        return llvmType;
}

const llvm::Type* Type::getVecLLVMType(Module* m, int& simdLength) const
{
    const llvm::Type* llvmType = getRawVecLLVMType(m, simdLength);

    if (isRef_)
        return llvm::PointerType::getUnqual(llvmType);
    else
        return llvmType;
}

//------------------------------------------------------------------------------

ErrorType::ErrorType(bool isSimd /*= false*/)
    : Type( location(), Token::VAR, false, isSimd )
{}

Type* ErrorType::clone() const
{
    return new ErrorType();
}

bool ErrorType::validate(Module* m) const
{
    return true;
}

bool ErrorType::check(const Type* t, Module* m) const
{
    return true;
}

std::string ErrorType::toString() const
{
    return "error";
}

bool ErrorType::perRef() const
{
    swiftAssert(false, "unreachable");
    return false;
}

const llvm::Type* ErrorType::getRawLLVMType(Module* m) const
{
    swiftAssert(false, "unreachable");
    return 0;
}


const llvm::Type* ErrorType::defineLLVMType(
        llvm::OpaqueType*& opaque, 
        const UserType*& missing,
        Module* m) const
{
    swiftAssert(false, "unreachable");
    opaque = 0;
    missing = 0;
    return 0;
}

const llvm::Type* ErrorType::getRawVecLLVMType(Module* m, int& simdLength) const 
{
    swiftAssert(false, "unreachable");
    return 0;
}

//------------------------------------------------------------------------------

BaseType::TypeMap* BaseType::typeMap_ = 0;
BaseType::SizeMap* BaseType::sizeMap_ = 0;

BaseType::BaseType(location loc, TokenType modifier, std::string* id, bool isInOut, bool isSimd /*= false*/)
    : Type(loc, modifier, isInOut, isSimd)
    , id_(id)
{}

BaseType* BaseType::create(
        location loc, 
        TokenType modifier, 
        std::string* id, 
        bool isInOut)
{
    if ( typeMap_->find(*id) == typeMap_->end() )
        return new UserType(loc, modifier, id, isInOut); // a user defined type
    else
        return new ScalarType(loc, modifier, id); // a builtin type
}

BaseType::~BaseType()
{
    delete id_;
}

bool BaseType::check(const Type* type, Module* m) const
{
    if ( const BaseType* bt = type->cast<BaseType>() )
    {
        Class* class1 = m->lookupClass( id() );
        Class* class2 = m->lookupClass( bt->id() );

        // invalid clases and different pointers mean different types
        return class1 && class2 && (class1 == class2);
    }

    return false;
}

std::string BaseType::toString() const
{
    return *id_;
}

Class* BaseType::lookupClass(Module* m) const
{
    Class* c = m->lookupClass(id_);
    return c;
}

const std::string* BaseType::id() const
{
    return id_;
}

const char* BaseType::cid() const
{
    return id_->c_str();
}

void BaseType::initTypeMap(llvm::LLVMContext* lctxt)
{
    typeMap_ = new TypeMap();
    TypeMap& typeMap = *typeMap_;

    typeMap["bool"]   = llvm::IntegerType::getInt1Ty(*lctxt);

    typeMap["int8"]   = llvm::IntegerType::getInt8Ty(*lctxt);
    typeMap["uint8"]  = llvm::IntegerType::getInt8Ty(*lctxt);
    typeMap["sat8"]   = llvm::IntegerType::getInt8Ty(*lctxt);
    typeMap["usat8"]  = llvm::IntegerType::getInt8Ty(*lctxt);

    typeMap["int16"]  = llvm::IntegerType::getInt16Ty(*lctxt);
    typeMap["uint16"] = llvm::IntegerType::getInt16Ty(*lctxt);
    typeMap["sat16"]  = llvm::IntegerType::getInt16Ty(*lctxt);
    typeMap["usat16"] = llvm::IntegerType::getInt16Ty(*lctxt);

    typeMap["int32"]  = llvm::IntegerType::getInt32Ty(*lctxt);
    typeMap["uint32"] = llvm::IntegerType::getInt32Ty(*lctxt);
    typeMap["int"]    = llvm::IntegerType::getInt32Ty(*lctxt);
    typeMap["uint"]   = llvm::IntegerType::getInt32Ty(*lctxt);

    typeMap["int64"]  = llvm::IntegerType::getInt64Ty(*lctxt);
    typeMap["uint64"] = llvm::IntegerType::getInt64Ty(*lctxt);
    typeMap["index"]  = llvm::IntegerType::getInt64Ty(*lctxt);

    typeMap["real"]   = llvm::TypeBuilder<llvm::types::ieee_float,  true>::get(*lctxt);
    typeMap["real32"] = llvm::TypeBuilder<llvm::types::ieee_float,  true>::get(*lctxt);
    typeMap["real64"] = llvm::TypeBuilder<llvm::types::ieee_double, true>::get(*lctxt);

    sizeMap_ = new SizeMap();
    SizeMap& sizeMap = *sizeMap_;

    sizeMap["bool"]   = 1;
    sizeMap["int8"]   = 1;
    sizeMap["uint8"]  = 1;
    sizeMap["sat8"]   = 1;
    sizeMap["usat8"]  = 1;

    sizeMap["int16"]  = 2;
    sizeMap["uint16"] = 2;
    sizeMap["sat16"]  = 2;
    sizeMap["usat16"] = 2;

    sizeMap["int32"]  = 4;
    sizeMap["uint32"] = 4;
    sizeMap["int"]    = 4;
    sizeMap["uint"]   = 4;

    sizeMap["int64"]  = 8;
    sizeMap["uint64"] = 8;
    sizeMap["index"]  = 8;

    sizeMap["real"]   = 4;
    sizeMap["real32"] = 4;
    sizeMap["real64"] = 8;
}

void BaseType::destroyTypeMap()
{
    delete typeMap_;
    delete sizeMap_;
}

//------------------------------------------------------------------------------

ScalarType::ScalarType(location loc, TokenType modifier, std::string* id, bool isSimd /*= false*/)
    : BaseType(loc, modifier, id, false, isSimd)
{
    swiftAssert( typeMap_->find(*this->id()) != typeMap_->end(), "must be found" );
}

ScalarType* ScalarType::clone() const
{
    return new ScalarType( loc_, modifier_, new std::string(*id()) );
}

bool ScalarType::isBool() const
{
    return *id() == "bool";
}

bool ScalarType::isIndex() const
{
    return *id() == "index";
}

bool ScalarType::isInt() const
{
    return *id() == "int";
}

bool ScalarType::perRef() const
{
    return false;
}

bool ScalarType::validate(Module* m) const
{
    return true;
}

const llvm::Type* ScalarType::getRawLLVMType(Module* m) const
{
    return (*typeMap_)[ *id() ];
}

const llvm::Type* ScalarType::defineLLVMType(
        llvm::OpaqueType*& opaque, 
        const UserType*& missing,
        Module* m) const
{
    opaque = 0;
    missing = 0;
    return getLLVMType(m);
}

const llvm::Type* ScalarType::getRawVecLLVMType(Module* m, int& simdLength) const 
{
    const llvm::Type* llvmType = getLLVMType(m);
    simdLength = vec::TypeVectorizer::lengthOfScalar(llvmType, Context::SIMD_WIDTH);
    return vec::TypeVectorizer::vecScalar(llvmType, simdLength, Context::SIMD_WIDTH);
}

bool ScalarType::isFloat() const
{
    return *id() == "real" || *id() == "real32" || *id() == "real64";
}

bool ScalarType::isInteger() const
{
    return isSigned() || isUnsigned();
}

bool ScalarType::isSigned() const
{
    return *id() == "int" 
        || *id() == "int8" 
        || *id() == "int16" 
        || *id() == "int32" 
        || *id() == "int64";
}

bool ScalarType::isUnsigned() const
{
    return *id() == "uint" 
        || *id() == "uint8" 
        || *id() == "uint16" 
        || *id() == "uint32" 
        || *id() == "uint64" 
        || *id() == "index";
}

int ScalarType::sizeOf() const
{
    return (*sizeMap_)[*id()];
}

bool ScalarType::isScalar(const std::string* id)
{
    return typeMap_->find(*id) != typeMap_->end();
}

//------------------------------------------------------------------------------

UserType::UserType(location loc, TokenType modifier, std::string* id, bool isInOut /*= false*/, bool isSimd /*= false*/)
    : BaseType(loc, modifier, id, isInOut, isSimd)
{}

UserType* UserType::clone() const
{
    return new UserType(loc_, modifier_, new std::string(*id()), isRef_);
}

bool UserType::perRef() const
{
    return true;
}

bool UserType::validate(Module* m) const
{
    if ( m->lookupClass(id()) == 0 )
    {
        errorf( loc_, "class '%s' is not defined in module '%s'", cid(), m->cid() );
        return false;
    }

    return true;
}

const llvm::Type* UserType::getRawLLVMType(Module* m) const
{
    Class* c = m->lookupClass( id() );
    swiftAssert(c, "must be found");
    const llvm::Type* llvmType = c->getLLVMType();

    return llvmType;
}

const llvm::Type* UserType::defineLLVMType(
        llvm::OpaqueType*& opaque, 
        const UserType*& missing,
        Module* m) const
{
    opaque = 0;
    missing = 0;
    const llvm::Type* llvmType = getLLVMType(m);

    if (!llvmType)
        missing = this;

    return llvmType;
}

const llvm::Type* UserType::getRawVecLLVMType(Module* m, int& simdLength) const 
{
    Class* c = lookupClass(m);
    swiftAssert( c->isSimd(), "not declared as simd type" );

    const llvm::StructType* vecType = c->getVecType();
    simdLength = c->getSimdLength();

    swiftAssert( vecType, "must be valid" );
    swiftAssert( simdLength > 0, "must be valid" );

    return vecType;
}

//------------------------------------------------------------------------------

NestedType::NestedType(location loc, TokenType modifier, bool isRef, Type* innerType)
    : Type(loc, modifier, isRef, false /*no simd type in all cases*/)
    , innerType_(innerType)
{}

NestedType::~NestedType()
{
    delete innerType_;
}

bool NestedType::validate(Module* m) const
{
    return innerType_->validate(m);
}

bool NestedType::check(const Type* type, Module* m) const
{
    if ( typeid(*this) != typeid(*type) )
        return false;

    swiftAssert( dynamic<NestedType>(type), 
            "must be castable to NestedType" );
    
    const NestedType* nestedType = (const NestedType*) type;

    return innerType_->check(nestedType->innerType_, m);
}

bool NestedType::perRef() const
{
    return false;
}

Type* NestedType::getInnerType()
{
    return innerType_;
}

const Type* NestedType::getInnerType() const
{
    return innerType_;
}

const llvm::Type* NestedType::getRawVecLLVMType(Module* m, int& simdLength) const
{
    swiftAssert(false, "unreachable"); 
    return 0;
}

//------------------------------------------------------------------------------

Ptr::Ptr(location loc, TokenType modifier, Type* innerType)
    : NestedType(loc, modifier, false, innerType)
{}

Ptr* Ptr::clone() const
{
    return new Ptr(location() , modifier_, innerType_->clone());
}

std::string Ptr::toString() const
{
    std::ostringstream oss;
    oss << "ptr{" << innerType_->toString() << '}';

    return oss.str();
}

const llvm::Type* Ptr::getRawLLVMType(Module* m) const
{
    return llvm::PointerType::getUnqual( innerType_->getRawLLVMType(m) );
}

const llvm::Type* Ptr::defineLLVMType(
        llvm::OpaqueType*& opaque, 
        const UserType*& missing,
        Module* m) const
{
    const llvm::Type* result = innerType_->defineLLVMType(opaque, missing, m);
    if (result)
        return llvm::PointerType::getUnqual(result);

    swiftAssert( !opaque, "must not be set" );
    swiftAssert( missing, "must be set" );

    opaque = llvm::OpaqueType::get(*m->lctxt_);

    return llvm::PointerType::getUnqual(opaque);
}

const Type* Ptr::derefPtr() const
{
    return innerType_->derefPtr();
}

Value* Ptr::recDeref(llvm::IRBuilder<>& builder, Value* value) const
{
    return builder.CreateLoad( recDeref(builder, value), value->getName() );
}

Value* Ptr::recDerefAddr(llvm::IRBuilder<>& builder, Value* value) const
{
    swiftAssert( !innerType_->isRef(), "must not be a reference" );
    swiftAssert( !isRef_, "must not be a reference" );

    value = builder.CreateLoad( value, value->getName() );

    if ( const Ptr* ptr = innerType_->cast<Ptr>() )
        return ptr->recDeref(builder, value);

    return value;
}

//------------------------------------------------------------------------------

Container::Container(location loc, TokenType modifier, Type* innerType)
    : NestedType(loc, modifier, false, innerType)
{}

std::string Container::toString() const
{
    std::ostringstream oss;
    oss << containerStr() << '{' << innerType_->toString() << '}';

    return oss.str();
}

const llvm::Type* Container::defineLLVMType(
        llvm::OpaqueType*& opaque, 
        const UserType*& missing,
        Module* m) const
{
    swiftAssert(false, "TODO");
    opaque = 0;
    missing = 0;
    return 0;
}

void Container::emitCreate(Context* ctxt, 
                           const llvm::Type* allocType, 
                           Value* aggPtr, 
                           Value* size,
                           int simdLength)
{
    LLVMBuilder& builder = ctxt->builder_;
    llvm::LLVMContext& lctxt = ctxt->lctxt();

    const llvm::PointerType* ptrType = llvm::PointerType::getUnqual(allocType);

    // adjust size to simdLength-1 if necessary
    Value* adjustedSize = (simdLength > 1) 
        ? adjustSize(ctxt, size, simdLength)
        : size;

    Value* ptr = ctxt->createMalloc(adjustedSize, ptrType);
    Value*  ptrDstAddr = createInBoundsGEP_0_i32(lctxt, builder, aggPtr, POINTER, aggPtr->getNameStr() + ".ptr");
    Value* sizeDstAddr = createInBoundsGEP_0_i32(lctxt, builder, aggPtr, SIZE, aggPtr->getNameStr() + ".size");

    // and store
    builder.CreateStore(ptr, ptrDstAddr);
    builder.CreateStore(size, sizeDstAddr);
}

void Container::emitCopy(Context* ctxt, 
                         const llvm::Type* allocType, 
                         Value* dst, 
                         Value* src, 
                         int simdLength)

{
    LLVMBuilder& builder = ctxt->builder_;
    llvm::LLVMContext& lctxt = ctxt->lctxt();

    Value* size = createLoadInBoundsGEP_0_i32(lctxt, builder, src, SIZE);

    emitCreate(ctxt, allocType, dst, size, simdLength);

    Value* srcPtr = createLoadInBoundsGEP_0_i32(lctxt, builder, src, POINTER, src->getNameStr() + ".ptr");
    Value* dstPtr = createLoadInBoundsGEP_0_i32(lctxt, builder, dst, POINTER, dst->getNameStr() + ".ptr");

    Value* adjustedSize = adjustSize(ctxt, size, simdLength);
    ctxt->createMemCpy(dstPtr, srcPtr, adjustedSize);
}

Value* Container::adjustSize(Context* ctxt, Value* size, int simdLength)
{
    llvm::LLVMContext& lctxt = ctxt->lctxt();

    if (simdLength > 1) // round up
        size = ctxt->builder_.CreateAdd( size, createInt64(lctxt, simdLength-1) );

    return ctxt->builder_.CreateUDiv( size, createInt64(lctxt, simdLength) );
}

//------------------------------------------------------------------------------

Array::Array(location loc, TokenType modifier, Type* innerType)
    : Container(loc, modifier, innerType)
{}

Array* Array::clone() const
{
    return new Array(location() , modifier_, innerType_->clone());
}

std::string Array::containerStr() const
{
    return "array";
}

const llvm::Type* Array::getRawLLVMType(Module* m) const
{
    llvm::LLVMContext& lctxt = *m->lctxt_;

    LLVMTypes llvmTypes(2);
    llvmTypes[POINTER] = llvm::PointerType::getUnqual( innerType_->getLLVMType(m) );
    llvmTypes[SIZE] = llvm::IntegerType::getInt64Ty(lctxt);


    const llvm::StructType* st = llvm::StructType::get(lctxt, llvmTypes);
    m->getLLVMModule()->addTypeName( toString().c_str(), st );
     
    return st;
}

void Array::emitCreate(Context* ctxt, Value* aggPtr, Value* size) const
{
    const llvm::Type* inner = innerType_->getLLVMType(ctxt->module_);
    Container::emitCreate(ctxt, inner, aggPtr, size, 1);
}

void Array::emitCopy(Context* ctxt, Value* dst, Value* src) const
{
    const llvm::Type* inner = innerType_->getLLVMType(ctxt->module_);
    Container::emitCopy(ctxt, inner, dst, src, 1);
}

//------------------------------------------------------------------------------

Simd::Simd(location loc, TokenType modifier, Type* innerType)
    : Container(loc, modifier, innerType)
{}

Simd* Simd::clone() const
{
    return new Simd( location(), modifier_, innerType_->clone() );
}

std::string Simd::containerStr() const
{
    return "simd";
}

const llvm::Type* Simd::getRawLLVMType(Module* m) const
{
    llvm::LLVMContext& lctxt = *m->lctxt_;

    LLVMTypes llvmTypes(2);
    int simdLength;
    llvmTypes[POINTER] = llvm::PointerType::getUnqual( 
            innerType_->getRawVecLLVMType(m, simdLength) );
    llvmTypes[SIZE] = llvm::IntegerType::getInt64Ty(lctxt);

    const llvm::StructType* st = llvm::StructType::get(lctxt, llvmTypes);
    m->getLLVMModule()->addTypeName( toString().c_str(), st );
     
    return st;
}

void Simd::emitCreate(Context* ctxt, Value* aggPtr, Value* size) const
{
    int simdLength;
    const llvm::Type* vecType = innerType_->getRawVecLLVMType(ctxt->module_, simdLength);

    Container::emitCreate(ctxt, vecType, aggPtr, size, simdLength);
}

void Simd::emitCopy(Context* ctxt, Value* dst, Value* src) const
{
    int simdLength;
    const llvm::Type* vecType = innerType_->getRawVecLLVMType(ctxt->module_, simdLength);

    Container::emitCopy(ctxt, vecType, dst, src, simdLength);
}

} // namespace swift
