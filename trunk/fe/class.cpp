#include "fe/class.h"

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

Class::Class(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : Definition(id, parent, line)
    , hasCreate_(false)
{}

Class::~Class()
{
    delete classMember_;
}

/*
    further methods
*/

void Class::createDefaultConstructor()
{
    swiftAssert(hasCreate_ == false, "hasCreate_ must be true");

    Method* create = new Method(CREATE, new std::string("create"), this, -1);

    // link with this class
    if (classMember_ == 0)
        classMember_ = create;
    else
    {
        // prepend default constructor
        create->next_ = classMember_;
        classMember_ = create;
        symtab->insert(create);
    }
}

bool Class::analyze()
{
    const std::string& id = *id_;
    if (   id ==  "int" || id ==  "int8" || id ==  "int16" || id ==  "int32" || id ==  "int64"
        || id == "uint" || id == "uint8" || id == "uint16" || id == "uint32" || id == "uint64"
        || id == "sat8" || id == "sat16" || id ==  "usat8" || id == "usat16"
        || id == "real" || id == "real32"|| id ==  "real64"
        || id == "bool")
    {
        // skip builtin types.
        return true;
    }


    // assume true as initial state
    bool result = true;

    symtab->enterClass(this);

    // for each class member
    for (ClassMember* iter = classMember_; iter != 0; iter = iter->next_)
    {
        /*
            TODO since this is an O(n^2) algorithm it should be checked
            whether in real-world-programms an O(n log n) algorithm with sorting
            is faster
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

                if ( Sig::check(methodIter->second->sig_, method->sig_) )
                {
                    errorf(methodIter->second->line_, "there is already a method '%s' defined in '%s' line %i",
                        methodIter->second->toString().c_str(),
                        method->getFullName().c_str(), method->line_);

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

/*
    constructor and destructor
*/

ClassMember::ClassMember(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : Symbol(id, parent, line)
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

MemberVar::MemberVar(Type* type, std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : ClassMember(id, parent, line)
    , type_(type)
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
