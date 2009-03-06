#ifndef SWIFT_EXPR_LIST_H
#define SWIFT_EXPR_LIST_H

#include "fe/syntaxtree.h"

namespace swift {

// forward declarations
class Expr;

//------------------------------------------------------------------------------

/**
 * @brief This is a list of expresoins.
 *
 * This comma sperated list of \a Expr instances used in 
 * function/method/contructor calls.
 */
class ExprList : public Node
{
public:

    /*
     * constructor and destructor
     */

    ExprList(int modifier, Expr* expr, ExprList* next = 0, int line = NO_LINE);
    virtual ~ExprList();

    /*
     * virtual methods
     */

    virtual bool analyze();

private:

    /*
     * data
     */

    int modifier_;   ///< \a INOUT or 0 if no modifier is used.
    Expr* expr_;     ///< the Expr owned by this instance
    ExprList* next_; ///< next element in the list
};

} // namespace swift

#endif // SWIFT_EXPR_LIST_H
