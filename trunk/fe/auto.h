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

//------------------------------------------------------------------------------

class Context;

// it is important to declare all union members of YYSTYPE here
class Class;
class Decl;
class Def;
class Expr;
class ExprList;
class InOut;
class Lexer;
class MemberFct;
class MemberVar;
class Module;
class Scope;
class Stmnt;
class Type;

//------------------------------------------------------------------------------

}

// include auto generated parser header before tokens
#include "parser.inner.h"

namespace swift {

typedef Parser::token Token;
typedef Parser::token_type TokenType;

extern int g_lexer_line;
extern std::string* g_lexer_filename;

FILE* lexer_init(const char* filename);

}

int swift_lex(swift::Parser::semantic_type* val, swift::Parser::location_type* loc);

#endif // SWIFT_AUTO_H
