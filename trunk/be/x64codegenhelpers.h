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

#ifndef BE_X64_CODE_GEN_HELPERS_H
#define BE_X64_CODE_GEN_HELPERS_H

#include <string>

#include "be/x64regalloc.h"

// forward declarations
namespace me {
    struct Op;
    struct Undef;
    struct Const;
    struct Reg;

    struct AssignInstr;
}

namespace be {

std::string suffix(int);

std::string reg2str(me::Reg* reg);
std::string reg2str(int color, me::Op::Type type);
std::string memvar2str(me::MemVar* memVar, size_t Offset);
std::string ptr2str(me::Reg* reg, size_t offset);

std::string ccsuffix(me::AssignInstr* ai, int type, bool neg = false);
std::string jcc(me::BranchInstr* bi, bool neg = false);

std::string cst2str(me::Const* cst);
std::string mcst2str(me::Const* cst);

std::string sar_cst2str(int type);
std::string sgn_cst2str(me::Const* cst);
std::string rdx2str(int type);
std::string div2str(int type);

std::string instr2str(me::AssignInstr* ai);
std::string mul2str(int type);
std::string div2str(int type);

std::string cst_op_cst(me::AssignInstr* ai, me::Const* cst1, me::Const* cst2, bool mem = false);
std::string un_minus_cst(me::AssignInstr* ai, me::Const* cst, bool mem = false);

std::string neg_mask(int type, bool mem = false);

}

#endif // BE_X64_CODE_GEN_HELPERS_H
