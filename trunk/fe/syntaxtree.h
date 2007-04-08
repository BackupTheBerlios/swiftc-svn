#ifndef SWIFT_SYNTAXTREE_H
#define SWIFT_SYNTAXTREE_H

#include <string>

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
     * used to count the revision of this variable for SSA form <br>
     * 0 -> first revision, only used for phi-functions <br>
     * -1 -> this is already a revision <br>
     */
    int revision_;

    SymTabEntry(std::string* id, int line = NO_LINE, Node* parent = 0)
        : Node(line, parent)
        , id_(id)
        , revision_(0) // start with revision 0
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
