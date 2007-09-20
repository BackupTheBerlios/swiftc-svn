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

                if ( Method::Signature::check(methodIter->second->signature_, method->signature_) )
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

MemberVar::~MemberVar()
{
    delete type_;
}

bool MemberVar::analyze()
{
    // TODO

    return true;
}

std::string MemberVar::toString() const
{
    std::ostringstream oss;

    oss << type_->toString() << " " << *id_;

    return oss.str();
}
