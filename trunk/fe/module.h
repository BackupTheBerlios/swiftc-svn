/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SWIFT_MODULE_H
#define SWIFT_MODULE_H

#include <map>

#include "utils/list.h"
#include "utils/stringhelper.h"

#include "fe/auto.h"
#include "fe/location.hh"
#include "fe/syntaxtree.h"

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
    typedef std::map<const std::string*, Class*, StringPtrCmp> ClassMap;

    DefinitionList definitions_; ///< Linked List of Definition objects.
    ClassMap classes_; ///< Each Module knows all its classes, sorted by its identifier.

    /*
     * constructor and destructor
     */

    Module(std::string* id, location loc);
    virtual ~Module();

    /*
     * further methods
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
     * constructor
     */

    Definition(std::string* id, Symbol* parent, location loc);

    /*
     * further methods
     */

    virtual bool analyze() = 0;
};

} // namespace swift

#endif // SWIFT_MODULE_H
