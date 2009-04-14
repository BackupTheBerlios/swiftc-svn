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
#include "fe/memberfunction.h"
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

void Class::autoGenMethods()
{
    addDefaultCreate();
    addCopyCreate();
    addAssignOperators();
}

void Class::addDefaultCreate()
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

        create = new Create(this);
        symtab->insert(create);
        create->statements_ = 0;

        prependMember(create);
    }
}

void Class::addCopyCreate()
{
    // check whether there is already a copy constructor
    BaseType* newType = new BaseType(CONST_PARAM, this);
    TypeList in;
    in.push_back(newType);
    Method* create = symtab->lookupCreate(this, in, 0);

    if (!create)
    {
        copyCreate_ = COPY_AUTO;

        create = new Create(this);
        create->statements_ = 0;
        symtab->insert(create);
        symtab->insertParam( new Param(newType, new std::string("arg")) );

        prependMember(create);
    }
    else
        delete newType;
}

void Class::addAssignOperators()
{
    // lookup first create method
    std::string createStr = "create";
    Class::MemberFunctionMap::const_iterator iter = memberFunctions_.find(&createStr);

    // get iterator to the first method, which has not "create" as identifier
    Class::MemberFunctionMap::const_iterator last = memberFunctions_.upper_bound(&createStr);

    for (MemberFunction* create = 0; iter != last; ++iter)
    {
        create = iter->second;
        TypeList in = create->sig_->getIn();

        // is this the default constructor?
        if ( in.empty() )
            continue;

        // check wether there is already an assign operator defined with this in-types
        Assign* assign = symtab->lookupAssign(this, create->sig_->getIn(), 0);

        if (!assign)
        {
            assign = new Assign(this);
            assign->statements_ = 0;
            symtab->insert(assign);

            for (size_t i = 0; i < in.size(); ++i)
                symtab->insertParam( new Param(in[i]->clone(), new std::string("arg")) );

            prependMember(assign);
        }
    }
}

void Class::prependMember(ClassMember* newMember)
{
    // link with this class
    if (classMember_ == 0)
        classMember_ = newMember;
    else
    {
        // prepend newMember
        newMember->next_ = classMember_;
        classMember_ = newMember;
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
        MemberFunction* memberFunction = dynamic_cast<MemberFunction*>(iter);
        if (memberFunction)
        {
            /*
             * check whether there is method with the same name and the same signature
             */
            typedef Class::MemberFunctionMap::iterator Iter;
            Iter methodIter = memberFunctions_.find(memberFunction->id_);

            // move iter to point to method
            while (methodIter->second != memberFunction)
                ++methodIter;

            // and one further to the first item which is not itself
            ++methodIter;

            // find element behind the last one
            Iter last = memberFunctions_.upper_bound(memberFunction->id_);

            for (; methodIter != last; ++methodIter)
            {
                // check methodQualifier_
                if ( typeid(*methodIter->second) != typeid(*memberFunction) )
                    continue;

                if ( methodIter->second->sig_->check(memberFunction->sig_) )
                {
                    // TODO better error message
                    errorf( methodIter->second->line_, 
                            "there is already a member function '%s' defined in '%s' line %i",
                            methodIter->second->toString().c_str(),
                            memberFunction->getFullName().c_str(), memberFunction->line_ );

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
