#include "class.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/method.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/type.h"

#include "me/functab.h"
#include "me/ssa.h"

/*
    constructor and destructor
*/

Class::Class(std::string* id, int line /*= NO_LINE*/, Node* parent /*= 0*/)
    : Definition(line, parent)
    , id_(id)
{}

Class::~Class()
{
    delete classMember_;
}

/*
    further methods
*/

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
            Iter methodIter = methods_.find(method->proc_.id_);

            // move iter to point to method
            while (methodIter->second != method)
                ++methodIter;

            // and one further to the first item which is not itself
            ++methodIter;

            // find element behind the last one
            Iter last = methods_.upper_bound(method->proc_.id_);

            for (; methodIter != last; ++methodIter)
            {
                // check methodQualifier_
                if (methodIter->second->methodQualifier_ != method->methodQualifier_)
                    continue;

                if ( Sig::check(methodIter->second->proc_.sig_, method->proc_.sig_) )
                {
                    std::stack<std::string> idStack;

// TODO
//                     for (Node* nodeIter = methodIter->second->parent_; nodeIter != 0; nodeIter = nodeIter->parent_)
//                         idStack.push( nodeIter->toString() );
//
//                     std::ostringstream oss;
//
//                     while ( !idStack.empty() )
//                     {
//                         oss << idStack.top();
//                         idStack.pop();
//
//                         if ( !idStack.empty() )
//                             oss << '.';
//                     }

                    errorf(methodIter->second->line_, "there is already a method '%s' defined in '%s' line %i",
                        method->toString().c_str(),
                        methodIter->second->toString().c_str(), method->line_); // TODO

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

std::string Class::toString() const
{
    return *id_;
}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

ClassMember::ClassMember(int line, Node* parent /*= 0*/)
    : Node(line, parent)
    , next_(0)
{}

ClassMember::~ClassMember()
{
    delete next_;
}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

MemberVar::MemberVar(Type* type, std::string* id, int line /*= NO_LINE*/, Node* parent /*= 0*/)
    : ClassMember(line, parent)
    , type_(type)
    , id_(id)
{}

MemberVar::~MemberVar()
{
    delete type_;
}

/*
    further methods
*/

bool MemberVar::analyze()
{
    // TODO

    return true;
}
