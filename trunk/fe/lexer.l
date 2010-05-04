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

%{

#include <string>
#include <iostream>

#include <cstdio>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/parser.h"
#include "fe/syntaxtree.h"


namespace swift {
    int g_line = 0;
    int g_comment_level;
}

using namespace swift;

#define YY_DECL int swift_lex (Parser::semantic_type* val, Parser::location_type* loc)

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

[+\-*/%&|\^=()\[\]{},.;:\<>@] { // single character tokens
    return yytext[0];
}

::  { return Token::DOUBLE_COLON; }

mod { return Token::MOD_OP; }
div { return Token::DIV_OP; }
==  { return Token::EQ_OP; }
\<> { return Token::NE_OP; }
\<= { return Token::LE_OP; }
>=  { return Token::GE_OP; }
and { return Token::AND_OP; }
or  { return Token::OR_OP; }
xor { return Token::XOR_OP; }
not { return Token::NOT_OP; }

c_call  { return Token:: C_CALL; }
vc_call { return Token::VC_CALL; }

inc { return Token::INC_OP; }
dec { return Token::DEC_OP; }

\+= { return  Token::ADD_ASSIGN; }
\-= { return  Token::SUB_ASSIGN; }
\*= { return  Token::MUL_ASSIGN; }
\/= { return  Token::DIV_ASSIGN; }
mod= { return Token::MOD_ASSIGN; }
and= { return Token::AND_ASSIGN; }
or=  { return Token:: OR_ASSIGN; }
xor= { return Token::XOR_ASSIGN; }

\<-     { return Token::MOVE_OP; }
\<->    { return Token::SWAP_OP; }
->      { return Token::ARROW; }

if      { return Token::IF; }
else    { return Token::ELSE; }
elif    { return Token::ELIF; }
for     { return Token::FOR; }
while   { return Token::WHILE; }
repeat  { return Token::REPEAT; }
until   { return Token::UNTIL; }

return   { return Token::RETURN; }
break    { return Token::BREAK; }
continue { return Token::CONTINUE; }

scope    { return Token::SCOPE; }
class    { return Token::CLASS; }
end      { return Token::END; }
create   { return Token::CREATE; }
destroy  { return Token::DESTROY; }
reader   { return Token::READER; }
writer   { return Token::WRITER; }
assign   { return Token::ASSIGN; }
routine  { return Token::ROUTINE; }
operator { return Token::OPERATOR; }


inout   { return Token::INOUT; }

shl     { return Token::SHL; }
shr     { return Token::SHR; }
rol     { return Token::ROL; }
ror     { return Token::ROR; }

var   { return Token::VAR; }
const { return Token::CONST; }
ptr   { return Token::PTR; }
array { return Token::ARRAY; }
simd  { return Token::SIMD; }

public      { return Token::PUBLIC; }
protected   { return Token::PROTECTED; }
package     { return Token::PACKAGE; }
private     { return Token::PRIVATE; }

nil   { return Token::NIL; }
self  { return Token::SELF; }

{INTEGER}x      { val->expr_ = new Literal(Token::L_INDEX,  g_line); ((Literal*) val->expr_)->box_.size_   = strtol (yytext, 0, 10); return Token::L_INDEX; }

{INTEGER}       { val->expr_ = new Literal(Token::L_INT,    g_line); ((Literal*) val->expr_)->box_.int_    = strtol (yytext, 0, 10); return Token::L_INT; }
{INTEGER}b      { val->expr_ = new Literal(Token::L_INT8,   g_line); ((Literal*) val->expr_)->box_.int8_   = strtol (yytext, 0, 10); return Token::L_INT8; }
{INTEGER}w      { val->expr_ = new Literal(Token::L_INT16,  g_line); ((Literal*) val->expr_)->box_.int16_  = strtol (yytext, 0, 10); return Token::L_INT16; }
{INTEGER}d      { val->expr_ = new Literal(Token::L_INT32,  g_line); ((Literal*) val->expr_)->box_.int32_  = strtol (yytext, 0, 10); return Token::L_INT32; }
{INTEGER}q      { val->expr_ = new Literal(Token::L_INT64,  g_line); ((Literal*) val->expr_)->box_.int64_  = strtoll(yytext, 0, 10); return Token::L_INT64; }
{INTEGER}sb     { val->expr_ = new Literal(Token::L_SAT8,   g_line); ((Literal*) val->expr_)->box_.sat8_   = strtol (yytext, 0, 10); return Token::L_SAT8; }
{INTEGER}sw     { val->expr_ = new Literal(Token::L_SAT16,  g_line); ((Literal*) val->expr_)->box_.sat16_  = strtol (yytext, 0, 10); return Token::L_SAT16; }

{INTEGER}u      { val->expr_ = new Literal(Token::L_UINT,   g_line); ((Literal*) val->expr_)->box_.uint_   = strtoul (yytext, 0, 10); return Token::L_UINT; }
{INTEGER}ub     { val->expr_ = new Literal(Token::L_UINT8,  g_line); ((Literal*) val->expr_)->box_.uint8_  = strtoul (yytext, 0, 10); return Token::L_UINT8; }
{INTEGER}uw     { val->expr_ = new Literal(Token::L_UINT16, g_line); ((Literal*) val->expr_)->box_.uint16_ = strtoul (yytext, 0, 10); return Token::L_UINT16; }
{INTEGER}ud     { val->expr_ = new Literal(Token::L_UINT32, g_line); ((Literal*) val->expr_)->box_.uint32_ = strtoul (yytext, 0, 10); return Token::L_UINT32; }
{INTEGER}uq     { val->expr_ = new Literal(Token::L_UINT64, g_line); ((Literal*) val->expr_)->box_.uint64_ = strtoull(yytext, 0, 10); return Token::L_UINT64; }
{INTEGER}usb    { val->expr_ = new Literal(Token::L_USAT8,  g_line); ((Literal*) val->expr_)->box_.usat8_  = strtoul (yytext, 0, 10); return Token::L_USAT8; }
{INTEGER}usw    { val->expr_ = new Literal(Token::L_USAT16, g_line); ((Literal*) val->expr_)->box_.usat16_ = strtoul (yytext, 0, 10); return Token::L_USAT16; }

{FLOAT}         { val->expr_ = new Literal(Token::L_REAL,   g_line); ((Literal*) val->expr_)->box_.float_  = strtod(yytext, 0); return Token::L_REAL; }
{FLOAT}d        { val->expr_ = new Literal(Token::L_REAL32, g_line); ((Literal*) val->expr_)->box_.float_  = strtod(yytext, 0); return Token::L_REAL32; }
{FLOAT}q        { val->expr_ = new Literal(Token::L_REAL64, g_line); ((Literal*) val->expr_)->box_.double_ = strtod(yytext, 0); return Token::L_REAL64; }

true            { val->expr_ = new Literal(Token::L_TRUE,   g_line); ((Literal*) val->expr_)->box_.bool_   = true;  return Token::L_TRUE; }
false           { val->expr_ = new Literal(Token::L_FALSE,  g_line); ((Literal*) val->expr_)->box_.bool_   = false; return Token::L_TRUE; }

{ID} { val->id_ = new std::string(yytext); return Token::ID; }


#[^\n]* { /* end of line commment */ }

[ \t]   { /* skip whitespaces */ }

\\\n { ++g_line; }

\n {
    BEGIN EOL_STATE;
    ++g_line; // count line number
    return Token::EOL;
}

. { printf("%i: error: invalid input character '%c'\n", g_line, yytext[0]); }


<EOL_STATE>[ \t]    { /* skip whitespaces */ }
<EOL_STATE>#[^\n]*  { /* end of line commment */ }
<EOL_STATE>\n       { ++g_line; /* count line number */ }
<EOL_STATE>.        { unput(yytext[0]); BEGIN INITIAL; }

\/\*            { BEGIN COMMENT; }
<COMMENT>\*\/   { BEGIN INITIAL; }
<COMMENT>\n     { ++g_line; /* count line number */ }
<COMMENT>.      { /*skip*/  }

\/\+                    { BEGIN NESTED_COMMENT; ++g_comment_level; }
<NESTED_COMMENT>\/\+    { ++g_comment_level;  }
<NESTED_COMMENT>\+\/    { --g_comment_level; if (g_comment_level == 0) BEGIN INITIAL; }
<NESTED_COMMENT>\n      { ++g_line; }
<NESTED_COMMENT>.       { /*skip*/ }


%%

int yywrap()
{
    return 1;
}

namespace swift {
    FILE* lexer_init(const char* filename) 
    {
        g_line = 1;
        yyin = fopen(filename, "r");
        return yyin;
    }
}
