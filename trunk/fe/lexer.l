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

%{

#include <string>
#include <iostream>

#include <cstdio>

#include "utils/assert.h"

#include "fe/auto.h"
#include "fe/cmdlineparser.h"
#include "fe/error.h"
#include "fe/typenode.h"


namespace swift {
    int g_lexer_line = 0;
    int g_lexer_comment_level;
    std::string* g_lexer_filename = 0;
}

using namespace swift;

#define YY_DECL int swift_lex (Parser::semantic_type* val, Parser::location_type* loc)
#define SET_LOC loc->initialize(g_lexer_filename); loc->end.line = g_lexer_line; loc->step()
#define SET_VAL val->expr_ = l

%}

ID          [_a-zA-Z][a-zA-Z0-9_]*
DIGIT       [0-9]

FLOAT       {DIGIT}+\.{DIGIT}*|{DIGIT}*\.{DIGIT}+
INTEGER     {DIGIT}+

/*
%x CHAR_STATE
%x STRING_STATE
*/
%x COMMENT
%x NESTED_COMMENT
%x EOL_STATE

%%

[()\[\]{},.;:@] { // single character tokens
    SET_LOC; return yytext[0];
}

\+  { SET_LOC; return Token::ADD; }
-   { SET_LOC; return Token::SUB; }
\*  { SET_LOC; return Token::MUL; }
\/  { SET_LOC; return Token::DIV; }
%   { SET_LOC; return Token::MOD; }
&   { SET_LOC; return Token::AND; }
\|  { SET_LOC; return Token::OR; }
\^  { SET_LOC; return Token::XOR; }
!   { SET_LOC; return Token::NOT; }
and { SET_LOC; return Token::L_AND; }
or  { SET_LOC; return Token::L_OR; }
xor { SET_LOC; return Token::L_XOR; }
not { SET_LOC; return Token::L_NOT; }
==  { SET_LOC; return Token::EQ; }
\<> { SET_LOC; return Token::NE; }
!=  { SET_LOC; return Token::NE; /* TODO which variant should be final? */ }
\<  { SET_LOC; return Token::LT; }
\<= { SET_LOC; return Token::LE; }
>   { SET_LOC; return Token::GT; }
>=  { SET_LOC; return Token::GE; }
inc { SET_LOC; return Token::INC; }
dec { SET_LOC; return Token::DEC; }

=   { SET_LOC; return Token::ASGN; }
::  { SET_LOC; return Token::DOUBLE_COLON; }

c_call  { SET_LOC; return Token:: C_CALL; }
vc_call { SET_LOC; return Token::VC_CALL; }

\+=  { SET_LOC; return Token::ASGN_ADD; }
\-=  { SET_LOC; return Token::ASGN_SUB; }
\*=  { SET_LOC; return Token::ASGN_MUL; }
\/=  { SET_LOC; return Token::ASGN_DIV; }
mod= { SET_LOC; return Token::ASGN_MOD; }
and= { SET_LOC; return Token::ASGN_AND; }
or=  { SET_LOC; return Token::ASGN_OR; }
xor= { SET_LOC; return Token::ASGN_XOR; }

\<-     { SET_LOC; return Token::MOVE; }
\<->    { SET_LOC; return Token::SWAP; }
->      { SET_LOC; return Token::ARROW; }

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
assign   { SET_LOC; return Token::ASSIGN; }
writer   { SET_LOC; return Token::WRITER; }
routine  { SET_LOC; return Token::ROUTINE; }
operator { SET_LOC; return Token::OPERATOR; }


shl     { SET_LOC; return Token::SHL; }
shr     { SET_LOC; return Token::SHR; }
rol     { SET_LOC; return Token::ROL; }
ror     { SET_LOC; return Token::ROR; }

var   { SET_LOC; return Token::VAR; }
const { SET_LOC; return Token::CONST; }
ptr   { SET_LOC; return Token::PTR; }
array { SET_LOC; return Token::ARRAY; }
simd  { SET_LOC; return Token::SIMD; }

public      { SET_LOC; return Token::PUBLIC; }
protected   { SET_LOC; return Token::PROTECTED; }
package     { SET_LOC; return Token::PACKAGE; }
private     { SET_LOC; return Token::PRIVATE; }

nil   { SET_LOC; return Token::NIL; }
self  { SET_LOC; return Token::SELF; }

{INTEGER}x      { SET_LOC; Box b; b.size_   = strtol  (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_INDEX ); return Token::L_INDEX; }
                                                                                  
{INTEGER}       { SET_LOC; Box b; b.int_    = strtol  (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_INT   ); return Token::L_INT; }
{INTEGER}b      { SET_LOC; Box b; b.int8_   = strtol  (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_INT8  ); return Token::L_INT8; }
{INTEGER}w      { SET_LOC; Box b; b.int16_  = strtol  (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_INT16 ); return Token::L_INT16; }
{INTEGER}d      { SET_LOC; Box b; b.int32_  = strtol  (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_INT32 ); return Token::L_INT32; }
{INTEGER}q      { SET_LOC; Box b; b.int64_  = strtoll (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_INT64 ); return Token::L_INT64; }
{INTEGER}sb     { SET_LOC; Box b; b.sat8_   = strtol  (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_SAT8  ); return Token::L_SAT8; }
{INTEGER}sw     { SET_LOC; Box b; b.sat16_  = strtol  (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_SAT16 ); return Token::L_SAT16; }
                                                                                  
{INTEGER}u      { SET_LOC; Box b; b.uint_   = strtoul (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_UINT  ); return Token::L_UINT; }
{INTEGER}ub     { SET_LOC; Box b; b.uint8_  = strtoul (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_UINT8 ); return Token::L_UINT8; }
{INTEGER}uw     { SET_LOC; Box b; b.uint16_ = strtoul (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_UINT16); return Token::L_UINT16; }
{INTEGER}ud     { SET_LOC; Box b; b.uint32_ = strtoul (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_UINT32); return Token::L_UINT32; }
{INTEGER}uq     { SET_LOC; Box b; b.uint64_ = strtoull(yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_UINT64); return Token::L_UINT64; }
{INTEGER}usb    { SET_LOC; Box b; b.usat8_  = strtoul (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_USAT8 ); return Token::L_USAT8; }
{INTEGER}usw    { SET_LOC; Box b; b.usat16_ = strtoul (yytext, 0, 10); val->expr_ = new Literal(*loc, b, Token::L_USAT16); return Token::L_USAT16; }
                                                                                  
{FLOAT}         { SET_LOC; Box b; b.float_  = strtod(yytext, 0);       val->expr_ = new Literal(*loc, b, Token::L_REAL  ); return Token::L_REAL; }
{FLOAT}d        { SET_LOC; Box b; b.float_  = strtod(yytext, 0);       val->expr_ = new Literal(*loc, b, Token::L_REAL32); return Token::L_REAL32; }
{FLOAT}q        { SET_LOC; Box b; b.double_ = strtod(yytext, 0);       val->expr_ = new Literal(*loc, b, Token::L_REAL64); return Token::L_REAL64; }
                                                                                  
true            { SET_LOC; Box b; b.bool_   = true;                    val->expr_ = new Literal(*loc, b, Token::L_TRUE  ); return Token::L_TRUE; }
false           { SET_LOC; Box b; b.bool_   = false;                   val->expr_ = new Literal(*loc, b, Token::L_FALSE ); return Token::L_TRUE; }

{ID} { SET_LOC; val->id_ = new std::string(yytext); return Token::ID; }


#[^\n]* { /* end of line commment */ }

[ \t]   { /* skip whitespaces */ }

\\\n { ++g_lexer_line; }

\n {
    BEGIN EOL_STATE;
    SET_LOC;
    ++g_lexer_line; // count line number
    return Token::EOL;
}

. { printf("%i: error: invalid input character '%c'\n", g_lexer_line, yytext[0]); }


<EOL_STATE>[ \t]    { /* skip whitespaces */ }
<EOL_STATE>#[^\n]*  { /* end of line commment */ }
<EOL_STATE>\n       { ++g_lexer_line; /* count line number */ }
<EOL_STATE>.        { unput(yytext[0]); BEGIN INITIAL; }

\/\*            { BEGIN COMMENT; }
<COMMENT>\*\/   { BEGIN INITIAL; }
<COMMENT>\n     { ++g_lexer_line; /* count line number */ }
<COMMENT>.      { /*skip*/  }

\/\+                    { BEGIN NESTED_COMMENT; ++g_lexer_comment_level; }
<NESTED_COMMENT>\/\+    { ++g_lexer_comment_level;  }
<NESTED_COMMENT>\+\/    { --g_lexer_comment_level; if (g_lexer_comment_level == 0) BEGIN INITIAL; }
<NESTED_COMMENT>\n      { ++g_lexer_line; }
<NESTED_COMMENT>.       { /*skip*/ }

%%

namespace swift {

FILE* lexer_init(const char* filename) 
{
    delete g_lexer_filename;
    g_lexer_filename = new std::string(filename);
    g_lexer_line = 1;
    yyin = fopen(filename, "r");
    return yyin;
}

}

#undef SET_LOC
#undef SET_VAL
