#include "statement.h"

#include <sstream>

#include "error.h"
#include "expr.h"
#include "symboltable.h"
#include "type.h"

namespace swift {

Declaration::~Declaration()
{
    delete type_;
}

std::string Declaration::toString() const
{
    std::ostringstream oss;
    oss << type_->toString() << " " << *id_;

    return oss.str();
}

bool Declaration::analyze()
{
    if ( typeid(*type_->baseType_) == typeid(UserType) )
    {
        UserType* userType = (UserType*) type_->baseType_;
        // it is a user defined type - so check whether it has been defined
        if (symtab.lookupClass(userType->id_) == 0)
        {
            errorf( line_, "class '%s' is not defined in this module", type_->baseType_->toString().c_str() );
            return false;
        }
    }

    // everything ok. so insert the local
    symtab.insert( new Local(type_, id_, line_, 0) );

    return true;
}

//------------------------------------------------------------------------------

ExprStatement::~ExprStatement()
{
    delete expr_;
}

bool ExprStatement::analyze()
{
    return expr_->analyze();
}

} // namespace swift
