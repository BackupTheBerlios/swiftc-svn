#include "class.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/statement.h"
#include "fe/symtab.h"

#include "me/scopetab.h"
#include "me/ssa.h"


//------------------------------------------------------------------------------

Local::~Local()
{
    delete type_;
};

//------------------------------------------------------------------------------

Class::~Class()
{
    delete classMember_;
}

bool Class::analyze()
{
    symtab.enterClass(id_);
//     scopetab.registerStruct();

    // for each class member
    for (ClassMember* iter = classMember_; iter != 0; iter = iter->next_)
        iter->analyze();

    symtab.leaveClass();

    return true;
}

Parameter::~Parameter()
{
    delete type_;

    if (next_)
        delete next_;
}

std::string Parameter::toString() const
{
    std::ostringstream oss;

    switch (parameterQualifier_)
    {
        case IN:    oss << "in ";       break;
        case INOUT: oss << "inout ";    break;
        case OUT:   oss << "out ";      break;

        default:
            swiftAssert(false, "illegal case value");
            return "";
    }

    oss << type_->toString() << " ";
    oss << *id_;

    return oss.str();
}

//------------------------------------------------------------------------------

Method::~Method()
{
    delete returnType_;
    delete statements_;

    // delete each parameter
    for (size_t i = 0; i < params_.size(); ++i)
        delete params_[i];

    // delete each method
    for (LocalMap::iterator iter = locals_.begin(); iter != locals_.end(); ++iter)
        delete iter->second;
}

std::string Method::toString() const
{
    std::ostringstream oss;

    switch (methodQualifier_)
    {
        case READER:    oss << "reader ";   break;
        case WRITER:    oss << "writer ";   break;
        case ROUTINE:   oss << "routine ";  break;
        default:
            swiftAssert(false, "illegal case value");
            return "";
    }

    if (returnType_) {
        oss << returnType_->toString() << " ";
        oss << " ";
    }

    oss << *id_ << '(';
    for (size_t i = 0; i < params_.size(); ++i) {
        oss << params_[i]->toString();
        if (i + 1 < params_.size())
            oss << ", ";
    }

    oss << ')';

    return oss.str();
}

bool Method::analyze()
{
    bool result = true;

    symtab.enterMethod(id_);
    Scope* scope = scopetab.insertFunction(id_);
    scopetab.enter(scope);

    // analyze each parameter
    for (size_t i = 0; i < symtab.method_->params_.size(); ++i)
        symtab.method_->params_[i]->type_->validate();

    // analyze each statement
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        iter->analyze();

    scopetab.leave();
    symtab.leaveMethod();

    return result;
}

//------------------------------------------------------------------------------

MemberVar::~MemberVar()
{
    delete type_;
}

bool MemberVar::analyze()
{
    return true;
}

std::string MemberVar::toString() const
{
    std::ostringstream oss;

    oss << type_->toString() << " " << *id_;

    return oss.str();
}
