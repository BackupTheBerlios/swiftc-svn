#include "var.h"

#include "fe/type.h"

/*
    constructor and destructor
*/

Var::Var(Type* type, std::string* id, int line /*= NO_LINE*/)
    : type_(type)
    , id_(id)
    , line_(line)
{}

Var::~Var()
{
    delete id_;
    delete type_;
}

/*
    further methods
*/

std::string Var::toString() const
{
    return *id_;
}

//------------------------------------------------------------------------------

/*
    constructor
*/

Local::Local(Type* type, std::string* id, int line /*= NO_LINE*/)
    : Var(type, id, line)
    , regNr_(0) // start with an invalid value
{}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

Param::Param(Kind kind, Type* type, std::string* id /*= 0*/, int line /*= NO_LINE*/)
    : Var(type, id, line)
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

bool Param::analyze() const
{
    return type_->validate();
}
