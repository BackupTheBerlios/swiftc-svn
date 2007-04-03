#ifndef SWIFT_SYNTAXTREE_H
#define SWIFT_SYNTAXTREE_H

#include <map>
#include <string>
#include <vector>

#include "utils/stringptrcmp.h"
#include "fe/parser.h"


// forward declaration
struct Class;

//------------------------------------------------------------------------------

struct Node
{
    enum {
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
    virtual ~Node() {};

    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------

struct SymTabEntry : public Node
{
    enum {
        REVISED_VAR = -1 /// if revision == -1 this var is already a revised one
    };

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

    std::string extractOriginalId();
};

//------------------------------------------------------------------------------

struct Definition : public SymTabEntry
{
    Definition* next_;

    Definition(std::string* id, int line, Node* parent = 0)
        : SymTabEntry(id, line, parent)
        , next_(0)
    {}
    ~Definition()
    {
        delete next_;
    }
    virtual bool analyze() = 0;
};

//------------------------------------------------------------------------------

struct Module : public SymTabEntry
{
    typedef std::map<std::string*, Class*, StringPtrCmp> ClassMap;

    Definition* definitions_;

    ClassMap    classes_;

    Module(std::string* id, int line = NO_LINE, Node* parent = 0)
        : SymTabEntry(id, line, parent)
    {}
    ~Module()
    {
        delete definitions_;
    }

    std::string toString() const;
    bool analyze();
};

//------------------------------------------------------------------------------

struct SyntaxTree
{
    Module* rootModule_;

    bool analyze();

    /**
     * Destroy the syntax tree recursivly. Do not delete id_s. They are needed
     * in the next pass.
     */
    void destroy()
    {
        delete rootModule_;
    }
};

//------------------------------------------------------------------------------

extern SyntaxTree syntaxtree;

#endif // SWIFT_SYNTAXTREE_H
