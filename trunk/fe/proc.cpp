#include "proc.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/class.h"
#include "fe/method.h"
#include "fe/parser.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/var.h"

#include "me/functab.h"

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
    const Param* firstOut = findFirstOut(numIn);

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

const Param* Sig::findFirstOut(size_t& numIn) const
{
    numIn = 0;
    // shall hold the result
    Param* firstOut = 0;

    PARAMS_CONST_EACH(iter, params_)
    {
        firstOut = iter->value_;

        if (firstOut->kind_ != Param::ARG)
            break; // found first out
        else
            ++numIn;
    }

    return firstOut;
}

const Param* Sig::findFirstOut() const
{
    size_t dummy;
    return findFirstOut(dummy);
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

Proc::Proc(std::string* id, Method* method)
    : id_(id)
    , rootScope_( new Scope(0) )
//     , kind_(METHOD)
    , method_(method)
{}

Proc::~Proc()
{
    delete rootScope_;
}

/*
    getters and setters
*/

Method* Proc::getMethod()
{
    return method_;
}

/*
    further methods
*/

void Proc::appendParam(Param* param)
{
    sig_.params_.append(param);
}

Param* Proc::findParam(std::string* id)
{
    PARAMS_EACH(iter, sig_.params_)
    {
        if (*iter->value_->id_ == *id_)
            return iter->value_;
    }

    // -> not found, so return 0
    return 0;
}

bool Proc::analyze()
{
    bool result = true;

    /*
        it is assumed here that the symtab already points to the correct
        method and a function for the functab hast been created.
    */

    if (gencode)
    {
        /*
            build a function name for the functab consisting of the class name,
            the method name and a counted number to prevent name clashes
            due to overloading
        */
        std::ostringstream oss;

        oss << *symtab->class_->id_ << '#';

        if (getMethod()->methodQualifier_ == OPERATOR)
            oss << "operator";
        else
            oss << *id_;

        static int counter = 0;

        oss << '#' << counter;
        ++counter;

        functab->insertFunction( new std::string(oss.str()) );

        // insert the first label since every function must start with one
        functab->appendInstr( new LabelInstr() );
    }

    // analyze each statement
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    // insert the last label since every function must end with one
    if (gencode)
        functab->appendInstr( new LabelInstr() );

    symtab->leaveMethod();

    return result;
}

std::string Proc::toString() const
{
    return *id_ + sig_.toString();
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

Local* Scope::lookupLocal(int regNr)
{
    RegNrMap::iterator iter = regNrs_.find(regNr);
    if ( iter != regNrs_.end() )
        return iter->second;
    else
    {
        // try to find in parent scope
        if (parent_)
            return parent_->lookupLocal(regNr);
        else
            return 0;
    }
}
