#include "method.h"

#include <sstream>

#include "fe/error.h"
#include "fe/symtab.h"

#include "me/functab.h"
#include "me/ssa.h"

//------------------------------------------------------------------------------

Parameter::~Parameter()
{
    delete type_;
}

bool Parameter::check(const Parameter* param1, const Parameter* param2)
{
    if (param1->kind_ != param2->kind_)
        return false;

    if ( Type::check(param1->type_, param2->type_) )
        return true;

    // else
    return false;
}

std::string Parameter::toString() const
{
    std::ostringstream oss;

    if (kind_ == RES_INOUT)
        oss << "inout ";

    oss << type_->toString() << " ";
//     oss << *id_;

    return oss.str();
}

//------------------------------------------------------------------------------

Scope::~Scope()
{
    // delete each child scope
    for (ScopeList::Node* iter = childScopes_.first(); iter != childScopes_.sentinel(); iter = iter->next())
        delete iter->value_;
}

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

//------------------------------------------------------------------------------

bool Method::Signature::check(const Method::Signature& sig1, const Method::Signature& sig2)
{
    // if the sizes do not match the signature is obviously different
    if ( sig1.params_.size() != sig2.params_.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    const Signature::Params::Node* param1 = sig1.params_.first();
    const Signature::Params::Node* param2 = sig2.params_.first();

    while (result && param1 != sig1.params_.sentinel())
    {
        result = Parameter::check(param1->value_, param2->value_);

        // traverse both nodes to the next node
        param1 = param1->next();
        param2 = param2->next();
    }

    return result;
}

bool Method::Signature::checkIngoing(const Method::Signature& insig1, const Method::Signature& insig2)
{
    // count the number of ingoing parameters of insig1
    size_t numIn1;
    // and find first outcoming parameter of this signatur
    const Parameter* firstOut1 = insig1.findFirstOut(numIn1);

    // count the number of ingoing parameters of insig2
    size_t numIn2;
    insig2.findFirstOut(numIn2);

    // if the sizes do not match the signature is obviously different
    if ( numIn1 != numIn2 )
        return false;

    // assume a true result in the beginning
    bool result = true;

    const Signature::Params::Node* param1 = insig1.params_.first();
    const Signature::Params::Node* param2 = insig1.params_.first();

    while (result && param1 != insig1.params_.sentinel() && param1->value_ != firstOut1)
    {
        result = Parameter::check(param1->value_, param2->value_);

        // traverse both nodes to the next node
        param1 = param1->next();
        param2 = param2->next();
    }

    return result;
}

bool Method::Signature::checkIngoing(const Method::Signature& insig) const
{
    // and count the number of ingoing paramters
    size_t numIn;
    // and find first outcoming parameter of this signatur
    const Parameter* firstOut = findFirstOut(numIn);

    // if the sizes do not match the signature is obviously different
    if ( numIn != insig.params_.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    const Signature::Params::Node* thisParam = params_.first();
    const Signature::Params::Node* inParam   = insig.params_.first();

    while (result && inParam != insig.params_.sentinel() && inParam->value_ != firstOut)
    {
        result = Parameter::check(thisParam->value_, inParam->value_);

        // traverse both nodes to the next node
        thisParam = thisParam->next();
        inParam   = inParam->next();
    }

    return result;
}

const Parameter* Method::Signature::findFirstOut() const
{
    size_t dummy;
    return findFirstOut(dummy);
}

const Parameter* Method::Signature::findFirstOut(size_t& numIn) const
{
    // find first outcoming parameter of this signatur
    Parameter* firstOut;
    numIn = 0; // and count the number of ingoing paramters

    for (const Params::Node* iter = params_.first(); iter != params_.sentinel(); iter = iter->next())
    {
        firstOut = iter->value_;

        if (firstOut->kind_ != Parameter::ARG)
            break;

        ++numIn;
    }

    return firstOut;
}

Method::~Method()
{
    delete statements_;
    delete rootScope_;

    // delete each parameter
    for (Params::iterator iter = params_.begin(); iter != params_.end(); ++iter)
        delete *iter;
}

void Method::appendParameter(Parameter* param)
{
    params_.insert(param);
    signature_.params_.append(param);
}

std::string Method::toString() const
{
    std::ostringstream oss;

    switch (methodQualifier_)
    {
        case CREATE:                        break;
        case READER:    oss << "reader ";   break;
        case WRITER:    oss << "writer ";   break;
        case ROUTINE:   oss << "routine ";  break;
        case OPERATOR:  oss << "operator "; break;
        default:
            swiftAssert(false, "illegal case value");
            return "";
    }

    oss << *id_ << '(';

    size_t i = 0;
    for (Params::iterator iter = params_.begin(); iter != params_.end(); ++iter, ++i)
    {
        oss << (*iter)->toString();
        if (i + 1 < params_.size())
            oss << ", ";
    }

    oss << ')';

    return oss.str();
}

bool Method::analyze()
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
