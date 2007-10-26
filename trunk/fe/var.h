#ifndef SWIFT_VAR_H
#define SWIFT_VAR_H

#include <string>

// forward declaration
struct Type;

//------------------------------------------------------------------------------

/**
 * This class is the base for a Local or a Param. It is the return value
 * for symtab lookups.
 */
struct Var
{
    Type* type_;
    std::string* id_;

/*
    constructor and destructor
*/

    Var(Type* type, std::string* id);
    virtual ~Var();

/*
    further methods
*/

    std::string toString() const;
};

//------------------------------------------------------------------------------

/**
 * This class represents either an ordinary local variable used by the
 * programmer or a compiler generated variable used to store a temporary value.
 */
struct Local : struct Var
{
    /**
     * regNr_ > 0   a TEMP with nr regNr <br>
     * regNr_ = 0   invalid (is reserved for literals) <br>
     * regNr_ < 0   a VAR with nr -regNr <br>
     *
     * TEMP -> a variable created by the compiler so it is only
     *      defined once (it already has SSA property) <br>
     * VAR -> an ordinary variable defined by the programmer
     *      (SSA form must be generated for this Local)
     */
    int regNr_;

/*
    constructor
*/

    Local(Type* type, std::string* id);
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

    Param(Kind kind, Type* type, std::string* id = 0);

/*
    further methods
*/

    /// Check whether the type of both Parameter objects fit.
    static bool check(const Param* param1, const Param* param2);
};

#endif // SWIFT_VAR_H
