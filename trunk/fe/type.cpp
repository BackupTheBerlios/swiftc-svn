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

#include "utils/assert.h"

#include "fe/class.h"
#include "fe/error.h"
#include "fe/expr.h"
#include "fe/symtab.h"

#include "me/arch.h"
#include "me/functab.h"
#include "me/ssa.h"
#include "me/offset.h"
#include "me/struct.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor
 */

Type::Type(int modifier, location loc)
    : Node(loc) 
    , modifier_(modifier)
{}

/*
 * virtual methods
 */

bool Type::isBool() const
{
    return false;
}

bool Type::isIndex() const
{
    return false;
}

bool Type::isInt() const
{
    return false;
}

/*
 * further methods
 */

Type* Type::constClone() const
{
    Type* type = this->clone();
    type->modifier_ = Token::CONST;

    return type;
}

Type* Type::varClone() const
{
    Type* type = this->clone();
    type->modifier_ = Token::VAR;

    return type;
}

const BaseType* Type::isInnerAtomic() const
{
    return isAtomic() ? isInner() : 0;
}

bool Type::isNonInnerBuiltin() const
{
    return !isInner() && isBuiltin();
}

int& Type::modifier()
{
    return modifier_;
}

const int& Type::modifier() const
{
    return modifier_;
}

bool Type::isReadOnly() const
{
    return modifier_ == Token::CONST || modifier_ == Token::CONST_REF;
}

me::Reg* Type::loadPtr(me::Var* var) const
{
#ifdef SWIFT_DEBUG
    std::string str = "tmp";
    me::Reg* ptr = me::functab->newReg(me::Op::R_PTR, &str);
#else // SWIFT_DEBUG
    me::Reg* ptr = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

    me::LoadPtr* loadPtr = new me::LoadPtr(ptr, var, 0, 0);
    me::functab->appendInstr(loadPtr);

    return ptr;
}

//------------------------------------------------------------------------------

/*
 * init statics
 */

BaseType::TypeMap* BaseType::typeMap_ = 0;

/*
 * constructor and destructor
 */

BaseType::BaseType(int modifier, std::string* id, location loc)
    : Type(modifier, loc)
    , id_(id)
    , builtin_( typeMap_->find(*id) != typeMap_->end() ) // is it a builtin type?
{}

BaseType::BaseType(int modifier, const Class* _class)
    : Type(modifier, location() ) // TODO location
    , id_( new std::string(*_class->id_) )
    , builtin_( typeMap_->find(*id_) != typeMap_->end() ) // is it a builtin type?
{}

BaseType::~BaseType()
{
    delete id_;
}

/*
 * virtual methods
 */

BaseType* BaseType::clone() const
{
    return new BaseType( modifier_, new std::string(*id_), loc_);
}

bool BaseType::validate() const
{
    if ( symtab->lookupClass(id_) == 0 )
    {
        errorf( loc_, "class '%s' is not defined in this module", id_->c_str() );
        return false;
    }

    return true;
}

bool BaseType::check(const Type* type) const
{
    const BaseType* bt = dynamic_cast<const BaseType*>(type);

    if (!bt)
        return false;

    Class* class1 = symtab->lookupClass(id_);
    Class* class2 = symtab->lookupClass(bt->id_);

    // both classes must exist
    swiftAssert(class1, "first class not found in the symbol table");
    swiftAssert(class2, "second class not found in the symbol table");

    if (class1 != class2) 
    {
        // different pointers -> hence different types
        return false;
    }

    return true;
}

me::Op::Type BaseType::toMeType() const
{
    if (builtin_)
        return typeMap_->find(*id_)->second;
    else
    {
        if ( isActuallyPtr() )
            return me::Op::R_PTR; // params are passed in pointers
        else
        {
            swiftAssert(modifier_ == Token::VAR || modifier_ == Token::CONST, 
                    "impossible modifier_ value");
            return me::Op::R_MEM;
        }
    }
}

bool BaseType::isAtomic() const
{
    return builtin_;
}

bool BaseType::isInternalAtomic() const
{
    return builtin_ || isActuallyPtr();
}

bool BaseType::isBuiltin() const
{
    return builtin_;
}

const BaseType* BaseType::isInner() const
{
    return this;
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

size_t BaseType::sizeOf() const
{
    if (builtin_)
        return me::Op::sizeOf( typeMap_->find(*id_)->second );
    else
        return lookupClass()->meStruct_->sizeOf();
}

bool BaseType::isActuallyPtr() const
{
    return modifier_ == Token::CONST_REF || modifier_ == Token::REF;
}

me::Var* BaseType::createVar(const std::string* id /*= 0*/) const
{
    me::Op::Type meType = toMeType();

    me::Var* var;
    if (meType == me::Op::R_MEM)
    {
        Class* _class = lookupClass();
#ifdef SWIFT_DEBUG
        var = me::functab->newMemVar(_class->meStruct_, id);
#else // SWIFT_DEBUG
        var = me::functab->newMemVar(_class->meStruct_);
#endif // SWIFT_DEBUG
    }
    else
    {
#ifdef SWIFT_DEBUG
        var = me::functab->newReg(meType, id);
#else // SWIFT_DEBUG
        var = me::functab->newReg(meType);
#endif // SWIFT_DEBUG
    }

    return var;
}

me::Reg* BaseType::derefToInnerstPtr(me::Var* var) const
{
    if ( isActuallyPtr() )
    {
        // reg is a hidden ptr
        swiftAssert( typeid(*var) == typeid(me::Reg), "must be a Reg here" );
        return (me::Reg*) var; 
    }

    // load adress into pointer reg
    swiftAssert(!builtin_, "LoadPtr not allowed");
    
    return loadPtr(var);
}

const BaseType* BaseType::unnestPtr() const
{
    return this;
}

const Ptr* BaseType::unnestInnerstPtr() const
{
    swiftAssert(false, "unreachable code");
    return 0;
}

bool BaseType::hasAssignCreate(const TypeList& /*in*/, 
                               bool /*isCreate*/, 
                               location loc) const
{
    swiftAssert(false, "unreachable code");
    return false;
}

std::string BaseType::toString() const
{
    return *id_;
}

/*
 * further methods
 */

Class* BaseType::lookupClass() const
{
    Class* _class = symtab->lookupClass(id_);
    swiftAssert(_class, "must be found");
    return _class;
}

const std::string* BaseType::getId() const
{
    return id_;
}

/*
 * static methods
 */

bool BaseType::isBuiltin(const std::string* id)
{
    return typeMap_->find(*id) != typeMap_->end();
}

void BaseType::initTypeMap()
{
    typeMap_ = new TypeMap();

    (*typeMap_)["bool"]   = me::Op::R_BOOL;

    (*typeMap_)["int8"]   = me::Op::R_INT8;
    (*typeMap_)["int16"]  = me::Op::R_INT16;
    (*typeMap_)["int32"]  = me::Op::R_INT32;
    (*typeMap_)["int64"]  = me::Op::R_INT64;

    (*typeMap_)["sat8"]   = me::Op::R_SAT8;
    (*typeMap_)["sat16"]  = me::Op::R_SAT16;

    (*typeMap_)["uint8"]  = me::Op::R_UINT8;
    (*typeMap_)["uint16"] = me::Op::R_UINT16;
    (*typeMap_)["uint32"] = me::Op::R_UINT32;
    (*typeMap_)["uint64"] = me::Op::R_UINT64;

    (*typeMap_)["usat8"]   = me::Op::R_USAT8;
    (*typeMap_)["usat16"]  = me::Op::R_USAT16;

    (*typeMap_)["real32"] = me::Op::R_REAL32;
    (*typeMap_)["real64"] = me::Op::R_REAL64;

    (*typeMap_)["int"]    = me::arch->getPreferedInt();
    (*typeMap_)["uint"]   = me::arch->getPreferedUInt();
    (*typeMap_)["index"]  = me::arch->getPreferedIndex();
    (*typeMap_)["real"]   = me::arch->getPreferedReal();
}

void BaseType::destroyTypeMap()
{
    delete typeMap_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

NestedType::NestedType(int modifier, Type* innerType, location loc)
    : Type(modifier, loc)
    , innerType_(innerType)
{}

NestedType::~NestedType()
{
    delete innerType_;
}

/*
 * virtual methods
 */

bool NestedType::validate() const
{
    return innerType_->validate();
}

bool NestedType::isBuiltin() const
{
    return true;
}

const BaseType* NestedType::isInner() const
{
    return 0;
}

bool NestedType::check(const Type* type) const
{
    if ( typeid(*this) != typeid(*type) )
        return false;

    swiftAssert( dynamic_cast<const NestedType*>(type), 
            "must be castable to NestedType" );
    
    const NestedType* nestedType = (const NestedType*) type;

    return innerType_->check(nestedType->innerType_);
}

/*
 * further methods
 */

Type* NestedType::getInnerType()
{
    return innerType_;
}

const Type* NestedType::getInnerType() const
{
    return innerType_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Ptr::Ptr(int modifier, Type* innerType, location loc)
    : NestedType(modifier, innerType, loc)
{}

Ptr* Ptr::clone() const
{
    return new Ptr(modifier_, innerType_->clone(), location() ); // TODO location
}

/*
 * virtual methods
 */

me::Op::Type Ptr::toMeType() const
{
    return me::Op::R_PTR;
}

me::Var* Ptr::createVar(const std::string* id /*= 0*/) const
{
#ifdef SWIFT_DEBUG
    return me::functab->newReg(me::Op::R_PTR, id);
#else // SWIFT_DEBUG
    return me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG
}

bool Ptr::isAtomic() const
{
    return true;
}

bool Ptr::isInternalAtomic() const
{
    return true;
}

bool Ptr::isActuallyPtr() const
{
    return true;
}

size_t Ptr::sizeOf() const
{
    return me::Op::sizeOf(me::Op::R_PTR);
}

const BaseType* Ptr::unnestPtr() const
{
    return innerType_->unnestPtr();
}

const Ptr* Ptr::unnestInnerstPtr() const
{
    const Ptr* ptr = dynamic_cast<const Ptr*>(innerType_);
    if (ptr)
        return ptr->unnestInnerstPtr();
    else
        return this;
}


me::Reg* Ptr::derefToInnerstPtr(me::Var* var) const
{
    swiftAssert( typeid(*var) == typeid(me::Reg), "TODO: must be a Reg here" );

    if ( innerType_->isActuallyPtr() )
    {
        swiftAssert(var->type_ == me::Op::R_PTR, "must be a ptr");

#ifdef SWIFT_DEBUG
        std::string str = "tmp";
        me::Reg* derefed = me::functab->newReg(me::Op::R_PTR, &str);
#else // SWIFT_DEBUG
        me::Reg* derefed = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

        me::AssignInstr* derefInstr = new me::AssignInstr('^', derefed, (me::Reg*) var);
        me::functab->appendInstr(derefInstr);

        return innerType_->derefToInnerstPtr(derefed);
    }
    // else

    // this is already the innerst pointer
    return (me::Reg*) var;
}

bool Ptr::hasAssignCreate(const TypeList& in, bool hasCreate, location loc) const
{
    std::string methodStr = hasCreate ? "constructer" : "assignment";

    if ( in.size() != 1 )
    {
        errorf(loc, "a 'ptr' %s takes exactly one argument", methodStr.c_str() );
        return false;
    }

    if ( !check(in[0]) )
    {
        errorf( loc_, "types do not match in 'ptr' %s", methodStr.c_str() );
        return false;
    }

    return true;
}

std::string Ptr::toString() const
{
    std::ostringstream oss;
    oss << "ptr{" << innerType_->toString() << '}';

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * init statics
 */

me::Struct* Container::meContainer_ = 0;
me::Member* Container::meContainerPtr_ = 0;
me::Member* Container::meContainerSize_ = 0;

/*
 * constructor
 */

Container::Container(int modifier, Type* innerType, location loc)
    : NestedType(modifier, innerType, loc)
{}

/*
 * virtual methods
 */

me::Op::Type Container::toMeType() const
{
    return me::Op::R_MEM;
}

me::Var* Container::createVar(const std::string* id /*= 0*/) const
{
#ifdef SWIFT_DEBUG
    return me::functab->newMemVar(meContainer_, id);
#else // SWIFT_DEBUG
    return me::functab->newMemVar(meContainer_);
#endif // SWIFT_DEBUG
}

bool Container::isAtomic() const
{
    return false;
}

bool Container::isInternalAtomic() const
{
    return modifier_ == Token::CONST_REF || modifier_ == Token::REF;
}

bool Container::isActuallyPtr() const
{
    return false;
}

size_t Container::sizeOf() const
{
    return meContainer_->sizeOf();
}

me::Reg* Container::derefToInnerstPtr(me::Var* var) const
{
    return loadPtr(var);
}

bool Container::hasAssignCreate(const TypeList& in, bool hasCreate, location loc) const
{
    std::string methodStr = hasCreate ? "constructer" : "assignment";

    if ( in.size() != 1 )
    {
        errorf(loc, "a '%s' container %s takes exactly one argument", 
                containerStr().c_str(),
                methodStr.c_str() );
        return false;
    }

    if (hasCreate)
    {
        if ( in[0]->isIndex() )
            return true;

        else
        {
            errorf( loc_, "'%s' container constructor expects one argument of class 'index'", 
                    containerStr().c_str() );
        }
    }
    else if ( !check(in[0]) )
    {
        errorf( loc_, "types do not match in '%s' container %s", 
                containerStr().c_str(),
                methodStr.c_str() );
        return false;
    }

    return true;
}

const BaseType* Container::unnestPtr() const
{
    swiftAssert(false, "unreachable code");
    return 0;
}

const Ptr* Container::unnestInnerstPtr() const
{
    swiftAssert(false, "unreachable code");
    return 0;
}

std::string Container::toString() const
{
    std::ostringstream oss;
    oss << containerStr() << '{' << innerType_->toString() << '}';

    return oss.str();
}

/*
 * static methods
 */

void Container::initMeContainer()
{
    // TODO make the choice of R_UINT64 arch independent

#ifdef SWIFT_DEBUG

    meContainer_ = me::functab->newStruct("Container");
    me::functab->enterStruct(meContainer_);

    meContainerPtr_  = me::functab->appendMember( new me::AtomicAggregate(me::Op::R_PTR), "ptr" );
    meContainerSize_ = me::functab->appendMember( new me::AtomicAggregate(me::Op::R_UINT64), "size");

#else // SWIFT_DEBUG

    meContainer_ = me::functab->newStruct();
    me::functab->enterStruct(meContainer_);

    meContainerPtr_  = me::functab->appendMember( new me::AtomicAggregate(me::Op::R_PTR) );
    meContainerSize_ = me::functab->appendMember( new me::AtomicAggregate(me::Op::R_UINT64) );

#endif // SWIFT_DEBUG

    me::functab->leaveStruct();
    //meContainer_->analyze();
}

me::Struct* Container::getMeStruct()
{
    return meContainer_;
}

me::StructOffset* Container::createContainerPtrOffset()
{
    return new me::StructOffset(meContainer_, meContainerPtr_);
}

me::StructOffset* Container::createContainerSizeOffset()
{
    return new me::StructOffset(meContainer_, meContainerPtr_);
}

size_t Container::getContainerSize()
{
    return meContainer_->sizeOf();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Array::Array(int modifier, Type* innerType, location loc)
    : Container(modifier, innerType, loc)
{}

/*
 * virtual methods
 */

Array* Array::clone() const
{
    return new Array(modifier_, innerType_->clone(), location() ); // TODO location
}

std::string Array::containerStr() const
{
    return "array";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Simd::Simd(int modifier, Type* innerType, location loc)
    : Container(modifier, innerType, loc)
{}

/*
 * virtual methods
 */

Simd* Simd::clone() const
{
    return new Simd(modifier_, innerType_->clone(), location() ); // TODO location
}

std::string Simd::containerStr() const
{
    return "simd";
}

/*
 * further methods
 */

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
