#ifndef SWIFT_PROC
#define SWIFT_PROC

#include <string>

#include "utils/list.h"

// forward declaraions
struct Param;
struct Statement;
struct Scope;
struct Method;

//------------------------------------------------------------------------------

/**
 * This class abstracts a Signature of a method, routine etc. It has a
 * List of Param objects and some useful methods.
*/
struct Sig
{
    typedef List<Param*> Params;

    /**
     * This list stores the Param objects. The parameters are sorted from left
     * to right as given in the Signature of the procedure. Thus first are the
     * ARG then the RES and RES_INOUT (in no special order) Param objects.
     */
    Params params_;

/*
    methods
*/

    /// Analyses this Signature for correct syntax.
    bool analyze() const;

    /// Check whether two given signatures fit.
    static bool check(const Signature& sig1, const Signature& sig2);

    /**
     * Finds first RES or RES_INOUT Param in the Params List.
     *
     * @return The first RES or RES_INOUT Param, 0 if there exists no outgoing
     *      Param.
     */
    const Param* findFirstOut() const;

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
    std::string* id_;       ///< The identifier of this Proc.
    Statement* statements_; ///< The statements_ inside this Proc.
    Scope* rootScope_;      ///< The root Scope where vars of this Proc are stored.
    Sig sig_;               ///< The signature of this Proc.

    enum Kind
    {
        METHOD,
        ROUTINE
    };

    Kind kind_; ///< Is this an aggregate of a Method or a Routine?

    union
    {
        Method* method_; ///< Used if this is an aggregate of a Method.
    };

/*
    constructor and destructor
*/

    Proc(std::string* id, Method* method);
    ~Proc();

/*
    getters and setters
*/

    Method* getMethod();

/*
    further methods
*/
    /// Appends a Param to \a sig_.
    void appendParam(Param* param);

    /**
     * Find a Param by name.
     *
     * @return The Param or 0 if it was not found.
     */
    Param* findParem(std::string* id);

    /// Analyses this Proc for correct syntax
    bool analyze();

    std::string toString() const;
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
    typedef std::map<int, Local*> RegNrMap;

    Scope* parent_;         ///< 0 if root.
    ScopeList childScopes_; ///< List of child scopes.
    LocalMap locals_;       ///< Map of locals in this scope sorted by identifier.
    RegNrMap regNrs_;       ///< Map of locals in this scope sorted by RegNr.

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

    /// Returns the local by regNr, of this scope or parent scopes. 0 if nothing was found.
    Local* lookupLocal(int);
};

#endif //SWIFT_PROC
