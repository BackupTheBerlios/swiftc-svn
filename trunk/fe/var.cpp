#include "var.h"

#include "fe/type.h"

#include "me/functab.h"

namespace swift {

/*
    constructor and destructor
*/

Var::Var(Type* type, std::string* id, int varNr, int line /*= NO_LINE*/)
    : Symbol(id, 0, line) // Vars (Params or Locals) never have parents
    , type_(type)
    , varNr_(varNr)
{}

Var::~Var()
{
    delete type_;
}

//------------------------------------------------------------------------------

/*
    constructor
*/

Local::Local(Type* type, std::string* id, int varNr, int line /*= NO_LINE*/)
    : Var(type, id, varNr, line)
{}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

Param::Param(Kind kind, Type* type, std::string* id, int varNr, int line /*= NO_LINE*/)
    : Var(type, id, varNr, line)
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
#ifdef SWIFT_DEBUG
    me::functab->newVar( type_->baseType_->toMeType(), varNr_, id_ );
#else // SWIFT_DEBUG
    me::functab->newVar( type_->baseType_->toMeType(), varNr_ );
#endif // SWIFT_DEBUG

    return type_->validate();
}

} // namespace swift
