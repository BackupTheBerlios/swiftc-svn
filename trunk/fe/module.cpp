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

#include "fe/module.h"

#include <iostream>
#include <typeinfo>

#include "fe/class.h"
#include "fe/type.h"

#include "me/functab.h"
#include "me/struct.h"

namespace swift {

/*
 * constructor and destructor
 */

Module::Module(std::string* id, int line /*= NO_LINE*/)
    : Symbol(id, 0, line) // modules don't have parents
{}

Module::~Module()
{
    // for each definition
    for (DefinitionList::Node* iter = definitions_.first(); iter != definitions_.sentinel(); iter = iter->next())
        delete iter->value_;
}

/*
 * further methods
 */

bool Module::analyze()
{
    bool result = true;

    // for each class
    for (DefinitionList::Node* iter = definitions_.first(); iter != definitions_.sentinel(); iter = iter->next())
    {
        if ( typeid(*iter->value_) == typeid(Class) )
        {
            Class* _class = (Class*) iter->value_;
            me::functab->enterStruct(_class->meStruct_);
            
            // for each MemberVar
            for (ClassMember* iter = _class->classMember_; iter != 0; iter = iter->next_)
            {
                if ( typeid(*iter) != typeid(MemberVar) )
                    continue;

                MemberVar* mv = (MemberVar*) iter;

#ifdef SWIFT_DEBUG
                mv->meMember_ = new me::AtomicMember(
                        (*BaseType::typeMap_)[*mv->type_->baseType_->id_], *mv->id_ );
                me::functab->appendMember(mv->meMember_);
#else // SWIFT_DEBUG
                mv->meMember_ = new me::AtomicMember(
                        (*BaseType::typeMap_)[*mv->type_->baseType_->id_] );
                me::functab->appendMember(mv->meMember_);
#endif // SWIFT_DEBUG

            } // for each MemberVar

            me::functab->leaveStruct();
        } // if type == Class
    } // for each class

    // for each definition
    for (DefinitionList::Node* iter = definitions_.first(); iter != definitions_.sentinel(); iter = iter->next())
        result &= iter->value_->analyze();

    return result;
}

std::string Module::toString() const
{
    return *id_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Definition::Definition(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : Symbol(id, parent, line)
{}

} // namespace swift
