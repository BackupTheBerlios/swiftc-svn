#include "var.h"

#include "fe/type.h"

/*
    constructor and destructor
*/

Var::Var(Type* type, std::string* id)
    : type_(type)
    , id_(id)
{}

virtual ~Var()
{
    delete id_;
    delete type_;
}

/*
    further methods
*/

std::string Local::toString() const
{
    return *id_;
}

//------------------------------------------------------------------------------

/*
    constructor
*/

Local::Local(Type* type, std::string* id)
    : Var(type, id)
    , regNr_(0) // start with an invalid value
{}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

Param::Param(Kind kind, Type* type, std::string* id /*= 0*/)
    : Var(type, id)
    , kind_(kind)
{}

/*
    further methods
*/

bool Param::check(const Param* param1, const Param* param2)
{
    // first check whether kind_ fits
    if (param1->kind_ != param2->kind_)
        return false;

    // check whether type fits
    if ( Type::check(param1->type_, param2->type_) )
        return true;

    // else
    return false;
}
