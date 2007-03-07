#ifndef SWIFT_SYNTAXTREE_H
#define SWIFT_SYNTAXTREE_H

#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include "tokens.h"

namespace swift
{

// forward declaration
struct Class;

//------------------------------------------------------------------------------

struct StringPtrCmp
{
    bool operator () (const std::string* str1, const std::string* str2)
    {
        return *str1 < *str2;
    }
};

//------------------------------------------------------------------------------

struct Node
{
    int line_;
    /// NULL if root
    Node*   parent_;

    Node(int line = -1, Node* parent = 0)
        : line_(line)
        , parent_(parent)
    {}
    virtual ~Node() {};

    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------

struct SymTabEntry : public Node
{
    std::string* id_;

    SymTabEntry(std::string* id, int line = -1, Node* parent = 0)
        : Node(line, parent)
        , id_(id)
    {}
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

    Module(std::string* id, int line = -1, Node* parent = 0)
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
};

//------------------------------------------------------------------------------

#define SWIFT_TO_STRING_ERROR default: swiftAssert(false, "illegal case value"); return "";

} // namespace swift

#endif // SWIFT_SYNTAXTREE_H
