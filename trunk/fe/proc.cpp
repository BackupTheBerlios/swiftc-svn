#inlude "proc.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/var.h"

//------------------------------------------------------------------------------

bool Sig::check(const Method::Sig& sig1, const Method::Sig& sig2)
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
        result = Parameter::check(param1->value_, param2->value_);

        // traverse both nodes to the next node
        param1 = param1->next();
        param2 = param2->next();
    }

    return result;
}

const Param* Sig::findFirstOut() const
{
    // shall hold the result
    Parameter* firstOut = 0;

    PARAMS_CONST_EACH(iter, params_)
    {
        firstOut = iter->value_;

        if (firstOut->kind_ != Parameter::ARG)
            break; // found first out
    }

    return firstOut;
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
    , kind_(METHOD)
    , method_(method)
{}

Proc::~Proc()
{
    delete rootScope_;

    // delete each parameter
    for (Params::iterator iter = params_.begin(); iter != params_.end(); ++iter)
        delete *iter;
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

Param* Param::findParem(std::string* id)
{
    PARAM_EACH(iter, sig_.param_)
    {
        if (*iter->value_->id_ == *id_)
            return iter->value_;
    }

    // -> not found, so return 0
    return 0;
}

std::string Proc::toString() const
{
    return *id + sig_.toString();
}

bool Proc::analyze()
{
    static int counter = 0;

    bool result = true;

    symtab->enterMethod(this);

    if (gencode)
    {
        /*
            build a function name for the functab consisting of the class name,
            the method name and a counted number to prevent name clashes
            due to overloading
        */
        std::ostringstream oss;

        oss << *symtab->class_->id_ << '#';

        if (methodQualifier_ == OPERATOR)
            oss << "operator";
        else
            oss << *id_;

        oss << '#' << counter;
        ++counter;

        functab->insertFunction( new std::string(oss.str()) );

        // insert the first label since every function must start with one
        functab->appendInstr( new LabelInstr() );
    }

    // validate each parameter
    for (Params::iterator iter = params_.begin(); iter != params_.end(); ++iter)
        result &= (*iter)->type_->validate();

    // is it an operator?
    if (methodQualifier_ == OPERATOR)
    {
        /*
            check signature
        */
        if (signature_.params_.size() >= 1)
        {
            // check whether the first type matches the type of the current class
            if ( *symtab->class_->id_ != *signature_.params_.first()->value_->type_->baseType_->id_ )
            {
                errorf( line_, "The the first parameter of this operator must be of type %s",
                    symtab->class_->id_->c_str() );
                result = false;
            }
        }

        // minus needs special handling -> can be unary or binary
        bool unaryMinus = false;

        if (   *id_ == "+"
            || *id_ == "-"
            || *id_ == "*"
            || *id_ == "/"
            || *id_ == "mod"
            || *id_ == "div"
            || *id_ == "=="
            || *id_ == "<>"
            || *id_ == "<"
            || *id_ == ">"
            || *id_ == "<="
            || *id_ == ">="
            || *id_ == "and"
            || *id_ == "or"
            || *id_ == "xor")
        {
            Signature::Params::Node* param1 = signature_.params_.first();
            Signature::Params::Node* param2 = param1->next();
            Signature::Params::Node* param3 = param2->next();

            if (   signature_.params_.size() != 3
                || param1->value_->kind_ != Parameter::ARG
                || param2->value_->kind_ != Parameter::ARG
                || param3->value_->kind_ != Parameter::RES)
            {
                if (*id_ == "-")
                    unaryMinus = true;
                else
                {
                    errorf( line_, "The '%s'-operator must exactly have two incoming and one outgoing parameter",
                        id_->c_str() );
                    result = false;
                }
            }

        }
        if (*id_ == "not" || unaryMinus)
        {
            Signature::Params::Node* param1 = signature_.params_.first();
            Signature::Params::Node* param2 = param1->next();

            if (   signature_.params_.size() != 2
                || param1->value_->kind_ != Parameter::ARG
                || param2->value_->kind_ != Parameter::RES)
            {
                if (*id_ == "-")
                {
                    errorf(line_,
                        "The '-'-operator must either have exactly two incoming and one outgoing or one incoming and one outgoing parameter");
                }
                else
                {
                    errorf( line_, "The '%s'-operator must exactly have one incoming and one outgoing parameter",
                        id_->c_str() );
                }
                result = false;
            }
        }
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
