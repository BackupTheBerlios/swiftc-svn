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

using namespace swift;

int keyLine = -1;
int commentLevel = 0;
int indentationLevel = 0;

int swift::currentLine = 1;

int swift::getKeyLine()
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

mod { return MOD_OP; }
div { return DIV_OP; }
==  { return EQ_OP; }
\<> { return NE_OP; }
\<= { return LE_OP; }
>=  { return GE_OP; }
and { return AND_OP; }
or  { return OR_OP; }
xor { return XOR_OP; }
not { return NOT_OP; }

inc { return INC_OP; }
dec { return DEC_OP; }

\+= { return ADD_ASSIGN; }
\-= { return SUB_ASSIGN; }
\*= { return MUL_ASSIGN; }
\/= { return DIV_ASSIGN; }
mod= { return MOD_ASSIGN; }
and= { return AND_ASSIGN; }
or=  { return  OR_ASSIGN; }
xor= { return XOR_ASSIGN; }

\<-     { return MOVE_OP; }
\<->    { return SWAP_OP; }
->      { return ARROW; }

if       { return IF; }
else     { return ELSE; }
elif     { return ELIF; }
for      { return FOR; }
while    { return WHILE; }
do_while { return DO_WHILE; }

return   { return RETURN; }
break    { return BREAK; }
continue { return CONTINUE; }

scope   { keyLine = currentLine; return SCOPE; }
class   { keyLine = currentLine; return CLASS; }
end     { keyLine = currentLine; return END; }

reader      { keyLine = currentLine; return READER; }
writer      { keyLine = currentLine; return WRITER; }
routine     { keyLine = currentLine; return ROUTINE; }
operator    { keyLine = currentLine; return OPERATOR; }

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

int yywrap()
{
    return 1;
}

FILE* swift::lexerInit(const char* filename) {
    currentLine = 1;
    yyin = fopen(filename, "r");
    return yyin;
}
