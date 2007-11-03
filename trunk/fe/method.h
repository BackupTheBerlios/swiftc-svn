#ifndef SWIFT_METHOD_H
#define SWIFT_METHOD_H

#include <map>
#include <string>

#include "fe/class.h"

// forward declaration
struct Param;

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

    std::string toString() const;
};

#define PARAMS_EACH(iter, params) \
    LIST_EACH(Sig::Params, iter, params)

#define PARAMS_CONST_EACH(iter, params) \
    LIST_CONST_EACH(Sig::Params, iter, params)

//------------------------------------------------------------------------------

/**
 * This class represents a Method of a Class. Because there may be one day
 * routines all the logic is handled in Proc in order to share code.
*/
struct Method : public ClassMember
{
    Proc proc_;           ///< Handles the logic.
    int methodQualifier_; ///< Either READER, WRITER, ROUTINE, CREATE or OPERATOR.

/*
    constructor
*/

    Method(int methodQualifier, std::string* id, int line = NO_LINE, Node* parent = 0);

/*
    further methods
*/

    void appendParam(Param* param);
    virtual bool analyze();
    std::string toString() const;
};

#endif // SWIFT_METHOD_H
