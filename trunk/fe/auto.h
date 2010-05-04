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

#ifndef SWIFT_AUTO_H
#define SWIFT_AUTO_H

namespace swift {

// it is important to declare all union members of YYSTYPE here
class Class;
class ClassMember;
class Decl;
class Definition;
class Expr;
class ExprList;
class Lexer;
class MemberVar;
class MemberFunction;
class Module;
class Param;
class SimdPrefix;
class Statement;
class Tuple;
class Type;

//------------------------------------------------------------------------------

std::string* operatorToString(int _operator);

}

// include auto generated parser header before tokens
#include "parser.inner.h"

namespace swift {
typedef Parser::token Token;

extern int g_line;

FILE* lexer_init(const char* filename);

}

int swift_lex(swift::Parser::semantic_type* val, swift::Parser::location_type* loc);

#endif // SWIFT_AUTO_H
