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

%option noyywrap
%option yylineno

%{

#include <string>
#include <iostream>
#include <set>

#include <cstdio>

#include "utils/assert.h"

#include "fe/token.h"
#include "fe/typenode.h"

namespace swift {
    int g_lexer_comment_level;
    std::string* g_lexer_filename = 0;
}

using namespace swift;

// define lex interface
#define YY_DECL swift::Token swift_lex()
#define yyterminate() return swift::Token(0, 0, Token::END_OF_FILE)

// macro magic for convenience 
#define RETURN_TOKEN(type) return Token(g_lexer_filename, swift_lineno, (type))
#define RETURN_LITERAL(type, lit) return Token(g_lexer_filename, swift_lineno, (type))
#define RETURN_ID(type) return Token(g_lexer_filename, swift_lineno, (type), new std::string(swift_text))
#define GET_BASE (swift_leng > 2 && tolower(swift_text[1]) == 'x') ? 16 : 10

%}

ID      [_a-zA-Z][_a-zA-Z0-9]*

D       [0-9]
H       [0-9a-fA-F]
E       [Ee][+-]?{D}+

FLOAT   {D}+{E}|{D}*\.{D}+({E})?|{D}+\.{D}*({E})?
INTEGER {D}+|0[xX]{H}+

%x COMMENT
%x NESTED_COMMENT

%%

\/\*          { BEGIN COMMENT; }
<COMMENT>\*\/ { BEGIN INITIAL; }
<COMMENT>\n   { /*skip*/  }
<COMMENT>.    { /*skip*/  }

\/\+                 { BEGIN NESTED_COMMENT; ++g_lexer_comment_level; }
<NESTED_COMMENT>\/\+ { ++g_lexer_comment_level;  }
<NESTED_COMMENT>\+\/ { --g_lexer_comment_level; if (g_lexer_comment_level == 0) BEGIN INITIAL; }
<NESTED_COMMENT>\n   { /*skip*/  }
<NESTED_COMMENT>.    { /*skip*/ }

[@()\[\]{},.:] { // single character tokens
    RETURN_TOKEN(swift_text[0]);
}

->   { RETURN_TOKEN(Token::ARROW); }

[~!]                     { RETURN_ID(Token::NOT); }
`{ID}                    { RETURN_ID(Token::BACKTICK_ID); }
[*/%&]|\<\<|>>           { RETURN_ID(Token::MUL); }
[+\-]                    { RETURN_ID(Token::ADD_SUB); }
[|\^]                    { RETURN_ID(Token::OR_XOR);  }
@{ID}                    { RETURN_ID(Token::AT_ID); }
\<=|>=|\==|!=|\<|>       { RETURN_ID(Token::REL); }
=|\<\<=|>>=|[+\-*/%|^&]= { RETURN_ID(Token::ASGN); }

::  { RETURN_TOKEN(Token::DOUBLE_COLON); }

c_call  { RETURN_TOKEN(Token:: C_CALL); }
vc_call { RETURN_TOKEN(Token::VC_CALL); }

if      { RETURN_TOKEN(Token::IF); }
else    { RETURN_TOKEN(Token::ELSE); }
elif    { RETURN_TOKEN(Token::ELIF); }
for     { RETURN_TOKEN(Token::FOR); }
while   { RETURN_TOKEN(Token::WHILE); }
repeat  { RETURN_TOKEN(Token::REPEAT); }
until   { RETURN_TOKEN(Token::UNTIL); }

return   { RETURN_TOKEN(Token::RETURN); }
break    { RETURN_TOKEN(Token::BREAK); }
continue { RETURN_TOKEN(Token::CONTINUE); }

scope    { RETURN_TOKEN(Token::SCOPE); }
class    { RETURN_TOKEN(Token::CLASS); }
end      { RETURN_TOKEN(Token::END); }
def      { RETURN_TOKEN(Token::DEF); }
static   { RETURN_TOKEN(Token::STATIC); }

var       { RETURN_TOKEN(Token::VAR); }
const     { RETURN_TOKEN(Token::CONST); }
ptr       { RETURN_TOKEN(Token::PTR); }
array     { RETURN_TOKEN(Token::ARRAY); }
simd      { RETURN_TOKEN(Token::SIMD); }

broadcast  { RETURN_TOKEN(Token::BROADCAST); }
simd_range { RETURN_TOKEN(Token::SIMD_RANGE); }

nil   { RETURN_TOKEN(Token::NIL); }
this  { RETURN_TOKEN(Token::THIS); }

{INTEGER}x      { Box b; b.size_   = strtol  (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_INDEX,  new Literal(*loc, b, Token::L_INDEX )); }

{INTEGER}       { Box b; b.int_    = strtol  (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_INT,    new Literal(*loc, b, Token::L_INT   )); }
{INTEGER}b      { Box b; b.int8_   = strtol  (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_INT8,   new Literal(*loc, b, Token::L_INT8  )); }
{INTEGER}w      { Box b; b.int16_  = strtol  (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_INT16,  new Literal(*loc, b, Token::L_INT16 )); }
{INTEGER}d      { Box b; b.int32_  = strtol  (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_INT32,  new Literal(*loc, b, Token::L_INT32 )); }
{INTEGER}q      { Box b; b.int64_  = strtoll (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_INT64,  new Literal(*loc, b, Token::L_INT64 )); }
{INTEGER}sb     { Box b; b.sat8_   = strtol  (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_SAT8,   new Literal(*loc, b, Token::L_SAT8  )); }
{INTEGER}sw     { Box b; b.sat16_  = strtol  (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_SAT16,  new Literal(*loc, b, Token::L_SAT16 )); }

{INTEGER}u      { Box b; b.uint_   = strtoul (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_UINT,   new Literal(*loc, b, Token::L_UINT  )); }
{INTEGER}ub     { Box b; b.uint8_  = strtoul (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_UINT8,  new Literal(*loc, b, Token::L_UINT8 )); }
{INTEGER}uw     { Box b; b.uint16_ = strtoul (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_UINT16, new Literal(*loc, b, Token::L_UINT16)); }
{INTEGER}ud     { Box b; b.uint32_ = strtoul (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_UINT32, new Literal(*loc, b, Token::L_UINT32)); }
{INTEGER}uq     { Box b; b.uint64_ = strtoull(swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_UINT64, new Literal(*loc, b, Token::L_UINT64)); }
{INTEGER}usb    { Box b; b.usat8_  = strtoul (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_USAT8,  new Literal(*loc, b, Token::L_USAT8 )); }
{INTEGER}usw    { Box b; b.usat16_ = strtoul (swift_text, 0, GET_BASE); RETURN_LITERAL(Token::L_USAT16, new Literal(*loc, b, Token::L_USAT16)); }
                                                                         
{FLOAT}         { Box b; b.float_  = strtof(swift_text, 0);             RETURN_LITERAL(Token::L_REAL,   new Literal(*loc, b, Token::L_REAL  )); }
{FLOAT}d        { Box b; b.float_  = strtof(swift_text, 0);             RETURN_LITERAL(Token::L_REAL32, new Literal(*loc, b, Token::L_REAL32)); }
{FLOAT}q        { Box b; b.double_ = strtod(swift_text, 0);             RETURN_LITERAL(Token::L_REAL64, new Literal(*loc, b, Token::L_REAL64)); }
                                                                         
true            { Box b; b.bool_   = true;                              RETURN_LITERAL(Token::L_BOOL,   new Literal(*loc, b, Token::L_BOOL  )); }
false           { Box b; b.bool_   = false;                             RETURN_LITERAL(Token::L_BOOL,   new Literal(*loc, b, Token::L_BOOL  )); }

{ID} { RETURN_ID(Token::ID); }

\\\n {}

#[^\n]* { /* end of line commment */ }
[\n;][\n; \t]* { RETURN_TOKEN(Token::EOL); }

[ \t]   { /* skip whitespaces */ }

. { printf("%i: error: invalid input character '%c'\n", swift_lineno, swift_text[0]); }

%%

#undef GET_BASE
#undef RETURN_TOKEN
#undef RETURN_ID
#undef RETURN_LITERAL

namespace swift {

FILE* lexer_init(const char* filename) 
{
    delete g_lexer_filename;
    g_lexer_filename = new std::string(filename);
    swift_lineno = 1;
    swift_in = fopen(filename, "r");
    return swift_in;

    // HACK: omit warning
    if (false)
        yyunput(0, 0);
}

}
