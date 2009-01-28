#ifndef SWIFT_VAR_H
#define SWIFT_VAR_H

#include <string>

#include "fe/syntaxtree.h"

#include "me/functab.h"

namespace me {
    struct Reg;
}

namespace swift {

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

    me::Reg* reg_;

    enum
    {
        NO_LINE = -1
    };

    /*
     * constructor and destructor
     */

    Var(Type* type, me::Reg* reg, std::string* id, int line = NO_LINE);
    virtual ~Var();
};

//------------------------------------------------------------------------------

/**
 * This class represents either an ordinary local variable used by the
 * programmer or a compiler generated variable used to store a temporary value.
 */
struct Local : public Var
{
    /*
     * constructor
     */

    Local(Type* type, me::Reg* reg, std::string* id, int line = NO_LINE);
};

//------------------------------------------------------------------------------

/**
 * This class abstracts an parameter of a method, routine etc. 
 *
 * It knows its Kind and Type. Optionally it may know its identifier. So when
 * the Parser sees a parameter a Param with \a id_ will be created. If just a
 * Param is needed to check whether a signature fits a Param without \a id_ can
 * be used.
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
     * constructor
     */

    Param(Kind kind, Type* type, std::string* id, int line = NO_LINE);

    /*
     * further methods
     */

    /// Check whether the type of both Param objects fit.
    static bool check(const Param* param1, const Param* param2);

    /// Check whether this Param has a correct Type.
    bool validateAndCreateReg(); 
};

} // namespace swift

#endif // SWIFT_VAR_H
