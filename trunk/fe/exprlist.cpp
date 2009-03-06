#include "fe/exprlist.h"

#include "fe/expr.h"

/*
 * constructor and destructor
 */

ExprList::ExprList(int modifier, Expr* expr, ExprList* next /*= 0*/, int line /*= NO_LINE*/)
    : Node(line)
    , modifier_(modifier)
    , expr_(expr)
    , next_(next)
{}

ExprList::~ExprList()
{
    delete expr_;
    if (next_)
        delete next_;
}

/*
 * virtual methods
 */

bool ExprList::analyze()
{
    bool result = true;

    // for each expr in this list
    for (ExprList* iter = this; iter != 0; iter = iter->next_)
        result &= iter->expr_->analyze();

    return result;
}
