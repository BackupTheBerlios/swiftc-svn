#ifndef SWIFT_METHOD_H
#define SWIFT_METHOD_H

#include <map>
#include <string>

#include "fe/class.h"
#include "fe/sig.h"

// forward declaration
struct Param;

//------------------------------------------------------------------------------

/**
 * This class represents a Method of a Class. Because there may be one day
 * routines all the logic is handled in Proc in order to share code.
*/
struct Method : public ClassMember
{
    int methodQualifier_;   ///< Either READER, WRITER, ROUTINE, CREATE or OPERATOR.
    Statement* statements_; ///< The statements_ inside this Proc.
    Scope* rootScope_;      ///< The root Scope where vars of this Proc are stored.
    Sig sig_;               ///< The signature of this Method.

/*
    constructor and destructor
*/

    Method(int methodQualifier, std::string* id, Symbol* parent, int line = NO_LINE);
    ~Method();

/*
    further methods
*/

    virtual bool analyze();
};

#endif // SWIFT_METHOD_H
