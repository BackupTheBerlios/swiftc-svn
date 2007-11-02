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
    /// NULL if root
    Node*   parent_;

/*
    constructor and destructor
*/

    Node(int line = NO_LINE, Node* parent = 0)
        : line_(line)
        , parent_(parent)
    {}
    virtual ~Node() {}
};

//------------------------------------------------------------------------------

/**
 * This class capsulates the root Module and the root to \a analyze.
 */
struct SyntaxTree
{
    Module* rootModule_;

    ~SyntaxTree();

    bool analyze();
};

//------------------------------------------------------------------------------

extern SyntaxTree* syntaxtree;

#endif // SWIFT_SYNTAXTREE_H
