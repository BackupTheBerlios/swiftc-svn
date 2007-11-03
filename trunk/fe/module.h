#ifndef SWIFT_MODULE_H
#define SWIFT_MODULE_H

#include <map>

#include "utils/list.h"
#include "utils/stringhelper.h"

#include "fe/syntaxtree.h"
#include "fe/parser.h"

// forward declarations
struct Definition;

//------------------------------------------------------------------------------

/**
 * A Module consits of several \a definitions_. Class objects are stored inside
 * a map.
 */
struct Module : public Node
{
    typedef List<Definition*> DefinitionList;
    typedef std::map<std::string*, Class*, StringPtrCmp> ClassMap;

    std::string* id_;
    DefinitionList definitions_; ///< Linked List of Definition objects.
    ClassMap classes_; ///< Each Module knows all its classes, sorted by the identifier.

/*
    constructor and destructor
*/

    Module(std::string* id, int line = NO_LINE, Node* parent = 0);
    virtual ~Module();

/*
    further methods
*/

    bool analyze();
    std::string toString() const;
};

//------------------------------------------------------------------------------

/**
 * A Definition is either a Class or other things which are still TODO
 */
struct Definition : public Symbol
{
/*
    constructor
*/
    Definition(std::string* id, int line, Node* parent = 0);

/*
    further methods
*/

    virtual bool analyze() = 0;
};

#endif // SWIFT_MODULE_H
