#include "sig.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/class.h"
#include "fe/method.h"
#include "fe/parser.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/var.h"

#include "me/functab.h"

namespace swift {

//------------------------------------------------------------------------------

/*
    destructor
*/

Sig::~Sig()
{
    PARAMS_EACH(iter, params_)
        delete iter->value_;
}

/*
    further methods
*/

bool Sig::analyze() const
{
    bool result = true;

    PARAMS_CONST_EACH(iter, params_)
        result &= iter->value_->analyze();

    return result;
}

bool Sig::check(const Sig& sig1, const Sig& sig2)
{
    // if the sizes do not match the Sig is obviously different
    if ( sig1.params_.size() != sig2.params_.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    const Sig::Params::Node* param1 = sig1.params_.first();
    const Sig::Params::Node* param2 = sig2.params_.first();

    while (result && param1 != sig1.params_.sentinel())
    {
        result = Param::check(param1->value_, param2->value_);

        // traverse both nodes to the next node
        param1 = param1->next();
        param2 = param2->next();
    }

    return result;
}

bool Sig::checkIngoing(const Sig& insig) const
{
    // count the number of ingoing paramters
    size_t numIn;
    // and find first outcoming parameter of this signatur
    const Param* firstOut = findFirstOut(numIn)->value_;

    // if the sizes do not match the signature is obviously different
    if ( numIn != insig.params_.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    const Sig::Params::Node* thisParam = params_.first();
    const Sig::Params::Node* inParam   = insig.params_.first();

    while (result && inParam != insig.params_.sentinel() && thisParam->value_ != firstOut)
    {
        result = Param::check(thisParam->value_, inParam->value_);

        // traverse both nodes to the next node
        thisParam = thisParam->next();
        inParam   = inParam->next();
    }

    return result;
}

const Sig::Params::Node* Sig::findFirstOut(size_t& numIn) const
{
    numIn = 0;
    // shall hold the result
    Param* firstOut = 0;

    PARAMS_CONST_EACH(iter, params_)
    {
        firstOut = iter->value_;

        if (firstOut->kind_ != Param::ARG)
            return iter; // found first outcoming
        else
            ++numIn;
    }

    // not found
    return 0;
}

const Sig::Params::Node* Sig::findFirstOut() const
{
    size_t dummy;
    return findFirstOut(dummy);
}

Param* Sig::findParam(std::string* id)
{
    PARAMS_EACH(iter, params_)
    {
        if (*iter->value_->id_ == *id)
            return iter->value_;
    }

    // -> not found, so return 0
    return 0;
}

std::string Sig::toString() const
{
    std::ostringstream oss;
//     oss << '(';
    oss << "TODO";

//     // this is set to true in the loop below when the first outcoming param is found
//     bool noMoreInParms = false;
//
//     PARAMS_CONST_EACH(iter, params_)
//     {
//         Param* param = iter->value_;
//
//         if (param->kind_ != Param::ARG)
//         {
//             oss << ')'
//             oss << " -> ";
//         }
//     }
    return oss.str();
}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

Scope::Scope(Scope* parent)
    : parent_(parent)
{}

Scope::~Scope()
{
    // delete each child scope
    for (ScopeList::Node* iter = childScopes_.first(); iter != childScopes_.sentinel(); iter = iter->next())
        delete iter->value_;
}

/*
    further methods
*/

Local* Scope::lookupLocal(std::string* id)
{
    LocalMap::iterator iter = locals_.find(id);
    if ( iter != locals_.end() )
        return iter->second;
    else
    {
        // try to find in parent scope
        if (parent_)
            return parent_->lookupLocal(id);
        else
            return 0;
    }
}

} // namespace swift
