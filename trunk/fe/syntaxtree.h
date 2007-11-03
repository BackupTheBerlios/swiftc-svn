#ifndef SWIFT_SYNTAXTREE_H
#define SWIFT_SYNTAXTREE_H

#include <iostream>
#include <string>

#include "utils/assert.h"
#include "fe/parser.h"

// forward declaration
struct Class;

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
    constructor and destructor
*/

    Node(int line = NO_LINE);
    virtual ~Node() {}
};

//------------------------------------------------------------------------------

/**
 * A Symbol is a Node which also has an \a id_ and a \a toString method.
 */
struct Symbol : public Node
{
    std::string* id_;
    Node*   parent_; ///< 0 if root.

/*
    constructor and destructor
*/

    Symbol(std::string* id, Symbol* parent, int line = NO_LINE);
    ~Symbol();

/*
    further methods
*/
    /// Returns the \a id_ of this Symbol.
    std::string toString() const;

    /**
     * Returns the full name of the symbol.
     */
    std::string getFullName() const;
};

//------------------------------------------------------------------------------

/**
 * This class capsulates the root Module and the root to \a analyze.
 */
struct SyntaxTree
{
    Module* rootModule_;

/*
    destructor
*/

    ~SyntaxTree();

/*
    further methods
*/

    bool analyze();
};

//------------------------------------------------------------------------------

extern SyntaxTree* syntaxtree;

#endif // SWIFT_SYNTAXTREE_H
