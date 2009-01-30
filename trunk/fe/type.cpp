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

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/symtab.h"

namespace swift {

/*
 * init static
 */

BaseType::TypeMap* BaseType::typeMap_ = 0;

/*
 * constructor and destructor
 */

BaseType::BaseType(std::string* id, int line /*= NO_LINE*/)
    : Node(line)
    , id_(id)
    , builtin_( typeMap_->find(*id) != typeMap_->end() ) // is it a builtin type?
{}

BaseType::~BaseType()
{
    if (id_)
        delete id_;
}

/*
 * further methods
 */

BaseType* BaseType::clone() const
{
    return new BaseType( new std::string(*id_), builtin_ );
}

me::Op::Type BaseType::toMeType() const
{
    if (builtin_)
        return typeMap_->find(*id_)->second;
    else
        return me::Op::R_STACK;

    // TODO ptr support
}

bool BaseType::isBuiltin() const
{
    return builtin_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Type::Type(BaseType* baseType, int pointerCount, int line /*= NO_LINE*/)
    : Node(line)
    , baseType_(baseType)
    , pointerCount_(pointerCount)
{}

Type::~Type()
{
    delete baseType_;
}

/*
 * further methods
 */

Type* Type::clone() const
{
    return new Type(baseType_->clone(), pointerCount_, line_);
};

std::string Type::toString() const
{
    std::ostringstream oss;

    oss << *baseType_->id_;
    for (int i = 0; 0 < pointerCount_; ++i)
        oss << '^';

    return oss.str();
}

bool Type::check(Type* t1, Type* t2)
{
    if (t1->pointerCount_ != t2->pointerCount_)
        return false; // pointerCount_ does not match

    Class* class1 = symtab->lookupClass(t1->baseType_->id_);
    Class* class2 = symtab->lookupClass(t2->baseType_->id_);

    // both classes must exist
    swiftAssert(class1, "first class not found in the symbol table");
    swiftAssert(class2, "second class not found in the symbol table");

    if (class1 != class2) {
        // different pointers -> hence different types
        return false;
    }

    return true;
}

bool Type::validate() const
{
    if ( symtab->lookupClass(baseType_->id_) == 0 )
    {
        errorf( line_, "class '%s' is not defined in this module", baseType_->id_->c_str() );
        return false;
    }

    return true;
}

bool Type::isBool() const
{
    return *baseType_->id_ == "bool";
}

bool Type::isBuiltin() const
{
    return baseType_->isBuiltin() && pointerCount_ == 0;
}

} // namespace swift
