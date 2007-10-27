#include "method.h"

#include <sstream>

#include "fe/error.h"
#include "fe/symtab.h"

#include "me/functab.h"
#include "me/ssa.h"

/*
    constructor or destructor
*/

Method::Method(int methodQualifier, std::string* id, int line /*= NO_LINE*/, Node* parent /*= 0*/)
    : ClassMember(line, parent)
    , Proc(id, this)
    , methodQualifier_(methodQualifier)
{}

Method::~Method()
{
    delete statements_;
    delete rootScope_;

    // delete each parameter
    for (Params::iterator iter = params_.begin(); iter != params_.end(); ++iter)
        delete *iter;
}

/*
    further methods
*/

void Method::appendParam(Param* param)
{
    proc_.appendParam(param);
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
                || param1->value_->kind_ != Param::ARG
                || param2->value_->kind_ != Param::ARG
                || param3->value_->kind_ != Param::RES)
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
                || param1->value_->kind_ != Param::ARG
                || param2->value_->kind_ != Param::RES)
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
