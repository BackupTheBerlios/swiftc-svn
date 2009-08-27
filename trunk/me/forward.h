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

#ifndef ME_FORWARD_H
#define ME_FORWARD_H

#include <map>

#include "utils/list.h"
#include "utils/graph.h"
#include "utils/set.h"
#include "utils/map.h"

// include this file for useful forward declarations

namespace me {

struct BasicBlock;
typedef Graph<BasicBlock>::Node BBNode;
typedef List<BBNode*> BBList;
typedef Set<BBNode*> BBSet;

struct Function;

struct InstrBase;
typedef List<InstrBase*> InstrList;
typedef InstrList::Node InstrNode;

struct Op;
struct Const;
struct Var;
struct MemVar;
struct Reg;
typedef List<Var*> VarList;
typedef Map<int, Var*> VarMap;
typedef Set<Var*> VarSet;
typedef Set<Reg*> RegSet;
typedef std::vector<Var*> VarVec;
typedef Set<int> Colors;

} // namespace me

#endif // ME_FORWARD_H
