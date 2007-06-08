#ifndef SWIFT_SYNTAXTREE_H
#define SWIFT_SYNTAXTREE_H

#include <iostream>
#include <string>

#include "utils/assert.h"
#include "fe/parser.h"


// forward declaration
struct Class;

//------------------------------------------------------------------------------

struct Node
{
    enum
    {
        NO_LINE = -1 /// if this node does not map to a line number -1 is used
    };
    /// the line number which this node is mapped to
    int line_;
    /// NULL if root
    Node*   parent_;

    Node(int line = NO_LINE, Node* parent = 0)
        : line_(line)
        , parent_(parent)
    {}
    virtual ~Node() {}

    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------

struct SymTabEntry : public Node
{
    std::string* id_;
    /**
     * used to keep acount of the current register which holds the value
     * at the moment when in SSA form
    */
    int regNr_;
    /**
     * Magic numbers are used to find out which PseudoRegs belong together
     * when in SSA form.
    */
    int magic_;

    SymTabEntry(std::string* id, int line = NO_LINE, Node* parent = 0)
        : Node(line, parent)
        , id_(id)
        , regNr_(-1) // start with an invalid value
        , magic_(-1)
    {}
    ~SymTabEntry()
    {
        delete id_;
    }
};


//------------------------------------------------------------------------------

struct SyntaxTree
{
    Module* rootModule_;

    ~SyntaxTree();

    bool analyze();
};

//------------------------------------------------------------------------------

extern SyntaxTree* syntaxtree;

#endif // SWIFT_SYNTAXTREE_H
