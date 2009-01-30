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

#ifndef SWIFT_PARSER_H
#define SWIFT_PARSER_H

extern "C" int swiftparse();

namespace swift {

// it is important to declare all union members of YYSTYPE here
struct Type;
struct BaseType;

struct Module;
struct Definition;

struct Class;
struct ClassMember;
struct Param;
struct MemberVar;
struct Method;

struct Statement;
struct ExprList;
struct Expr;

//------------------------------------------------------------------------------

extern bool parseerror;

std::string* operatorToString(int _operator);

} // namespace swift

// include auto generated parser header before tokens
#include "parser.tab.hpp"

#endif // SWIFT_PARSER_H
