#ifndef SWIFT_MODULE_H
#define SWIFT_MODULE_H

#include <map>

#include "utils/stringptrcmp.h"

#include "fe/syntaxtree.h"
#include "fe/parser.h"


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
    ~Module();

    std::string toString() const;
    bool analyze();
};


#endif // SWIFT_MODULE_H
