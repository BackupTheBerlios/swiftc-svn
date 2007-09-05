#include "class.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/error.h"
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
    symtab->enterClass(this);

    // assume true as initial state
    bool result = true;

    // for each class member
    for (ClassMember* iter = classMember_; iter != 0; iter = iter->next_)
    {
        /*
            TODO since this is an O(n^2) algorithm it should be checked
            whether in real-world-programms an O(n log n) algorithm with sorting
            ist faster
        */
        if ( typeid(*iter) == typeid(Method) )
        {
            Method* method = (Method*) iter;

            /*
                check whether there is method with the same name and the same signature
            */
            typedef Class::MethodMap::iterator Iter;
            Iter methodIter = methods_.find(method->id_);

            // move iter to point to method
            while (methodIter->second != method)
                ++methodIter;

            // and one further to the first item which is not itself
            ++methodIter;

            // find element behind the last one
            Iter last = methods_.upper_bound(method->id_);

            for (; methodIter != last; ++methodIter)
            {
                // check methodQualifier_
                if (methodIter->second->methodQualifier_ != method->methodQualifier_)
                    continue;

                if ( methodIter->second->signature_ == method->signature_ )
                {
                    std::stack<std::string> idStack;

                    for (Node* nodeIter = methodIter->second->parent_; nodeIter != 0; nodeIter = nodeIter->parent_)
                        idStack.push( nodeIter->toString() );

                    std::ostringstream oss;

                    while ( !idStack.empty() )
                    {
                        oss << idStack.top();
                        idStack.pop();

                        if ( !idStack.empty() )
                            oss << '.';
                    }

                    errorf(methodIter->second->line_, "there is already a method '%s' defined in '%s' line %i",
                        method->toString().c_str(),
                        oss.str().c_str(), method->line_);

                    result = false;

                    break;
                }
            }
        }

        result &= iter->analyze();
    }

    symtab->leaveClass();

    return result;
}

//------------------------------------------------------------------------------

Parameter::~Parameter()
{
    delete type_;
}

bool Parameter::operator == (const Parameter& parameter) const
{
    if (kind_ != parameter.kind_)
        return false;

    if ( Type::check(type_, parameter.type_) )
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

bool Method::Signature::operator == (const Method::Signature& signature) const
{
    // if the sizes do not match the signature is obviously different
    if ( params_.size() != signature.params_.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    const Signature::Params::Node* param1 = params_.first();
    const Signature::Params::Node* param2 = signature.params_.first();

    while (result && param1 != params_.sentinel())
    {
        result = *param1->value_ == *param2->value_;

        // traverse both nodes to the next node
        param1 = param1->next();
        param2 = param2->next();
    }

    return result;
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

void Method::insertReturnTypesInSymtab()
{
    swiftAssert(returnTypeList_, "this may not be NULL");

    for (Parameter* iter = returnTypeList_; iter != 0; iter = iter->next_)
        symtab->insert(iter);
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
        default:
            swiftAssert(false, "illegal case value");
            return "";
    }

    if (returnTypeList_) {
        oss << returnTypeList_->toString() << " ";
        oss << " ";
    }

    oss << *id_ << '(';

    size_t i = 0;
    for (Params::iterator iter = params_.begin(); iter != params_.end(); ++iter, ++i) {
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

    /*
        build a function name for the functab consisting of the class name,
        the method name and a counted number to prevent name clashes
        due to overloading
    */
    std::ostringstream oss;
    oss << *symtab->class_->id_ << '#' << *id_ << '#' << counter;
    ++counter;

    functab->insertFunction( new std::string(oss.str()) );

    // insert the first label since every function must start with one
    functab->appendInstr( new LabelInstr() );

    // analyze each parameter
    for (Params::iterator iter = params_.begin(); iter != params_.end(); ++iter)
        result &= (*iter)->type_->validate();

    // analyze each statement
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    // insert the last label since every function must end with one
    functab->appendInstr( new LabelInstr() );

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
