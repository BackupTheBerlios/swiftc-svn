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

#ifndef SWIFT_PROC
#define SWIFT_PROC

#include <map>
#include <string>

#include "utils/list.h"
#include "utils/stringhelper.h"

namespace swift {

// forward declaraions
struct Param;
struct Statement;
struct Scope;
struct Local;
struct Method;

//------------------------------------------------------------------------------

/**
 * This class abstracts a signature of a method, routine etc. It has a
 * List of Param objects and some useful methods.
*/
struct Sig
{
    typedef List<Param*> Params;

    /**
     * This list stores the Param objects. The parameters are sorted from left
     * to right as given in the Sig of the procedure. Thus first are the
     * ARG then the RES and RES_INOUT (in no special order) Param objects.
     */
    Params params_;

/*
    destructor
*/

    ~Sig();

/*
    methods
*/

    /// Analyses this Sig for correct syntax.
    bool analyze() const;

    /// Check whether two given signatures fit.
    static bool check(const Sig& sig1, const Sig& sig2);

    /**
     * Check whether the ingoing part of the given Sig matches.
     *
     * @param inSig The Sig which should be checked. It is assumed that this
     *      this Sig only has an ingoing part.
     * @return true -> it fits, flase -> otherwise.
     */
    bool checkIngoing(const Sig& inSig) const;

    /**
     * Finds first RES or RES_INOUT Param in the Params List.
     *
     * @param numIn The number of ingoing parameters.
     * @return The first RES or RES_INOUT Param, 0 if there exists no outgoing
     *      Param.
     */
    const Params::Node* findFirstOut(size_t& numIn) const;

    /**
     * Finds first RES or RES_INOUT Param in the Params List.
     *
     * @return The first RES or RES_INOUT Param, 0 if there exists no outgoing
     *      Param.
     */
    const Params::Node* findFirstOut() const;

    /**
     * Find a Param by name.
     *
     * @return The Param or 0 if it was not found.
     */
    Param* findParam(std::string* id);


    std::string toString() const;
};

#define PARAMS_EACH(iter, params) \
    LIST_EACH(Sig::Params, iter, params)

#define PARAMS_CONST_EACH(iter, params) \
    LIST_CONST_EACH(Sig::Params, iter, params)

//------------------------------------------------------------------------------

/**
 * This represents the Scope of a Proc. It can have child scopes and knows of
 * its \a locals_ which are defined there.
 */
struct Scope
{
    typedef List<Scope*> ScopeList;
    typedef std::map<std::string*, Local*, StringPtrCmp> LocalMap;

    Scope* parent_;         ///< 0 if root.
    ScopeList childScopes_; ///< List of child scopes.
    LocalMap locals_;       ///< Map of locals in this scope sorted by identifier.

/*
    constructor and destructor
*/

    Scope(Scope* parent);
    ~Scope();

/*
    further methods
*/

    /// Returns the local by the id, of this scope or parent scopes. 0 if nothing was found.
    Local* lookupLocal(std::string* id);
};

} // namespace swift

#endif //SWIFT_PROC
