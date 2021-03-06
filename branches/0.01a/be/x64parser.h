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

#ifndef BE_X64_PARSER_H
#define BE_X64_PARSER_H

#include <fstream>

namespace me {

struct StackLayout;

// it is important to declare all union members of YYSTYPE here
struct Const;
struct MemVar;
struct Op;
struct Reg;
struct Undef;

struct AssignInstr;
struct BranchInstr;
struct CallInstr;
struct GotoInstr;
struct LabelInstr;
struct Load;
struct LoadPtr;
struct Reload;
struct Spill;
struct Store;

} // namespace be

extern "C" int x64parse();
extern std::ofstream* x64_ofs; 
extern me::StackLayout* x64_stacklayout;

// include auto generated parser header before tokens
#include "x64parser.tab.hpp"

#endif // BE_X64_PARSER_H
