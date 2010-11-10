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

#include "fe/auto.h"
#include "fe/cmdlineparser.h"
#include "fe/error.h"
#include "fe/typenode.h"

namespace swift {
    int g_lexer_comment_level;
    std::string* g_lexer_filename = 0;
}

using namespace swift;

#define YY_DECL int swift_lex (Parser::semantic_type* val, Parser::location_type* loc)
#define SET_LOC loc->initialize(g_lexer_filename); loc->end.line = swift_lineno; loc->step()
#define SET_VAL val->expr_ = l
#define SET_STR val->id_ = new std::string(swift_text)
#define GET_BASE (swift_leng > 2 && tolower(swift_text[1]) == 'x') ? 16 : 10

// not needed so omit warning
//FLOAT    {D}+\.{D}*|{D}*\.{D}+

%}

ID      [_a-zA-Z][_a-zA-Z0-9]*

D       [0-9]
H       [0-9a-fA-F]
E       [Ee][+-]?{D}+

FLOAT   {D}+{E}|{D}*\.{D}+({E})?|{D}+\.{D}*({E})?
INTEGER {D}+|0[xX]{H}+

/*
%x CHAR_STATE
%x STRING_STATE
*/
%x COMMENT
%x NESTED_COMMENT
%x EOL_STATE

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
    SET_LOC; return swift_text[0];
}

->   { SET_LOC; return Token::ARROW; }
\<-  { SET_LOC; return Token::MOVE; }
\<-> { SET_LOC; return Token::SWAP; }

[~!]                     { SET_LOC; SET_STR; return Token::NOT; }
`{ID}                    { SET_LOC; SET_STR; return Token::BACKTICK_ID; }
[*/%&]|\<\<|>>           { SET_LOC; SET_STR; return Token::MUL; }
[+\-]                    { SET_LOC; SET_STR; return Token::ADD_SUB; }
[|\^]                    { SET_LOC; SET_STR; return Token::OR_XOR;  }
@{ID}                    { SET_LOC; SET_STR; return Token::AT_ID; }
\<=|>=|\==|!=|\<|>       { SET_LOC; SET_STR; return Token::REL; }
=|\<\<=|>>=|[+\-*/%|^&]= { SET_LOC; SET_STR; return Token::ASGN; }

::  { SET_LOC; return Token::DOUBLE_COLON; }

c_call  { SET_LOC; return Token:: C_CALL; }
vc_call { SET_LOC; return Token::VC_CALL; }

if      { SET_LOC; return Token::IF; }
else    { SET_LOC; return Token::ELSE; }
elif    { SET_LOC; return Token::ELIF; }
for     { SET_LOC; return Token::FOR; }
while   { SET_LOC; return Token::WHILE; }
repeat  { SET_LOC; return Token::REPEAT; }
until   { SET_LOC; return Token::UNTIL; }

return   { SET_LOC; return Token::RETURN; }
break    { SET_LOC; return Token::BREAK; }
continue { SET_LOC; return Token::CONTINUE; }

scope    { SET_LOC; return Token::SCOPE; }
class    { SET_LOC; return Token::CLASS; }
end      { SET_LOC; return Token::END; }
create   { SET_LOC; return Token::CREATE; }
destroy  { SET_LOC; return Token::DESTROY; }
reader   { SET_LOC; return Token::READER; }
writer   { SET_LOC; return Token::WRITER; }
routine  { SET_LOC; return Token::ROUTINE; }

var       { SET_LOC; return Token::VAR; }
const     { SET_LOC; return Token::CONST; }
ptr       { SET_LOC; return Token::PTR; }
array     { SET_LOC; return Token::ARRAY; }
simd      { SET_LOC; return Token::SIMD; }

broadcast  { SET_LOC; return Token::BROADCAST; }
simd_range { SET_LOC; return Token::SIMD_RANGE; }

public      { SET_LOC; return Token::PUBLIC; }
protected   { SET_LOC; return Token::PROTECTED; }
package     { SET_LOC; return Token::PACKAGE; }
private     { SET_LOC; return Token::PRIVATE; }

nil   { SET_LOC; return Token::NIL; }
self  { SET_LOC; return Token::SELF; }

{INTEGER}x      { SET_LOC; Box b; b.size_   = strtol  (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_INDEX ); return Token::L_INDEX; }
                                                                                  
{INTEGER}       { SET_LOC; Box b; b.int_    = strtol  (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_INT   ); return Token::L_INT; }
{INTEGER}b      { SET_LOC; Box b; b.int8_   = strtol  (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_INT8  ); return Token::L_INT8; }
{INTEGER}w      { SET_LOC; Box b; b.int16_  = strtol  (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_INT16 ); return Token::L_INT16; }
{INTEGER}d      { SET_LOC; Box b; b.int32_  = strtol  (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_INT32 ); return Token::L_INT32; }
{INTEGER}q      { SET_LOC; Box b; b.int64_  = strtoll (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_INT64 ); return Token::L_INT64; }
{INTEGER}sb     { SET_LOC; Box b; b.sat8_   = strtol  (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_SAT8  ); return Token::L_SAT8; }
{INTEGER}sw     { SET_LOC; Box b; b.sat16_  = strtol  (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_SAT16 ); return Token::L_SAT16; }
                                                                                  
{INTEGER}u      { SET_LOC; Box b; b.uint_   = strtoul (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_UINT  ); return Token::L_UINT; }
{INTEGER}ub     { SET_LOC; Box b; b.uint8_  = strtoul (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_UINT8 ); return Token::L_UINT8; }
{INTEGER}uw     { SET_LOC; Box b; b.uint16_ = strtoul (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_UINT16); return Token::L_UINT16; }
{INTEGER}ud     { SET_LOC; Box b; b.uint32_ = strtoul (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_UINT32); return Token::L_UINT32; }
{INTEGER}uq     { SET_LOC; Box b; b.uint64_ = strtoull(swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_UINT64); return Token::L_UINT64; }
{INTEGER}usb    { SET_LOC; Box b; b.usat8_  = strtoul (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_USAT8 ); return Token::L_USAT8; }
{INTEGER}usw    { SET_LOC; Box b; b.usat16_ = strtoul (swift_text, 0, GET_BASE); val->expr_ = new Literal(*loc, b, Token::L_USAT16); return Token::L_USAT16; }
                                                                                  
{FLOAT}         { SET_LOC; Box b; b.float_  = strtof(swift_text, 0);       val->expr_ = new Literal(*loc, b, Token::L_REAL  ); return Token::L_REAL; }
{FLOAT}d        { SET_LOC; Box b; b.float_  = strtof(swift_text, 0);       val->expr_ = new Literal(*loc, b, Token::L_REAL32); return Token::L_REAL32; }
{FLOAT}q        { SET_LOC; Box b; b.double_ = strtod(swift_text, 0);       val->expr_ = new Literal(*loc, b, Token::L_REAL64); return Token::L_REAL64; }
                                                                                  
true            { SET_LOC; Box b; b.bool_   = true;                    val->expr_ = new Literal(*loc, b, Token::L_BOOL  ); return Token::L_BOOL; }
false           { SET_LOC; Box b; b.bool_   = false;                   val->expr_ = new Literal(*loc, b, Token::L_BOOL  ); return Token::L_BOOL; }

{ID} { SET_LOC; SET_STR; return Token::ID; }

\\\n {}

#[^\n]* { /* end of line commment */ }
[\n;][\n; \t]* { SET_LOC; return Token::EOL; }

[ \t]   { /* skip whitespaces */ }

. { printf("%i: error: invalid input character '%c'\n", swift_lineno, swift_text[0]); }

%%

#undef SET_LOC
#undef SET_VAL
#undef SET_STR
#undef GET_BASE

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

#undef SET_LOC
#undef SET_VAL
