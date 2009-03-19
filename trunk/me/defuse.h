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

#ifndef ME_DEFUSE_H
#define ME_DEFUSE_H

#include "utils/list.h"
#include "utils/graph.h"
#include "utils/map.h"

#include "me/basicblock.h"
#include "me/forward.h"

namespace me {

/// This class can be used to reference def and use information
struct DefUse
{
    DefUse() {}
    DefUse(Var* var, InstrNode* instrNode, BBNode* bbNode)
        : var_(var)
        , instrNode_(instrNode)
        , bbNode_(bbNode)
    {}

    void set(Var* var, InstrNode* instrNode, BBNode* bbNode)
    {
        var_        = var;
        instrNode_  = instrNode;
        bbNode_     = bbNode;
    }

    Var* var_;              /// < The var which is referenced here.
    InstrNode* instrNode_;  ///< The instruction which is referenced here.
    BBNode* bbNode_;        ///< The basic block where \a instr_ can be found.
};

//-------------------------------------------------------------------------------

typedef List<DefUse> DefUseList;

#define DEFUSELIST_EACH(iter, defUseList) \
    for (me::DefUseList::Node* (iter) = (defUseList).first(); (iter) != (defUseList).sentinel(); (iter) = (iter)->next())

//-------------------------------------------------------------------------------

struct VarDefUse
{
    DefUseList defs_;
    DefUseList uses_;
};

//------------------------------------------------------------------------------

/// Keeps vars with their definitions und uses.
typedef Map<Var*, VarDefUse*> VDUMap;

#define VDUMAP_EACH(iter, vdus) \
    for (VDUMap::iterator (iter) = (vdus).begin(); (iter) != (vdus).end(); ++(iter))

//------------------------------------------------------------------------------

} // namespace me

#endif // ME_DEFUSE_H
