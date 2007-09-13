#ifndef SWIFT_MODULE_H
#define SWIFT_MODULE_H

#include <map>

#include "utils/list.h"
#include "utils/stringhelper.h"

#include "fe/syntaxtree.h"
#include "fe/parser.h"


struct Definition : public SymTabEntry
{
    Definition(std::string* id, int line, Node* parent = 0)
        : SymTabEntry(id, line, parent)
    {}

    virtual bool analyze() = 0;
};

//------------------------------------------------------------------------------

struct Module : public SymTabEntry
{
    typedef List<Definition*> DefinitionList;
    DefinitionList definitions_;

    typedef std::map<std::string*, Class*, StringPtrCmp> ClassMap;
    ClassMap    classes_;

    Module(std::string* id, int line = NO_LINE, Node* parent = 0)
        : SymTabEntry(id, line, parent)
    {}
    ~Module();

    std::string toString() const;
    bool analyze();
};


#endif // SWIFT_MODULE_H
