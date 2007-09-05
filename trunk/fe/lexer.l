%{

#include <string>
#include <iostream>

#include <cstdio>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/lexer.h"
#include "fe/parser.h"
#include "fe/syntaxtree.h"

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
%x EOL_STATE

%%

[+\-*/%&|\^=()\[\]{},.;:\<>] { // single character tokens
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

if       { return IF; }
else     { return ELSE; }
elif     { return ELIF; }
for      { return FOR; }
while    { return WHILE; }
do_while { return DO_WHILE; }

scope   { keyLine = currentLine; return SCOPE; }
class   { keyLine = currentLine; return CLASS; }
end     { keyLine = currentLine; return END; }

reader  { keyLine = currentLine; return READER; }
writer  { keyLine = currentLine; return WRITER; }
routine { keyLine = currentLine; return ROUTINE; }

create  { keyLine = currentLine; return CREATE; }
destroy { keyLine = currentLine; return DESTROY; }

in      { return IN; }
inout   { return INOUT; }
out     { return OUT; }

shl     { return SHL; }
shr     { return SHR; }
rol     { return ROL; }
ror     { return ROR; }

public      { return PUBLIC; }
protected   { return PROTECTED; }
package     { return PACKAGE; }
private     { return PRIVATE; }

{INTEGER}x      { swiftlval.expr_ = new Literal(L_INDEX ); ((Literal*) swiftlval.expr_)->index_    = strtol (yytext, 0, 10); return L_INDEX; }

{INTEGER}       { swiftlval.expr_ = new Literal(L_INT,    currentLine); ((Literal*) swiftlval.expr_)->int_    = strtol (yytext, 0, 10); return L_INT; }
{INTEGER}b      { swiftlval.expr_ = new Literal(L_INT8,   currentLine); ((Literal*) swiftlval.expr_)->int8_   = strtol (yytext, 0, 10); return L_INT8; }
{INTEGER}w      { swiftlval.expr_ = new Literal(L_INT16,  currentLine); ((Literal*) swiftlval.expr_)->int16_  = strtol (yytext, 0, 10); return L_INT16; }
{INTEGER}d      { swiftlval.expr_ = new Literal(L_INT32,  currentLine); ((Literal*) swiftlval.expr_)->int32_  = strtol (yytext, 0, 10); return L_INT32; }
{INTEGER}q      { swiftlval.expr_ = new Literal(L_INT64,  currentLine); ((Literal*) swiftlval.expr_)->int64_  = strtoll(yytext, 0, 10); return L_INT64; }
{INTEGER}sb     { swiftlval.expr_ = new Literal(L_SAT8,   currentLine); ((Literal*) swiftlval.expr_)->sat8_   = strtol (yytext, 0, 10); return L_SAT8; }
{INTEGER}sw     { swiftlval.expr_ = new Literal(L_SAT16,  currentLine); ((Literal*) swiftlval.expr_)->sat16_  = strtol (yytext, 0, 10); return L_SAT16; }

{INTEGER}u      { swiftlval.expr_ = new Literal(L_UINT,   currentLine); ((Literal*) swiftlval.expr_)->uint_   = strtoul (yytext, 0, 10); return L_UINT; }
{INTEGER}ub     { swiftlval.expr_ = new Literal(L_UINT8,  currentLine); ((Literal*) swiftlval.expr_)->uint8_  = strtoul (yytext, 0, 10); return L_UINT8; }
{INTEGER}uw     { swiftlval.expr_ = new Literal(L_UINT16, currentLine); ((Literal*) swiftlval.expr_)->uint16_ = strtoul (yytext, 0, 10); return L_UINT16; }
{INTEGER}ud     { swiftlval.expr_ = new Literal(L_UINT32, currentLine); ((Literal*) swiftlval.expr_)->uint32_ = strtoul (yytext, 0, 10); return L_UINT32; }
{INTEGER}uq     { swiftlval.expr_ = new Literal(L_UINT64, currentLine); ((Literal*) swiftlval.expr_)->uint64_ = strtoull(yytext, 0, 10); return L_UINT64; }
{INTEGER}usb    { swiftlval.expr_ = new Literal(L_USAT8,  currentLine); ((Literal*) swiftlval.expr_)->usat8_  = strtoul (yytext, 0, 10); return L_USAT8; }
{INTEGER}usw    { swiftlval.expr_ = new Literal(L_USAT16, currentLine); ((Literal*) swiftlval.expr_)->usat16_ = strtoul (yytext, 0, 10); return L_USAT16; }

{FLOAT}         { swiftlval.expr_ = new Literal(L_REAL,   currentLine); ((Literal*) swiftlval.expr_)->real_   = strtod(yytext, 0); return L_REAL; }
{FLOAT}d        { swiftlval.expr_ = new Literal(L_REAL32, currentLine); ((Literal*) swiftlval.expr_)->real32_ = strtod(yytext, 0); return L_REAL32; }
{FLOAT}q        { swiftlval.expr_ = new Literal(L_REAL64, currentLine); ((Literal*) swiftlval.expr_)->real64_ = strtod(yytext, 0); return L_REAL64; }

true            { swiftlval.expr_ = new Literal(L_TRUE,   currentLine); ((Literal*) swiftlval.expr_)->bool_   = true;   return L_TRUE; }
false           { swiftlval.expr_ = new Literal(L_FALSE,  currentLine); ((Literal*) swiftlval.expr_)->bool_   = false;  return L_TRUE; }
nil             { swiftlval.expr_ = new Literal(L_NIL,    currentLine); ((Literal*) swiftlval.expr_)->ptr_    = 0;      return L_TRUE; }

{ID} { keyLine = currentLine; swiftlval.id_ = new std::string(yytext); return ID; }


#.*$ { /* end of line commment */ }

[ \t]   { /* skip whitespaces */ }

\n {
    BEGIN EOL_STATE;
    ++currentLine; // count line number
    return EOL;
}

.       { printf("%i: error: invalid input character '%c'\n", currentLine, yytext[0]); }


<EOL_STATE>[ \t]   { /* skip whitespaces */ }
<EOL_STATE>\n       { ++currentLine; /* count line number */ }
<EOL_STATE>.        { unput(yytext[0]); BEGIN INITIAL; }

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
