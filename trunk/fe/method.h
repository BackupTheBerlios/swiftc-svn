#ifndef SWIFT_METHOD_H
#define SWIFT_METHOD_H

#include <map>
#include <string>

#include "fe/class.h"
#include "fe/proc.h"

// forward declaration
struct Param;

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
