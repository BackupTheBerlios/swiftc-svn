#ifndef SWIFT_PROC
#define SWIFT_PROC

#include <map>
#include <string>

#include "utils/list.h"
#include "utils/stringhelper.h"

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
    const Param* findFirstOut(size_t& numIn) const;

    /**
     * Finds first RES or RES_INOUT Param in the Params List.
     *
     * @return The first RES or RES_INOUT Param, 0 if there exists no outgoing
     *      Param.
     */
    const Param* findFirstOut() const;

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
 * This class abstracts a a method, routine etc. It knows has a
 * List of Param objects and has some useful methods.
*/
struct Proc
{
};

//------------------------------------------------------------------------------

/**
 * This represents the Scope of a Proc. It can have child scopes and knows of
 * its \a locals_ which are defined there.
 */
struct Scope
{
    typedef List<Scope*> ScopeList;
    typedef std::map<std::string*, Local*, StringPtrCmp> LocalMap;
    typedef std::map<int, Local*> VarNrMap;

    Scope* parent_;         ///< 0 if root.
    ScopeList childScopes_; ///< List of child scopes.
    LocalMap locals_;       ///< Map of locals in this scope sorted by identifier.
    VarNrMap varNrs_;       ///< Map of locals in this scope sorted by varNr.

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

    /// Returns the local by varNr, of this scope or parent scopes. 0 if nothing was found.
    Local* lookupLocal(int varNr);
};

#endif //SWIFT_PROC
