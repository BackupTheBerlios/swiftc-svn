#include "fe/method.h"

#include <sstream>

#include "fe/error.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"
#include "me/ssa.h"

namespace swift {

/*
 * constructor and destructor
 */

Method::Method(int methodQualifier, std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : ClassMember(id, parent, line)
    , methodQualifier_(methodQualifier)
    , rootScope_( new Scope(0) )
{}

Method::~Method()
{
    delete rootScope_;
    delete statements_;
}

/*
 * further methods
 */

bool Method::analyze()
{
    symtab->enterMethod(this);

    /*
     * build a function name for the me::functab consisting of the class name,
     * the method name and a counted number to prevent name clashes
     * due to overloading
     */
    std::ostringstream oss;

    oss << *symtab->class_->id_ << '$';

    if (methodQualifier_ == OPERATOR)
        oss << "operator";
    else
        oss << *id_;

    static int counter = 0;

    oss << '$' << counter;
    ++counter;

    me::functab->insertFunction( new std::string(oss.str()) );

    bool result = true;
    result &= sig_.analyze(); // needs the valid function in the functab

    // insert the first label since every function must start with one
    me::functab->appendInstr( new me::LabelInstr() );

    // is it an operator?
    if (methodQualifier_ == OPERATOR)
    {
        /*
         * check signature
         */
        if (sig_.params_.size() >= 1)
        {
            // check whether the first type matches the type of the current class
            if ( *symtab->class_->id_ != *sig_.params_.first()->value_->type_->baseType_->id_ )
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
            Sig::Params::Node* param1 = sig_.params_.first();
            Sig::Params::Node* param2 = param1->next();
            Sig::Params::Node* param3 = param2->next();

            if (   sig_.params_.size() != 3
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
            Sig::Params::Node* param1 = sig_.params_.first();
            Sig::Params::Node* param2 = param1->next();

            if (   sig_.params_.size() != 2
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

    const Sig::Params::Node* firstOut = sig_.findFirstOut();

    // is there at least one argument?
    if ( !sig_.params_.empty() && sig_.params_.first() != firstOut)
    {
        // build function entry
        me::SetParams* setParams = new me::SetParams(0); // start with 0 args
        
        PARAMS_CONST_EACH(iter, sig_.params_)
        {
            const Param* param = iter->value_;

            me::Reg* reg = me::functab->lookupReg(param->varNr_);
            swiftAssert(reg, "must be found here");
            setParams->res_.push_back( me::Res(reg, param->varNr_, me::NO_CONSTRAINT) );
        }

        me::functab->appendInstr(setParams);
    }

    // analyze each statement
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    // insert the last label since every function must end with one
    me::functab->appendInstr( new me::LabelInstr() );

    // is there at least one result?
    if (firstOut)
    {
        // build function exit
        me::SetResults* setResults = new me::SetResults(0); // start with 0 args
        
        for (const Sig::Params::Node* iter = firstOut; iter != sig_.params_.sentinel(); iter = iter->next())
        {
            const Param* param = iter->value_;

            me::Reg* reg = me::functab->lookupReg(param->varNr_);
            swiftAssert(reg, "must be found here");
            setResults->arg_.push_back( me::Arg(reg, me::NO_CONSTRAINT) );
        }

        me::functab->appendInstr(setResults);
    }

    // insert the last label since every function must end with one
    me::functab->appendInstr( new me::LabelInstr() );

    symtab->leaveMethod();

    return result;
}

// std::string Method::toString() const
// {
//     std::ostringstream oss;
//
//     switch (methodQualifier_)
//     {
//         case CREATE:                        break;
//         case READER:    oss << "reader ";   break;
//         case WRITER:    oss << "writer ";   break;
//         case ROUTINE:   oss << "routine ";  break;
//         case OPERATOR:  oss << "operator "; break;
//         default:
//             swiftAssert(false, "illegal case value");
//             return "";
//     }
//
//     return oss.str() + sig_.toString();
// }

} // namespace swift
