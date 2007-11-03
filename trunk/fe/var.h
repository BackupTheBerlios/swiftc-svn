#ifndef SWIFT_VAR_H
#define SWIFT_VAR_H

#include <string>

#include "fe/syntaxtree.h"

// forward declaration
struct Type;

//------------------------------------------------------------------------------

/**
 * This class is the base for a Local or a Param. It is the return value
 * for symtab lookups.
 */
struct Var : public Symbol
{
    Type* type_;

    enum
    {
        NO_LINE = -1
    };
/*
    constructor and destructor
*/

    Var(Type* type, std::string* id, int line = NO_LINE);
    virtual ~Var();
};

//------------------------------------------------------------------------------

/**
 * This class represents either an ordinary local variable used by the
 * programmer or a compiler generated variable used to store a temporary value.
 */
struct Local : public Var
{
    /**
     * varNr_ > 0   a TEMP with nr varNr_ <br>
     * varNr_ = 0   invalid (is reserved for literals) <br>
     * varNr_ < 0   a VAR with nr -varNr_ <br>
     *
     * TEMP -> a variable created by the compiler so it is only
     *      defined once (it already has SSA property) <br>
     * VAR -> an ordinary variable defined by the programmer
     *      (SSA form must be generated for this Local)
     */
    int varNr_;

/*
    constructor
*/

    Local(Type* type, std::string* id, int varNr, int line = NO_LINE);
};

//------------------------------------------------------------------------------

/**
 * This class abstracts an parameter of a method, routine etc. It knows its
 * Kind and Type. Optionally it may know its identifier. So when the Parser sees
 * a parameter a Param with \a id_ will be created. If just a Param is needed to
 * check whether a signature fits a Param without \a id_ can be used.
 */
struct Param : public Var
{
    /// What kind of Param is this?
    enum Kind
    {
        ARG,      ///< An ingoing parameter
        RES,      ///< An outgoing parameter AKA result
        RES_INOUT ///< An in- and outgoing paramter/result
    };

    Kind kind_;

/*
    constructor
*/

    Param(Kind kind, Type* type, std::string* id = 0, int line = NO_LINE);

/*
    further methods
*/

    /// Check whether the type of both Param objects fit.
    static bool check(const Param* param1, const Param* param2);

    /// Check whether this Param has a correct Type.
    bool analyze() const;
};

#endif // SWIFT_VAR_H
