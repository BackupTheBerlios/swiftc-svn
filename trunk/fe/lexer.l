%{

#include <string>
#include <iostream>

#include <cstdio>

#include "../utils/assert.h"

#include "error.h"
#include "lexer.h"
#include "syntaxtree.h"
#include "expr.h"

int keyLine = -1;
int commentLevel = 0;
int indentationLevel = 0;

int currentLine = 1;

int getKeyLine()
{
    int result = keyLine;
    keyLine = -1;

    return result;
}

%}

ID          [a-zA-Z][a-zA-Z0-9_]*
DIGIT       [0-9]

FLOAT       {DIGIT}+\.{DIGIT}*|{DIGIT}*\.{DIGIT}+
INTEGER     {DIGIT}+

/*
%x CHAR_STATE
%x STRING_STATE
*/
%x COMMENT
%x NESTED_COMMENT

%%

[+\-*/%&|\^=()\[\]{},.;:] { // single character tokens
    return yytext[0];
}

==  { return EQ_OP; }
!=  { return NE_OP; }
\<= { return LE_OP; }
>=  { return GE_OP; }

inc { return INC; }
dec { return DEC; }

\+= { return ADD_ASSIGN; }
\-= { return SUB_ASSIGN; }
\*= { return MUL_ASSIGN; }
\/= { return DIV_ASSIGN; }
%=  { return MOD_ASSIGN; }
\|= { return  OR_ASSIGN; }
\^= { return XOR_ASSIGN; }

\<-     { return MOVE_OP; }
\<->    { return SWAP_OP; }
->      { return PTR_OP; }

var { keyLine = currentLine; return VAR; }
cst { keyLine = currentLine; return CST; }
def { keyLine = currentLine; return DEF; }

int     { return INT; }
int8    { return INT8; }
int16   { return INT16; }
int32   { return INT32; }
int64   { return INT64; }
sat8    { return SAT8; }
sat16   { return SAT16; }

uint    { return UINT; }
uint8   { return UINT8; }
uint16  { return UINT16; }
uint32  { return UINT32; }
uint64  { return UINT64; }
usat8   { return USAT8; }
usat16  { return USAT16; }

real    { return REAL; }
real32  { return REAL32; }
real64  { return REAL64; }

char    { return CHAR; }
char8   { return CHAR8; }
char16  { return CHAR16; }

string   { return STRING; }
string8  { return STRING8; }
string16 { return STRING16; }

bool    { return BOOL; }

array   { return ARRAY; }
simd    { return SIMD; }

scope   { keyLine = currentLine; return SCOPE; }
class   { keyLine = currentLine; return CLASS; }
end     { keyLine = currentLine; return END; }

reader  { keyLine = currentLine; return READER; }
writer  { keyLine = currentLine; return WRITER; }
routine { keyLine = currentLine; return ROUTINE; }

create  { return CREATE; }
destroy { return DESTROY; }

in      { return IN; }
inout   { return INOUT; }
out     { return OUT; }

true    { return TRUE_; }
false   { return FALSE_; }
nil     { return NIL; }

shl     { return SHL; }
shr     { return SHR; }
rol     { return ROL; }
ror     { return ROR; }

public      { return PUBLIC; }
protected   { return PROTECTED; }
package     { return PACKAGE; }
private     { return PRIVATE; }

{INTEGER}x      { yylval.expr_ = new Literal(L_INDEX ); ((Literal*) yylval.expr_)->index_    = strtol (yytext, 0, 10); return L_INDEX; }

{INTEGER}       { yylval.expr_ = new Literal(L_INT,    currentLine); ((Literal*) yylval.expr_)->int_    = strtol (yytext, 0, 10); return L_INT; }
{INTEGER}b      { yylval.expr_ = new Literal(L_INT8,   currentLine); ((Literal*) yylval.expr_)->int8_   = strtol (yytext, 0, 10); return L_INT8; }
{INTEGER}w      { yylval.expr_ = new Literal(L_INT16,  currentLine); ((Literal*) yylval.expr_)->int16_  = strtol (yytext, 0, 10); return L_INT16; }
{INTEGER}d      { yylval.expr_ = new Literal(L_INT32,  currentLine); ((Literal*) yylval.expr_)->int32_  = strtol (yytext, 0, 10); return L_INT32; }
{INTEGER}q      { yylval.expr_ = new Literal(L_INT64,  currentLine); ((Literal*) yylval.expr_)->int64_  = strtoll(yytext, 0, 10); return L_INT64; }
{INTEGER}sb     { yylval.expr_ = new Literal(L_SAT8,   currentLine); ((Literal*) yylval.expr_)->sat8_   = strtol (yytext, 0, 10); return L_SAT8; }
{INTEGER}sw     { yylval.expr_ = new Literal(L_SAT16,  currentLine); ((Literal*) yylval.expr_)->sat16_  = strtol (yytext, 0, 10); return L_SAT16; }

{INTEGER}u      { yylval.expr_ = new Literal(L_UINT,   currentLine); ((Literal*) yylval.expr_)->uint_   = strtoul (yytext, 0, 10); return L_UINT; }
{INTEGER}ub     { yylval.expr_ = new Literal(L_UINT8,  currentLine); ((Literal*) yylval.expr_)->uint8_  = strtoul (yytext, 0, 10); return L_UINT8; }
{INTEGER}uw     { yylval.expr_ = new Literal(L_UINT16, currentLine); ((Literal*) yylval.expr_)->uint16_ = strtoul (yytext, 0, 10); return L_UINT16; }
{INTEGER}ud     { yylval.expr_ = new Literal(L_UINT32, currentLine); ((Literal*) yylval.expr_)->uint32_ = strtoul (yytext, 0, 10); return L_UINT32; }
{INTEGER}uq     { yylval.expr_ = new Literal(L_UINT64, currentLine); ((Literal*) yylval.expr_)->uint64_ = strtoull(yytext, 0, 10); return L_UINT64; }
{INTEGER}usb    { yylval.expr_ = new Literal(L_USAT8,  currentLine); ((Literal*) yylval.expr_)->usat8_  = strtoul (yytext, 0, 10); return L_USAT8; }
{INTEGER}usw    { yylval.expr_ = new Literal(L_USAT16, currentLine); ((Literal*) yylval.expr_)->usat16_ = strtoul (yytext, 0, 10); return L_USAT16; }

{FLOAT}         { yylval.expr_ = new Literal(L_REAL,   currentLine); ((Literal*) yylval.expr_)->real_   = strtod(yytext, 0); return L_REAL; }
{FLOAT}d        { yylval.expr_ = new Literal(L_REAL32, currentLine); ((Literal*) yylval.expr_)->real32_ = strtod(yytext, 0); return L_REAL32; }
{FLOAT}q        { yylval.expr_ = new Literal(L_REAL64, currentLine); ((Literal*) yylval.expr_)->real64_ = strtod(yytext, 0); return L_REAL64; }

{ID} { keyLine = currentLine; yylval.id_ = new std::string(yytext); return ID; }


#.*$ { /* end of line commment */ }

[ \t]   { /* skip whitespaces */ }

\n {
    ++currentLine; // count line number
}
.       { printf("%i: error: invalid input character '%c'\n", currentLine, yytext[0]); }

\/\*            { BEGIN COMMENT; }
<COMMENT>\*\/   { BEGIN INITIAL; }
<COMMENT>\n     { ++currentLine; /* count line number */ }
<COMMENT>.      { /*skip*/  }

\/\+                    { BEGIN NESTED_COMMENT; ++commentLevel; }
<NESTED_COMMENT>\/\+    { ++commentLevel;  }
<NESTED_COMMENT>\+\/    { --commentLevel; if (commentLevel == 0) BEGIN INITIAL; }
<NESTED_COMMENT>\n      { ++currentLine; }
<NESTED_COMMENT>.       { /*skip*/ }


%%

int yywrap() {
    return 1;
}

FILE* lexerInit(const char* filename) {
    yyin = fopen(filename, "r");
    return yyin;
}