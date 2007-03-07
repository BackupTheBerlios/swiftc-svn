%{

#include <iostream>

#include "error.h"
#include "lexer.h"
#include "symboltable.h"
#include "syntaxtree.h"
#include "class.h"
#include "type.h"
#include "statement.h"
#include "expr.h"

swift::SyntaxTree*  syntaxTree_;
int pointerCount_;

using swift::symtab;
using swift::currentLine;
using swift::getKeyLine;

%}

%union
{
    int                 int_;

    std::string*        id_;

    swift::Module*      module_;
    swift::Definition*  definition_;

    swift::Class*       class_;
    swift::ClassMember* classMember_;
    swift::MemberVar*   memberVar_;
    swift::Method*      method_;
    swift::Parameter*   parameter_;

    swift::Type*        type_;
    swift::BaseType*    baseType_;

    swift::Expr*        expr_;
    swift::Arg*         arg_;

    swift::Statement*   statement_;
};

/*
    tokens
*/

// literals
%token <expr_>  L_INDEX
%token <expr_>  L_INT  L_INT8   L_INT16  L_INT32  L_INT64  L_SAT8  L_SAT16
%token <expr_> L_UINT L_UINT8  L_UINT16 L_UINT32 L_UINT64 L_USAT8 L_USAT16
%token <expr_> L_REAL L_REAL32 L_REAL64

// types
%token INDEX
%token  INT  INT8   INT16  INT32  INT64  SAT8  SAT16
%token UINT UINT8  UINT16 UINT32 UINT64 USAT8 USAT16
%token REAL REAL32 REAL64
%token CHAR CHAR8  CHAR16
%token STRING STRING8 STRING16
%token BOOL

// built-in template types
%token ARRAY SIMD

// special values
%token TRUE_ FALSE_ NIL

// type qualifier
%token VAR CST DEF
// parameter qualifier
%token IN INOUT OUT
// method qualifier
%token READER WRITER ROUTINE
// constructor / destructor
%token CREATE DESTROY

// built-in functions
%token SHL SHR ROL ROR

// operators
%token INC DEC
%token EQ_OP NE_OP
%token LE_OP GE_OP
%token MOVE_OP SWAP_OP PTR_OP

%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN AND_ASSIGN OR_ASSIGN XOR_ASSIGN

// protection
%token PUBLIC PROTECTED PACKAGE PRIVATE

// miscellaneous
%token SCOPE CLASS END


// controll commands
%token RETURN RESULT BREAK CONTINUE FOR FOR_EACH WHILE DO_WHILE REPEAT


%token <integer_>       INTEGER
%token <float_>         FLOAT
%token <id_>            ID

/*
    types
*/

%type <int_>        simple_type type_qualifier method_qualifier parameter_qualifier
%type <type_>       type
%type <parameter_>  parameter parameter_list

%type <definition_> class_definition definitions
%type <classMember_> class_body class_member
%type <method_>     method
%type <memberVar_>  member_var

%type <module_>     module
%type <expr_>       expr assign_expr mul_expr add_expr postfix_expr un_expr primary_expr
%type <arg_>        arg_list

%type <statement_>  statement_list statement
%type <baseType_>   base_type

%start module

%%

module
    :   {
            $$ = new swift::Module(new std::string("default"), currentLine);
            symtab.insert($$);
        }
        definitions
        {
            $$ = $<module_>1;
            $$->definitions_ = $2;
            $$->parent_ = 0;
            syntaxTree_->rootModule_ = $$;
        }
    ;

definitions
    : /*empty*/                                 { $$ = 0; }
    | class_definition opt_semis definitions    { $$ = $1; $$->next_ = $3; }
    ;

/*
    *******
    classes
    *******
*/

class_definition
    : CLASS ID
        {
            $$ = new swift::Class($2, currentLine);
            symtab.insert($<class_>$);
        }
        class_body END
        {
            $$ = $<class_>3;
            $<class_>$->classMember_= $4;
        }
    | CLASS ID
        {
            $$ = new swift::Class($2, currentLine);
            symtab.insert($<class_>$);
        }
        '{' template_list '}' class_body END
        {
            $$ = $<class_>3;
            $<class_>$->classMember_= $7;
        }
    ;

template_list
    : template_parameter
    | template_list ',' template_parameter
    ;

template_parameter
    : type { delete $1; }
    ;

class_body
    : /*empty*/                 { $$ = 0; }
    | class_member class_body   { $$ = $1; $1->next_ = $2; }
    | ';' class_body            { $$ = $2; }
    ;

class_member
    : method        { $$ = $1; }
    | member_var    { $$ = $1; }
    ;

/*
    *******
    methods
    *******
*/

method
    : method_qualifier /**/ ID
        {
            $$ = new swift::Method( $1, 0, $2, getKeyLine() );
            symtab.insert($$);
        }
        '(' parameter_list')' statement_list END
        {
            $$ = $<method_>3;
            $$->statements_ = $7;
        }
    | method_qualifier type ID
        {
            $$ = new swift::Method( $1, $2, $3, getKeyLine() );
            symtab.insert($$);
        }
        '(' parameter_list')' statement_list END
        {
            $$ = $<method_>4;
            $$->statements_ = $8;
        }
    ;

parameter_list
    : /*emtpy*/                     { $$ = 0; }
    | parameter                     { $$ = $1; $$->next_ = 0; }
    | parameter ',' parameter_list  { $$ = $1; $$->next_ = $3; }
    ;

parameter
    : /* default is IN */ type ID
        {
            $$ = new swift::Parameter(IN, $1, $2, currentLine);
            symtab.insert($$);
        }
    | parameter_qualifier type ID
        {
            $$ = new swift::Parameter($1, $2, $3, currentLine);
            symtab.insert($$);
        }
    ;

/*
    ***********
    member vars
    ***********
*/

member_var
    : type ID ';'
        {
            $$ = new swift::MemberVar($1, $2, currentLine);
            symtab.insert($$);
        }
    ;

/*
    **********
    statements
    **********
*/

statement_list
    : /*emtpy*/                 { $$ = 0; }
    | statement statement_list  { $$ = $1; $$->next_ = $2; }
    ;

statement
    : expr ';' opt_semis    { $$ = new swift::ExprStatement($1, currentLine); }
    | type ID ';'           { $$ = new swift::Declaration($1, $2, getKeyLine()); }
    ;

/*
    ***********
    expressions
    ***********
*/

expr
    : assign_expr               { $$ = $1; }
    | expr ',' assign_expr      { $$ = new swift::BinExpr(',', $1, $3, currentLine); }
    ;

assign_expr
    : add_expr                  { $$ = $1; }
    | assign_expr '=' add_expr  { $$ = new swift::AssignExpr('=', $1, $3, currentLine); }
    ;

/*
    : rel_expr                  { $$ = $1; }
    | assign_expr '=' rel_expr  { $$ = new swift::AssignExpr($1, $3, currentLine); }
    ;

rel_expr
    : add_expr
    | rel_expr '<' add_expr
    | rel_expr '>' add_expr
    | rel_expr LE add_expr
    | rel_expr GE add_expr
    ;*/

add_expr
    : mul_expr              { $$ = $1; }
    | add_expr '+' mul_expr { $$ = new swift::BinExpr('+', $1, $3, currentLine); }
    | add_expr '-' mul_expr { $$ = new swift::BinExpr('-', $1, $3, currentLine); }
    ;

mul_expr:
      un_expr               { $$ = $1; }
    | mul_expr '*' un_expr  { $$ = new swift::BinExpr('*', $1, $3, currentLine); }
    | mul_expr '/' un_expr  { $$ = new swift::BinExpr('/', $1, $3, currentLine); }
    ;

un_expr
    : postfix_expr          { $$ = $1; }
    | '^' un_expr           { $$ = new swift::UnExpr('^', $2, currentLine); }
    | '&' un_expr           { $$ = new swift::UnExpr('&', $2, currentLine); }
    | '-' un_expr           { $$ = new swift::UnExpr('-', $2, currentLine); }
    | '+' un_expr           { $$ = new swift::UnExpr('+', $2, currentLine); }
    ;

postfix_expr
    : primary_expr                  { $$ = $1; }
    | postfix_expr '.' ID
    | postfix_expr '(' ')'
    | postfix_expr '(' arg_list ')'
    ;

primary_expr
    : ID            { $$ = new swift::Id($1, getKeyLine()); }
    | L_INDEX       { $$ = $1; }
    | L_INT         { $$ = $1; }
    | L_INT8        { $$ = $1; }
    | L_INT16       { $$ = $1; }
    | L_INT32       { $$ = $1; }
    | L_INT64       { $$ = $1; }
    | L_SAT8        { $$ = $1; }
    | L_SAT16       { $$ = $1; }
    | L_UINT        { $$ = $1; }
    | L_UINT8       { $$ = $1; }
    | L_UINT16      { $$ = $1; }
    | L_UINT32      { $$ = $1; }
    | L_UINT64      { $$ = $1; }
    | L_USAT8       { $$ = $1; }
    | L_USAT16      { $$ = $1; }
    | L_REAL        { $$ = $1; }
    | L_REAL32      { $$ = $1; }
    | L_REAL64      { $$ = $1; }
    | '(' expr ')'  { $$ = $2; }
    ;

arg_list
    : assign_expr                   { $$ = new swift::Arg($1,  0, currentLine); }
    | assign_expr ',' arg_list      { $$ = new swift::Arg($1, $3, currentLine); }
    ;

opt_semis
    : /*empty*/
    | opt_semis ';'
    ;

method_qualifier
    : READER    { $$ = READER; }
    | WRITER    { $$ = WRITER; }
    | ROUTINE   { $$ = ROUTINE; }
    ;

parameter_qualifier
    : IN    { $$ = IN; }
    | INOUT { $$ = INOUT; }
    | OUT   { $$ = OUT; }
    ;

type
    : type_qualifier  base_type { pointerCount_ = 0; } pointer  { $$ = new swift::Type($1,  $2, pointerCount_, currentLine); }
    | /*default VAR*/ base_type { pointerCount_ = 0; } pointer  { $$ = new swift::Type(VAR, $1, pointerCount_, currentLine); }
    ;

base_type
    : simple_type                   { $$ = new swift::SimpleType($1, currentLine); }
    | ARRAY '{' type '}'            { $$ = new swift::Container(ARRAY, $3,  0, currentLine); }
    | SIMD  '{' type '}'            { $$ = new swift::Container(SIMD,  $3,  0, currentLine); }
    | ARRAY '{' type ',' expr '}'   { $$ = new swift::Container(ARRAY, $3, $5, currentLine); }
    | SIMD  '{' type ',' expr '}'   { $$ = new swift::Container(SIMD,  $3, $5, currentLine); }
    | ID                            { $$ = new swift::UserType($1/*, 0*/, currentLine); }
    | ID  '{' template_arg_list '}' { $$ = new swift::UserType($1/*, $3*/, currentLine); }
    ;

pointer
    : /**/
    | '^' pointer   { ++pointerCount_; }
    ;


template_arg_list
    : assign_expr
    | assign_expr ',' template_arg_list
    ;

type_qualifier
    : VAR   { $$ = VAR; }
    | CST   { $$ = CST; }
    | DEF   { $$ = DEF; }
    ;

/*type_modifier:
    '*'
    | '*' type_modifier
    ;*/

simple_type
    : INDEX     { $$ = INDEX; }
    | INT       { $$ = INT; }
    | INT8      { $$ = INT8; }
    | INT16     { $$ = INT16; }
    | INT32     { $$ = INT32; }
    | INT64     { $$ = INT64; }
    | SAT8      { $$ = SAT8; }
    | SAT16     { $$ = SAT16; }

    | UINT      { $$ = UINT; }
    | UINT8     { $$ = UINT8; }
    | UINT16    { $$ = UINT16; }
    | UINT32    { $$ = UINT32; }
    | UINT64    { $$ = UINT64; }
    | USAT8     { $$ = USAT8; }
    | USAT16    { $$ = USAT16; }

    | REAL      { $$ = REAL; }
    | REAL32    { $$ = REAL32; }
    | REAL64    { $$ = REAL64; }

    | CHAR      { $$ = CHAR; }
    | CHAR8     { $$ = CHAR8; }
    | CHAR16    { $$ = CHAR16; }

    | STRING    { $$ = STRING; }
    | STRING8   { $$ = STRING8; }
    | STRING16  { $$ = STRING16; }

    | BOOL      { $$ = BOOL; }
    ;

%%

/*
    FIXME better syntax error handling
*/

void yyerror(char *s)
{
    swift::errorf(currentLine, s);
    exit(0);
}

void parserInit(swift::SyntaxTree* syntaxTree)
{
    syntaxTree_  = syntaxTree;
}
