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

#ifndef SWIFT_SYNTAXTREE_H
#define SWIFT_SYNTAXTREE_H

#include <iostream>
#include <string>
#include <vector>

#include "utils/assert.h"
#include "fe/parser.h"

/*
 * forward declaration
 */

namespace me {
    class Op;
    class Var;
    class Reg;
}

namespace swift {

class Class;
class Var;

typedef std::vector<me::Op*> PlaceList;

//------------------------------------------------------------------------------

/**
 * This is the base class for all nodes in the SyntaxTree.
 * It knows its \a parent_ and its \a line_.
 */
struct Node
{
    enum
    {
        NO_LINE = -1 ///< if this node does not map to a line number -1 is used
    };

    int line_;///< the line number which this node is mapped to

    /*
     * constructor and destructor
     */

    Node(int line = NO_LINE);
    virtual ~Node() {}

    /*
     * virtual methods
     */

    virtual std::string toString() const = 0;

};

//------------------------------------------------------------------------------

/**
 * A Symbol is a Node which also has an \a id_ and a \a toString method.
 */
struct Symbol : public Node
{
    std::string* id_;
    Symbol* parent_; ///< 0 if root.

    /*
     * constructor and destructor
     */

    Symbol(std::string* id, Symbol* parent, int line = NO_LINE);
    virtual ~Symbol();

    /*
     * virtual methods
     */

    /// Returns the \a id_ of this Symbol.
    virtual std::string toString() const;

    /*
     * further methods
     */

    /**
     * Returns the full name of the symbol.
     */
    std::string getFullName() const;
};

//------------------------------------------------------------------------------

class TypeNode : public Node
{
public:

    /*
     * constructor and destructor
     */

    TypeNode(Type* type, int line = NO_LINE);
    virtual ~TypeNode();

    /*
     * virtual methods
     */
    
    virtual bool analyze() = 0;

    /*
     * further methods
     */

    const Type* getType() const;
    const me::Op* getPlace() const;
    me::Op* getPlace();

protected:

    /*
     * data
     */

    Type* type_;
    me::Op* place_;
};

//------------------------------------------------------------------------------

/**
 * This class capsulates the root Module and the root to \a analyze.
 */
struct SyntaxTree
{
    Module* rootModule_;

    /*
     * destructor
     */

    ~SyntaxTree();

    /*
     * further methods
     */

    bool analyze();
};

//------------------------------------------------------------------------------

extern SyntaxTree* syntaxtree;

} // namespace swift

#endif // SWIFT_SYNTAXTREE_H
