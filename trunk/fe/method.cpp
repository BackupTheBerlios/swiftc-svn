#include "method.h"

#include <sstream>

#include "fe/error.h"
#include "fe/symtab.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"
#include "me/ssa.h"

/*
    constructor
*/

Method::Method(int methodQualifier, std::string* id, int line /*= NO_LINE*/, Node* parent /*= 0*/)
    : ClassMember(line, parent)
    , proc_(id, this)
    , methodQualifier_(methodQualifier)
{}

/*
    further methods
*/

void Method::appendParam(Param* param)
{
    proc_.appendParam(param);
}

bool Method::analyze()
{
    bool result = true;

    symtab->enterMethod(this);

    // is it an operator?
    if (methodQualifier_ == OPERATOR)
    {
        /*
            check signature
        */
        if (proc_.sig_.params_.size() >= 1)
        {
            // check whether the first type matches the type of the current class
            if ( *symtab->class_->id_ != *proc_.sig_.params_.first()->value_->type_->baseType_->id_ )
            {
                errorf( line_, "The the first parameter of this operator must be of type %s",
                    symtab->class_->id_->c_str() );
                result = false;
            }
        }

        // minus needs special handling -> can be unary or binary
        bool unaryMinus = false;

        if (   *proc_.id_ == "+"
            || *proc_.id_ == "-"
            || *proc_.id_ == "*"
            || *proc_.id_ == "/"
            || *proc_.id_ == "mod"
            || *proc_.id_ == "div"
            || *proc_.id_ == "=="
            || *proc_.id_ == "<>"
            || *proc_.id_ == "<"
            || *proc_.id_ == ">"
            || *proc_.id_ == "<="
            || *proc_.id_ == ">="
            || *proc_.id_ == "and"
            || *proc_.id_ == "or"
            || *proc_.id_ == "xor")
        {
            Sig::Params::Node* param1 = proc_.sig_.params_.first();
            Sig::Params::Node* param2 = param1->next();
            Sig::Params::Node* param3 = param2->next();

            if (   proc_.sig_.params_.size() != 3
                || param1->value_->kind_ != Param::ARG
                || param2->value_->kind_ != Param::ARG
                || param3->value_->kind_ != Param::RES)
            {
                if (*proc_.id_ == "-")
                    unaryMinus = true;
                else
                {
                    errorf( line_, "The '%s'-operator must exactly have two incoming and one outgoing parameter",
                        proc_.id_->c_str() );
                    result = false;
                }
            }

        }

        if (*proc_.id_ == "not" || unaryMinus)
        {
            Sig::Params::Node* param1 = proc_.sig_.params_.first();
            Sig::Params::Node* param2 = param1->next();

            if (   proc_.sig_.params_.size() != 2
                || param1->value_->kind_ != Param::ARG
                || param2->value_->kind_ != Param::RES)
            {
                if (*proc_.id_ == "-")
                {
                    errorf(line_,
                        "The '-'-operator must either have exactly two incoming and one outgoing or one incoming and one outgoing parameter");
                }
                else
                {
                    errorf( line_, "The '%s'-operator must exactly have one incoming and one outgoing parameter",
                        proc_.id_->c_str() );
                }
                result = false;
            }
        }
    }

    result &= proc_.analyze();

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

    return oss.str() + proc_.toString();
}
