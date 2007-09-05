%{

#include <iostream>

#include "fe/class.h"
#include "fe/error.h"
#include "fe/expr.h"
#include "fe/lexer.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/syntaxtree.h"
#include "fe/type.h"


int pointercount = -1;
bool parseerror = false;

%}

%union
{
    int                 int_;

    std::string*        id_;

    Module*      module_;
    Definition*  definition_;

    Class*       class_;
    ClassMember* classMember_;
    MemberVar*   memberVar_;
    Method*      method_;
    Parameter*   parameter_;

    Type*        type_;
    BaseType*    baseType_;

    Expr*        expr_;
    Arg*         arg_;

    Statement*   statement_;
    InitList*    initList_;
};

/*
    tokens
*/

// literals
%token <expr_>  L_INDEX
%token <expr_>  L_INT  L_INT8   L_INT16  L_INT32  L_INT64  L_SAT8  L_SAT16
%token <expr_> L_UINT L_UINT8  L_UINT16 L_UINT32 L_UINT64 L_USAT8 L_USAT16
%token <expr_> L_REAL L_REAL32 L_REAL64
%token <expr_> L_TRUE L_FALSE  L_NIL

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

// control flow
%token IF ELSE ELIF FOR WHILE DO_WHILE
%token RETURN RESULT BREAK CONTINUE

// protection
%token PUBLIC PROTECTED PACKAGE PRIVATE

// miscellaneous
%token SCOPE CLASS END EOL

%token <integer_>   INTEGER
%token <float_>     FLOAT
%token <id_>        ID

/*
    types
*/

%type <int_>        simple_type method_qualifier
%type <type_>       type
%type <parameter_>  parameter parameter_list return_type_list return_type /*return_values*/

%type <initList_>   init_list init_list_item

%type <definition_> class_definition definitions
%type <classMember_> class_body class_member
%type <method_>     method
%type <memberVar_>  member_var

%type <module_>     module
%type <expr_>       expr mul_expr add_expr postfix_expr un_expr primary_expr
%type <arg_>        arg_list

%type <statement_>  statement_list statement
%type <baseType_>   base_type

%start file

%%

file
    : module
    | EOL module
    ;

module
    :   {
            $$ = new Module(new std::string("default"), currentLine);
            symtab->insert($$);
        }
        definitions
        {
            $$ = $<module_>1;
            $$->definitions_ = $2;
            $$->parent_ = 0;
            syntaxtree->rootModule_ = $$;
        }
    ;

definitions
    : /*empty*/                     { $$ = 0; }
    | definitions class_definition  { $$ = $2; $$->next_ = $1; }
    ;

/*
    *******
    classes
    *******
*/

class_definition
    : CLASS ID EOL
        {
            $$ = new Class($2, currentLine);
            symtab->insert($<class_>$);
        }
        class_body END EOL
        {
            $$ = $<class_>4;
            $<class_>$->classMember_= $5;
        }
    | CLASS ID EOL
        {
            $$ = new Class($2, currentLine);
            symtab->insert($<class_>$);
        }
        '{' template_list '}' class_body END EOL
        {
            $$ = $<class_>4;
            $<class_>$->classMember_= $8;
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
    | EOL class_body            { $$ = $2; }
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
            $$ = new Method( $1, 0, $2, getKeyLine() );
            symtab->insert($$);
        }
        '(' parameter_list')'
        EOL statement_list END EOL
        {
            $$ = $<method_>3;
            $$->statements_ = $8;
        }
    | method_qualifier return_type_list '=' ID
        {
            $$ = new Method( $1, $2, $4, getKeyLine() );
            symtab->insert($$);
            $$->insertReturnTypesInSymtab();
        }
        '(' parameter_list')'
        EOL statement_list END EOL
        {
            $$ = $<method_>5;
            $$->statements_ = $10;
        }
    | CREATE
        {
            $$ = new Method( CREATE, 0, new std::string("create"), getKeyLine() );
            symtab->insert($$);
        }
        '(' parameter_list')'
        EOL statement_list END EOL
        {
            $$ = $<method_>2;
            $$->statements_ = $7;
        }
    ;

return_type_list
    : return_type                       { $$ = $1; $$->next_ = 0; }
    | return_type ',' return_type_list  { $$ = $1; $$->next_ = $3; }
    ;

return_type
    : type ID
        {
            $$ = new Parameter(Parameter::ARG, $1, $2, currentLine);
        }
    | INOUT type ID
        {
            $$ = new Parameter(Parameter::RES_INOUT, $2, $3, currentLine);
        }
    ;

parameter_list
    : /*emtpy*/                     { $$ = 0; }
    | parameter                     { $$ = $1; $$->next_ = 0; }
    | parameter ',' parameter_list  { $$ = $1; $$->next_ = $3; }
    ;

parameter
    : type ID
        {
            $$ = new Parameter(Parameter::ARG, $1, $2, currentLine);
            symtab->insert($$);
        }
    ;

/*
    ***********
    member vars
    ***********
*/

member_var
    : type ID EOL
        {
            $$ = new MemberVar($1, $2, currentLine);
            symtab->insert($$);
        }
    ;

/*
    **********
    statements
    **********
*/

init_list
    : init_list_item                { $$ = $1; $$->next_ =  0; }
    | init_list_item ',' init_list  { $$ = $1; $$->next_ = $3; }
    ;

init_list_item
    : expr              { $$ = new InitList( 0, $1, currentLine ); }
    | '{' init_list '}' { $$ = new InitList($2,  0, currentLine ); }
    | '{' '}'           { $$ = new InitList( 0,  0, currentLine ); }
    ;

statement_list
    : /*emtpy*/                 { $$ = 0; }
    | statement statement_list  { $$ = $1; $$->next_ = $2; }
    ;

statement
    : expr EOL                  { $$ = new ExprStatement($1, currentLine); }
    | expr '=' init_list EOL    { $$ = new AssignStatement('=', $1, $3, currentLine); }

    | type ID EOL               { $$ = new Declaration($1, $2,  0, getKeyLine()); }
    | type ID '=' init_list EOL { $$ = new Declaration($1, $2, $4, getKeyLine()); }

    | IF expr EOL statement_list END EOL                         { $$ = new IfElStatement(0,    $2, $4,  0, currentLine); }
    | IF expr EOL statement_list ELSE EOL statement_list END EOL { $$ = new IfElStatement(ELSE, $2, $4, $7, currentLine); }
/*     | IF rel_expr EOL statement_list ELIF '(' rel_expr ')' statement_list END EOL { $$ = new IfElStatement(ELIF, $3, $5, $10, currentLine); } */
    ;

/*
    ***********
    expressions
    ***********
*/

expr
    : add_expr              { $$ = $1; }
    | expr '<' add_expr     { $$ = new BinExpr('<', $1, $3, currentLine); }
    | expr '>' add_expr     { $$ = new BinExpr('>', $1, $3, currentLine); }
    | expr LE_OP add_expr   { $$ = new BinExpr(LE_OP, $1, $3, currentLine); }
    | expr GE_OP add_expr   { $$ = new BinExpr(GE_OP, $1, $3, currentLine); }
    | expr EQ_OP add_expr   { $$ = new BinExpr(EQ_OP, $1, $3, currentLine); }
    | expr NE_OP add_expr   { $$ = new BinExpr(NE_OP, $1, $3, currentLine); }
    ;

add_expr
    : mul_expr              { $$ = $1; }
    | add_expr '+' mul_expr { $$ = new BinExpr('+', $1, $3, currentLine); }
    | add_expr '-' mul_expr { $$ = new BinExpr('-', $1, $3, currentLine); }
    ;

mul_expr:
      un_expr               { $$ = $1; }
    | mul_expr '*' un_expr  { $$ = new BinExpr('*', $1, $3, currentLine); }
    | mul_expr '/' un_expr  { $$ = new BinExpr('/', $1, $3, currentLine); }
    ;

un_expr
    : postfix_expr          { $$ = $1; }
    | '^' un_expr           { $$ = new UnExpr('^', $2, currentLine); }
    | '&' un_expr           { $$ = new UnExpr('&', $2, currentLine); }
    | '-' un_expr           { $$ = new UnExpr('-', $2, currentLine); }
    | '+' un_expr           { $$ = new UnExpr('+', $2, currentLine); }
    | '!' un_expr           { $$ = new UnExpr('!', $2, currentLine); }
    ;

postfix_expr
    : primary_expr                  { $$ = $1; }
    | postfix_expr '.' ID
    | postfix_expr '(' ')'
    | postfix_expr '(' arg_list ')'
    ;

primary_expr
    : ID            { $$ = new Id($1, getKeyLine()); }
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
    | L_TRUE        { $$ = $1; }
    | L_FALSE       { $$ = $1; }
    | L_NIL         { $$ = $1; }
    | '(' expr ')'  { $$ = $2; }
    ;

arg_list
    : expr                   { $$ = new Arg($1,  0, currentLine); }
    | expr ',' arg_list      { $$ = new Arg($1, $3, currentLine); }
    ;

method_qualifier
    : READER    { $$ = READER; }
    | WRITER    { $$ = WRITER; }
    | ROUTINE   { $$ = ROUTINE; }
    ;

type
    : base_type { pointercount = 0; } pointer  { $$ = new Type($1, pointercount, currentLine); }
    ;

base_type
    : simple_type                   { $$ = new SimpleType($1, currentLine); }
    | ARRAY '{' type '}'            { $$ = new Container(ARRAY, $3,  0, currentLine); }
    | SIMD  '{' type '}'            { $$ = new Container(SIMD,  $3,  0, currentLine); }
    | ARRAY '{' type ',' expr '}'   { $$ = new Container(ARRAY, $3, $5, currentLine); }
    | SIMD  '{' type ',' expr '}'   { $$ = new Container(SIMD,  $3, $5, currentLine); }
    | ID                            { $$ = new UserType($1/*, 0*/, currentLine); }
    | ID  '{' template_arg_list '}' { $$ = new UserType($1/*, $3*/, currentLine); }
    ;

pointer
    : /**/
    | '^' pointer   { ++pointercount; }
    ;


template_arg_list
    : expr
    | expr ',' template_arg_list
    ;

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
    errorf(currentLine, s);
    parseerror = true;
}
