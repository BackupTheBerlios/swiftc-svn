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

#include <llvm/Support/TypeBuilder.h>

#include "utils/assert.h"

#include "fe/class.h"
#include "fe/error.h"

namespace swift {

//------------------------------------------------------------------------------

Type::Type(location loc, TokenType modifier)
    : Node(loc) 
    , modifier_(modifier)
{
    swiftAssert(modifier == Token::VAR || modifier == Token::CONST,
            "illegal modifier value");
}

TokenType Type::getModifier() const
{
    return modifier_;
}

//------------------------------------------------------------------------------

ErrorType::ErrorType(location loc, TokenType modifier)
    : Type(loc, modifier)
{}

Type* ErrorType::clone() const
{
    return new ErrorType(loc_, modifier_);
}

bool ErrorType::validate(Module* m) const
{
    return true;
}

bool ErrorType::check(const Type* type, Module* m) const
{
    return true;
}

std::string ErrorType::toString() const
{
    return "error";
}

bool ErrorType::isAtomic() const
{
    return false;
}

const llvm::Type* ErrorType::getLLVMType(Module* module) const
{
    swiftAssert(false, "unreachable");
    return 0;
}

//------------------------------------------------------------------------------

VoidType::VoidType(location loc)
    : Type(loc, Token::CONST)
{}

Type* VoidType::clone() const
{
    return new VoidType(loc_);
}

bool VoidType::validate(Module* m) const
{
    return true;
}

bool VoidType::check(const Type* type, Module* m) const
{
    errorf(loc_, "void value not ignored as it ought to be");
    return false;
}

std::string VoidType::toString() const
{
    return "void";
}

bool VoidType::isAtomic() const
{
    return false;
}

const llvm::Type* VoidType::getLLVMType(Module* module) const
{
    swiftAssert(false, "unreachable");
    return 0;
}

//------------------------------------------------------------------------------

BaseType::TypeMap* BaseType::typeMap_ = 0;

BaseType::BaseType(location loc, TokenType modifier, std::string* id)
    : Type(loc, modifier)
    , id_(id)
    , builtin_( typeMap_->find(*id) != typeMap_->end() ) // is it a builtin type?
{}

BaseType::BaseType(TokenType modifier, const Class* _class)
    : Type(location() , modifier)
    , id_( new std::string(*_class->id()) )
    , builtin_( typeMap_->find(*id_) != typeMap_->end() ) // is it a builtin type?
{}

BaseType::~BaseType()
{
    delete id_;
}

BaseType* BaseType::clone() const
{
    return new BaseType( loc_, modifier_, new std::string(*id_) );
}

bool BaseType::validate(Module* m) const
{
    if ( m->lookupClass(id_) == 0 )
    {
        errorf( loc_, "class '%s' is not defined in module '%s'", cid(), m->cid() );
        return false;
    }

    return true;
}

bool BaseType::check(const Type* type, Module* m) const
{
    if (const BaseType* bt = dynamic_cast<const BaseType*>(type))
    {

        Class* class1 = m->lookupClass(id_);
        Class* class2 = m->lookupClass(bt->id_);

        // both classes must exist
        swiftAssert(class1,  "first class not found");
        swiftAssert(class2, "second class not found");

        // different pointers mean different types
        if (class1 != class2) 
            return false;
        else
            return true;
    }

    return false;
}

std::string BaseType::toString() const
{
    return *id_;
}

const BaseType* BaseType::isInner() const
{
    return this;
}

bool BaseType::isBuiltin() const
{
    return builtin_;
}

bool BaseType::isSimple() const
{
    return builtin_;
}

bool BaseType::isAtomic() const
{
    return builtin_;
}

bool BaseType::isBool() const
{
    return *id_ == "bool";
}

bool BaseType::isIndex() const
{
    return *id_ == "index";
}

bool BaseType::isInt() const
{
    return *id_ == "int";
}

bool BaseType::isSigned() const
{
    return *id_ == "int" || *id_ == "int8" || *id_ == "int16" || *id_ == "int32" || *id_ == "int64";
}

bool BaseType::isUnsigned() const
{
    return *id_ == "uint" || *id_ == "uint8" || *id_ == "uint16" || *id_ == "uint32" || *id_ == "uint64" || *id_ == "index";
}

bool BaseType::isInteger() const
{
    return isSigned() || isUnsigned();
}

bool BaseType::isFloat() const
{
    return *id_ == "real" || *id_ == "real32" || *id_ == "real64";
}

const llvm::Type* BaseType::getLLVMType(Module* module) const
{
    if (builtin_)
        return (*typeMap_)[*id_];
    else
        return lookupClass(module)->llvmType();
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

bool BaseType::isBuiltin(const std::string* id)
{
    return typeMap_->find(*id) != typeMap_->end();
}

void BaseType::initTypeMap(llvm::LLVMContext* llvmCtxt)
{
    typeMap_ = new TypeMap();
    TypeMap& typeMap = *typeMap_;

    typeMap["bool"]   = llvm::TypeBuilder<llvm::types::i<1>, true>::get(*llvmCtxt);

    typeMap["int8"]   = llvm::TypeBuilder<llvm::types::i<8>, true>::get(*llvmCtxt);
    typeMap["sat8"]   = llvm::TypeBuilder<llvm::types::i<8>, true>::get(*llvmCtxt);
    typeMap["uint8"]  = llvm::TypeBuilder<llvm::types::i<8>, true>::get(*llvmCtxt);
    typeMap["usat8"]  = llvm::TypeBuilder<llvm::types::i<8>, true>::get(*llvmCtxt);

    typeMap["int16"]  = llvm::TypeBuilder<llvm::types::i<16>, true>::get(*llvmCtxt);
    typeMap["uint16"] = llvm::TypeBuilder<llvm::types::i<16>, true>::get(*llvmCtxt);
    typeMap["sat16"]  = llvm::TypeBuilder<llvm::types::i<16>, true>::get(*llvmCtxt);
    typeMap["usat16"] = llvm::TypeBuilder<llvm::types::i<16>, true>::get(*llvmCtxt);

    typeMap["int32"]  = llvm::TypeBuilder<llvm::types::i<32>, true>::get(*llvmCtxt);
    typeMap["uint32"] = llvm::TypeBuilder<llvm::types::i<32>, true>::get(*llvmCtxt);

    typeMap["int64"]  = llvm::TypeBuilder<llvm::types::i<64>, true>::get(*llvmCtxt);
    typeMap["uint64"] = llvm::TypeBuilder<llvm::types::i<64>, true>::get(*llvmCtxt);

    typeMap["real"]   = llvm::TypeBuilder<llvm::types::ieee_float, true>::get(*llvmCtxt);
    typeMap["real32"] = llvm::TypeBuilder<llvm::types::ieee_float, true>::get(*llvmCtxt);
    typeMap["real64"] = llvm::TypeBuilder<llvm::types::ieee_double, true>::get(*llvmCtxt);

    typeMap["int"]   = llvm::TypeBuilder<llvm::types::i<32>, true>::get(*llvmCtxt);
    typeMap["uint"]  = llvm::TypeBuilder<llvm::types::i<32>, true>::get(*llvmCtxt);
    typeMap["index"] = llvm::TypeBuilder<llvm::types::i<64>, true>::get(*llvmCtxt);
}

void BaseType::destroyTypeMap()
{
    delete typeMap_;
}

//------------------------------------------------------------------------------

NestedType::NestedType(location loc, TokenType modifier, Type* innerType)
    : Type(loc, modifier)
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

    swiftAssert( dynamic_cast<const NestedType*>(type), 
            "must be castable to NestedType" );
    
    const NestedType* nestedType = (const NestedType*) type;

    return innerType_->check(nestedType->innerType_, m);
}

Type* NestedType::getInnerType()
{
    return innerType_;
}

const Type* NestedType::getInnerType() const
{
    return innerType_;
}

const llvm::Type* NestedType::getLLVMType(Module* module) const
{
    swiftAssert(false, "TODO");
    return 0;
}

//------------------------------------------------------------------------------

Ptr::Ptr(location loc, TokenType modifier, Type* innerType)
    : NestedType(loc, modifier, innerType)
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

bool Ptr::isAtomic() const
{
    return true;
}

//------------------------------------------------------------------------------

Container::Container(location loc, TokenType modifier, Type* innerType)
    : NestedType(loc, modifier, innerType)
{}

std::string Container::toString() const
{
    std::ostringstream oss;
    oss << containerStr() << '{' << innerType_->toString() << '}';

    return oss.str();
}

bool Container::isAtomic() const
{
    return false;
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

//------------------------------------------------------------------------------

Simd::Simd(location loc, TokenType modifier, Type* innerType)
    : Container(loc, modifier, innerType)
{}

Simd* Simd::clone() const
{
    return new Simd(location() , modifier_, innerType_->clone());
}

std::string Simd::containerStr() const
{
    return "simd";
}

BaseType* Simd::getInnerType()
{
    swiftAssert( typeid(*innerType_) == typeid(BaseType),
            "inner type must be a BaseType");

    return (BaseType*) innerType_;
}

const BaseType* Simd::getInnerType() const
{
    swiftAssert( typeid(*innerType_) == typeid(BaseType),
            "inner type must be a BaseType");

    return (BaseType*) innerType_;
}

} // namespace swift
