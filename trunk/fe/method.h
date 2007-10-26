#ifndef SWIFT_METHOD_H
#define SWIFT_METHOD_H

#include <map>
#include <string>

#include "utils/stringhelper.h"

#include "fe/class.h"
#include "fe/syntaxtree.h"
#include "fe/type.h"
#include "fe/statement.h"

#include "me/pseudoreg.h"

// forward declaration
struct Param;

//------------------------------------------------------------------------------

/**
 * This class represents a Method of a Class. Because there may be one day
 * routines all the logic is handled in Proc in order to share code.
*/
struct Method : public ClassMember
{
    int methodQualifier_; ///< Either READER, WRITER, ROUTINE, CREATE or OPERATOR.
    Proc proc_;           ///< Handles the logic.

/*
    constructor and destructor
*/

    Method(int methodQualifier, std::string* id, int line = NO_LINE, Node* parent = 0);
    virtual ~Method();

/*
    further methods
*/

    void appendParam(Param* param);

    virtual std::string toString() const;
    virtual bool analyze();
};

#endif // SWIFT_METHOD_H
