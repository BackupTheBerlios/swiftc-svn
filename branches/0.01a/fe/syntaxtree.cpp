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

#include "fe/syntaxtree.h"

#include <iostream>
#include <sstream>
#include <stack>

#include "utils/assert.h"

#include "fe/module.h"
#include "fe/symtab.h"
#include "fe/type.h"

#include "me/ssa.h"

namespace swift {

SyntaxTree* syntaxtree = 0;

//------------------------------------------------------------------------------

/*
 * constructor
 */

Node::Node(location loc)
    : loc_(loc)
{}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Symbol::Symbol(std::string* id, Symbol* parent, location loc)
    : Node(loc)
    , id_(id)
    , parent_(parent)
{}

Symbol::~Symbol()
{
    delete id_;
}

/*
 * further methods
 */

std::string Symbol::toString() const
{
    return *id_;
}

std::string Symbol::getFullName() const
{
    // build full class name with module names: module1.module2.Class1.Class2 etc
    std::stack<std::string*> idStack;

    // put all identifiers on the stack
    for (const Symbol* iter = this; iter != 0; iter = iter->parent_)
        idStack.push(iter->id_);

    std::ostringstream oss;

    // and build string
    while ( !idStack.empty() )
    {
        oss << *idStack.top();
        idStack.pop();

        if ( !idStack.empty() )
            oss << '.';
    }

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

TypeNode::TypeNode(Type* type, location loc)
    : Node(loc)
    , type_(type)
    , storeNecessary_(false)
    , simdLength_(0)
{}

TypeNode::~TypeNode()
{
    delete type_;
}

/*
 * virtual methods
 */

void TypeNode::simdAnalyze(SimdAnalysis& simdAnalysis)
{
}

void TypeNode::setSimdLength(int simdLength)
{
    simdLength_ = simdLength;
}

/*
 * further methods
 */

const Type* TypeNode::getType() const
{
    return type_;
}

const me::Op* TypeNode::getPlace() const
{
    return place_;
}

me::Op* TypeNode::getPlace()
{
    return place_;
}

bool TypeNode::isStoreNecessary() const
{
    return storeNecessary_;
}

//------------------------------------------------------------------------------

/*
 * destructor
 */

SyntaxTree::~SyntaxTree()
{
    delete rootModule_;
}

/*
 * further methods
 */

bool SyntaxTree::analyze()
{
    symtab->reset();
    symtab->enterModule();

    bool result = rootModule_->analyze();

    symtab->leaveModule();

    return result;
}

} // namespace swift
