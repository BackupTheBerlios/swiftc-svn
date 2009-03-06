#ifndef SWIFT_TUPEL_H
#define SWIFT_TUPEL_H

#include "fe/syntaxtree.h"

namespace swift {

/*
 * forward declarations
 */
class Expr;
class Type;

//------------------------------------------------------------------------------

/**
 * @brief This class represents a comma sperated list of Expr instances used in
 * function/method/contructor calls.
 *
 * This is actually not a Expr, but belongs to expressions
 * so it is in this file.
 */
class Tupel : public Node
{
public:

    /*
     * constructor and destructor
     */

    Tupel(Tupel* next, int line = NO_LINE);
    virtual ~Tupel();

    /*
     * virtual methods
     */

    virtual bool analyze();

protected:

    /*
     * data
     */

    Tupel* next_; ///< next element in the list
};

//------------------------------------------------------------------------------

class DeclTupelElem : public Tupel
{
public:

    /*
     * contructor and destructor
     */

    DeclTupelElem(Type* type, std::string* id, Tupel* next, int line = NO_LINE);
    virtual ~DeclTupelElem();

private:

    /*
     * data
     */

    Type* type_;
    std::string* id_;
};

//------------------------------------------------------------------------------

class ExprTupelElem : public Tupel
{
public:

    ExprTupelElem(Expr* expr, Tupel* next, int line = NO_LINE);
    virtual ~ExprTupelElem();

    /*
     * virtual methods
     */

    virtual bool analyze();

private:

    /*
     * data
     */

    Expr expr_;
};

} // namespace swift

#endif // SWIFT_TUPEL_H
