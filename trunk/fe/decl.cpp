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

#include "fe/decl.h"

#include "fe/symtab.h"
#include "fe/type.h"

namespace swift {

/*
 * constructor and destructor
 */

Decl::Decl(Type* type, std::string* id, int line)
    : TypeNode(type, line)
    , id_(id)
    , local_(0) // This will be created in analyze
    , standAlone_(false)
{}

Decl::~Decl()
{
    if (local_)
        delete local_;
    else
        delete id_;
}

/*
 * further methods
 */

bool Decl::analyze()
{
    // check whether this type exists
    if ( !type_->validate() )
        return false;

    std::pair<Local*, bool> p = symtab->createNewLocal(type_, id_, line_);
    local_ = p.first;
    me::Var* meVar = local_->getMeVar();

    if (!p.second)
        return false;

    if (standAlone_)
    {
        me::AssignInstr* ai = new me::AssignInstr(
                '=', meVar, me::functab->newUndef(meVar->type_) );
        me::functab->appendInstr(ai);

        place_ = 0;

        return true;

    }

    if ( type_->isInternalAtomic() )
        place_ = meVar;
    else
    {
#ifdef SWIFT_DEBUG
        std::string tmpStr = std::string("p_") + meVar->id_;
        me::Reg* tmp = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
        me::Reg* tmp = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

        me::AssignInstr* ai = new me::AssignInstr(
                '=', meVar, me::functab->newUndef(meVar->type_) );
        me::functab->appendInstr(ai);
        me::functab->appendInstr( new me::LoadPtr(tmp, meVar, 0) );

        place_ = tmp;
    }

    return true;
}

/*
 * further methods
 */

void Decl::setAsStandAlone()
{
    standAlone_ = true;
}

std::string Decl::toString() const
{
    return local_->toString() + *id_;
}

} // namespace swift
