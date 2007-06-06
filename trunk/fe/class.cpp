#include "class.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/statement.h"
#include "fe/symtab.h"

#include "me/functab.h"
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
    symtab->enterClass(id_);

    // for each class member
    for (ClassMember* iter = classMember_; iter != 0; iter = iter->next_)
        iter->analyze();

    symtab->leaveClass();

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

void Scope::replaceRegNr(int oldNr, int newNr)
{
    RegNrMap::iterator iter = regNrs_.find(oldNr);
    if ( iter != regNrs_.end() )
    {
        Local* local = iter->second;
        regNrs_.erase(iter);    // remove from map
        local->regNr_ = newNr;  // update regNr
        regNrs_[newNr] = local; // put into the map again
    }
    else
    {
        // try to find in parent scope
        if (parent_)
            return parent_->replaceRegNr(oldNr, newNr);
        else
            swiftAssert(false, "no regNr found");
    }
}

//------------------------------------------------------------------------------

Method::~Method()
{
    delete returnType_;
    delete statements_;
    delete rootScope_;

    // delete each parameter
    for (size_t i = 0; i < params_.size(); ++i)
        delete params_[i];
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

    symtab->enterMethod(id_);
    functab->insertFunction( new std::string(*id_) );// TODO build a name consisting of module, class name, real name and arg-types

    // insert the first label since every function must start with one
    functab->appendInstr( new LabelInstr() );

    // analyze each parameter
    for (size_t i = 0; i < symtab->method_->params_.size(); ++i)
        symtab->method_->params_[i]->type_->validate();

    // analyze each statement
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        iter->analyze();

    symtab->leaveMethod();

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
