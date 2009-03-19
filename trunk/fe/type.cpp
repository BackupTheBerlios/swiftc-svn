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
#include "me/struct.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor
 */

Type::Type(int modifier, int line /*= NO_LINE*/)
    : Node(line) 
    , modifier_(modifier)
{}

/*
 * virtual methods
 */

bool Type::isAtomic() const
{
    return false;
}

bool Type::isBool() const
{
    return false;
}

//------------------------------------------------------------------------------

/*
 * init statics
 */

BaseType::TypeMap* BaseType::typeMap_ = 0;

/*
 * constructor and destructor
 */

BaseType::BaseType(int modifier, std::string* id, int line /*= NO_LINE*/)
    : Type(modifier, line)
    , id_(id)
    , builtin_( typeMap_->find(*id) != typeMap_->end() ) // is it a builtin type?
{}

BaseType::~BaseType()
{
    if (id_)
        delete id_;
}

/*
 * virtual methods
 */

BaseType* BaseType::clone() const
{
    return new BaseType( modifier_, new std::string(*id_), NO_LINE );
}


bool BaseType::validate() const
{
    if ( symtab->lookupClass(id_) == 0 )
    {
        errorf( line_, "class '%s' is not defined in this module", id_->c_str() );
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
        return me::Op::R_STACK;
}

bool BaseType::isAtomic() const
{
    return builtin_;
}

bool BaseType::isBool() const
{
    return *id_ == "bool";
}

me::Var* BaseType::createVar(const std::string* id /*= 0*/) const
{
    me::Op::Type meType = toMeType();

    me::Var* var;
    if (meType == me::Op::R_STACK)
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

const BaseType* BaseType::getFirstBaseType() const
{
    return this;
}


const Ptr* BaseType::derefToInnerstPtr() const
{
    swiftAssert(false, "unreachable code");
    return 0;
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

Container::Container(int modifier, Type* innerType, int line /*= NO_LINE*/)
    : Type(modifier, line)
    , innerType_(innerType)
{}

Container::~Container()
{
    delete innerType_;
}

/*
 * virtual methods
 */

bool Container::validate() const
{
    return innerType_->validate();
}

/*
 * further methods
 */

Type* Container::getInnerType()
{
    return innerType_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Ptr::Ptr(int modifier, Type* innerType, int line /*= NO_LINE*/)
    : Container(modifier, innerType, line)
{}

Ptr* Ptr::clone() const
{
    return new Ptr(modifier_, innerType_->clone(), NO_LINE);
}

/*
 * virtual methods
 */

bool Ptr::check(const Type* type) const
{
    const Ptr* ptr = dynamic_cast<const Ptr*>(type);

    if (!ptr)
        return false;
    
    return innerType_->check(ptr);
}

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

const BaseType* Ptr::getFirstBaseType() const
{
    return innerType_->getFirstBaseType();
}

const Ptr* Ptr::derefToInnerstPtr() const
{
    if ( typeid(*innerType_) == typeid(Ptr) )
    {
        // TODO deref
        return innerType_->derefToInnerstPtr();
    }
    else
        return this;
}

std::string Ptr::toString() const
{
    std::ostringstream oss;
    oss << "ptr{" << innerType_->toString() << '}';

    return oss.str();
}

} // namespace swift
