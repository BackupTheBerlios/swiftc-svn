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

#include "me/defuse.h"

#include <sstream>

#include "me/op.h"

namespace me {

//-------------------------------------------------------------------------------

/*
 * constructor
 */

DefUse::DefUse(Var* var, InstrNode* instrNode, BBNode* bbNode)
    : var_(var)
    , instrNode_(instrNode)
    , bbNode_(bbNode)
{}

/*
 * further methods
 */

void DefUse::set(Var* var, InstrNode* instrNode, BBNode* bbNode)
{
    var_        = var;
    instrNode_  = instrNode;
    bbNode_     = bbNode;
}

std::string DefUse::toString() const
{
    return "\t" + var_->toString() + "\t" + bbNode_->value_->name() +  "\t" + instrNode_->value_->toString();
}

//-------------------------------------------------------------------------------

std::string VarDefUse::toString() const
{
    std::ostringstream oss;
    oss << "defs: " << std::endl;

    DEFUSELIST_CONST_EACH(iter, defs_)
        oss << iter->value_.toString() << std::endl;

    oss << "uses: " << std::endl;

    DEFUSELIST_CONST_EACH(iter, uses_)
        oss << iter->value_.toString() << std::endl;

    oss << std::endl;

    return oss.str();
}

} // namespace me
