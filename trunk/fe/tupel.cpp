#include "fe/tupel.h"

#include "fe/expr.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Tupel::Tupel(Tupel* next, int line /*= NO_LINE*/)
    : Node(line)
    , next_(next)
{}

Tupel::~Tupel()
{
    if (next_)
        delete next_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

DeclTupelElem::DeclTupelElem(Type* type, std::string* id, Tupel* next, int line /*= NO_LINE*/)
    : Tupel(next, line)
    , type_(type)
    , id_(id)
{}

DeclTupelElem::~DeclTupelElem()
{
    delete type_;

    if (next_)
        delete next_;
}

/*
 * virtual methods
 */

bool DeclTupelElem::analyze()
{
    bool result = true;

    // for each expr in this list
    for (ExprList* iter = this; iter != 0; iter = iter->next_)
        result &= iter->expr_->analyze();

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

ExprTupelElem::ExprTupelElem(Expr* expr, Tupel* next, int line /*= NO_LINE*/)
    : Tupel(next, line)
    , expr_(expr)
{}

ExprTupelElem::~ExprTupelElem()
{
    delete expr_;

    if (next_)
        delete next_;
}

/*
 * virtual methods
 */

bool ExprTupelElem::analyze()
{
    bool result = true;

    // for each expr in this list
    for (ExprList* iter = this; iter != 0; iter = iter->next_)
        result &= iter->expr_->analyze();

    return result;
}

} // namespace swift
