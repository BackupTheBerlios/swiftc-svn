/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "fe/class.h"

#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/method.h"
#include "fe/signature.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/type.h"

#include "me/functab.h"
#include "me/offset.h"
#include "me/struct.h"
#include "me/ssa.h"

namespace swift {

/*
 * constructor and destructor
 */

Class::Class(std::string* id, Symbol* parent, int line)
    : Definition(id, parent, line)
    , defaultCreate_(DEFAULT_NONE)
    , copyCreate_(COPY_USER)
{}

Class::~Class()
{
    delete classMember_;
}

/*
 * further methods
 */

void Class::addConstructors()
{
    /*
     * handling of the default constructor
     */
    {
        // check whether there is already a default constructor
        TypeList in;
        Method* create = symtab->lookupCreate(this, in, 0);

        if (create)
            defaultCreate_ = DEFAULT_USER;
        else if (!hasCreate_)
        {
            /*
            * -> construct a trivial default constructor 
            * if there are no constructors defined at all
            */
            defaultCreate_ = DEFAULT_TRIVIAL;

            create = new Method(CREATE, new std::string("create"), this, NO_LINE);
            symtab->insert(create);
            create->statements_ = 0;

            // link with this class
            if (classMember_ == 0)
                classMember_ = create;
            else
            {
                // prepend default constructor
                create->next_ = classMember_;
                classMember_ = create;
            }
        }
    }

    /*
     * handling of the copy constructor
     */
    {
        // check whether there is already a copy constructor
        BaseType* newType = new BaseType(CONST_PARAM, this);
        TypeList in;
        in.push_back(newType);
        Method* create = symtab->lookupCreate(this, in, 0);

        if (!create)
        {
            copyCreate_ = COPY_AUTO;

            create = new Method(CREATE, new std::string("create"), this, NO_LINE);
            create->statements_ = 0;
            symtab->insert(create);
            symtab->insertParam( new Param(newType, new std::string("arg")) );

            // link with this class
            if (classMember_ == 0)
                classMember_ = create;
            else
            {
                // prepend copy constructor
                create->next_ = classMember_;
                classMember_ = create;
            }
        }
        else
            delete newType;
    }
}

bool Class::analyze()
{
    if ( BaseType::isBuiltin(id_) )
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
         * TODO since this is an O(n^2) algorithm it should be checked
         * whether in real-world-programms an O(n log n) algorithm with sorting
         * is faster
         */
        if ( typeid(*iter) == typeid(Method) )
        {
            Method* method = (Method*) iter;

            /*
             * check whether there is method with the same name and the same signature
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

                if ( methodIter->second->sig_->check(method->sig_) )
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
 * constructor and destructor
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
 * constructor and destructor
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
 * further methods
 */

bool MemberVar::analyze()
{
    // registerMeMember is actually the analyze which has been invoked prior
    return true;
}

bool MemberVar::registerMeMember()
{
    if (!type_->validate())
        return false;

    me::Op::Type meType = type_->toMeType();

    if (meType == me::Op::R_STACK)
    {
        swiftAssert( typeid(*type_) == typeid(BaseType),
                "must be a BaseType here" );
        BaseType* bt = (BaseType*) type_;

        Class* _class = bt->lookupClass();
        swiftAssert(_class, "must be found here");
        meMember_ = _class->meStruct_;
    }
    else
    {
        // -> it is a builtin type or a pointer
#ifdef SWIFT_DEBUG
        meMember_ = new me::AtomicMember(meType, *id_);
#else // SWIFT_DEBUG
        meMember_ = new me::AtomicMember(meType);
#endif // SWIFT_DEBUG
    }

    // append member to current me::Struct in all cases
    me::functab->appendMember(meMember_);

    return true;
}

} // namespace swift
