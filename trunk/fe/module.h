#ifndef SWIFT_MODULE_H
#define SWIFT_MODULE_H

#include <map>

#include "utils/list.h"
#include "utils/stringhelper.h"

#include "fe/syntaxtree.h"
#include "fe/parser.h"

namespace swift {

// forward declarations
struct Definition;

//------------------------------------------------------------------------------

/**
 * A Module consits of several \a definitions_. Class objects are stored inside
 * a map.
 */
struct Module : public Symbol
{
    typedef List<Definition*> DefinitionList;
    typedef std::map<std::string*, Class*, StringPtrCmp> ClassMap;

    DefinitionList definitions_; ///< Linked List of Definition objects.
    ClassMap classes_; ///< Each Module knows all its classes, sorted by the identifier.

/*
    constructor and destructor
*/

    Module(std::string* id, int line = NO_LINE);
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
    Definition(std::string* id, Symbol* parent, int line = NO_LINE);

/*
    further methods
*/

    virtual bool analyze() = 0;
};

} // namespace swift

#endif // SWIFT_MODULE_H
