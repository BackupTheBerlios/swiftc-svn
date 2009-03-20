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

#include <iostream>

#include "fe/class.h"
#include "fe/decl.h"
#include "fe/error.h"
#include "fe/expr.h"
#include "fe/exprlist.h"
#include "fe/functioncall.h"
#include "fe/lexer.h"
#include "fe/method.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/syntaxtree.h"
#include "fe/tupel.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"

namespace swift {

int pointercount = -1;

std::string* operatorToString(int _operator)
{
    std::string* str = new std::string();

    switch (_operator)
    {
        case AND_OP:
            *str = "and";
            break;
        case OR_OP:
            *str = "or";
            break;
        case XOR_OP:
            *str = "xor";
            break;
        case NOT_OP:
            *str = "not";
            break;
        case EQ_OP:
            *str = "==";
            break;
        case NE_OP:
            *str = "<>";
            break;
        case LE_OP:
            *str = "<=";
            break;
        case GE_OP:
            *str = ">=";
            break;
        case MOD_OP:
            *str = "mod";
            break;
        case DIV_OP:
            *str = "div";
            break;
        default:
            *str = (char) _operator;
    }

    return str;
}

} // namespace swift

void swifterror(const char *s);

using namespace swift;

%}

%union
{
    int                 int_;
    std::string*        id_;

    swift::Class*       class_;
    swift::ClassMember* classMember_;
    swift::Decl*        decl_;
    swift::Definition*  definition_;
    swift::Expr*        expr_;
    swift::ExprList*    exprList_;
    swift::MemberVar*   memberVar_;
    swift::Method*      method_;
    swift::Module*      module_;
    swift::Statement*   statement_;
    swift::Tupel*       tupel_;
    swift::Type*        type_;
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
%token PTR ARRAY SIMD

// type modifiers
%token VAR CONST

// parameter qualifier
%token INOUT

%token SELF

// method qualifier
%token READER WRITER ROUTINE OPERATOR

%token ARROW
%token DOUBLE_COLON

// constructor / destructor
%token CREATE DESTROY

// built-in functions
%token SHL SHR ROL ROR

// operators
%token INC_OP DEC_OP
%token EQ_OP NE_OP
%token LE_OP GE_OP
%token MOVE_OP SWAP_OP
%token AND_OP OR_OP XOR_OP NOT_OP
%token MOD_OP DIV_OP

%token C_CALL VC_CALL

%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN AND_ASSIGN OR_ASSIGN XOR_ASSIGN

// control flow
%token IF ELSE ELIF FOR WHILE DO_WHILE
%token RETURN RESULT BREAK CONTINUE

// protection
%token PUBLIC PROTECTED PACKAGE PRIVATE

// miscellaneous
%token SCOPE CLASS END EOL

%token <id_> ID

/*
    types
*/

%type <int_>        method_qualifier operator
%type <type_>       type
%type <decl_>       decl

%type <definition_> class_definition
%type <classMember_> class_body class_member
%type <method_>     method
%type <memberVar_>  member_var

%type <expr_>     expr rel_expr mul_expr add_expr postfix_expr un_expr primary_expr
%type <exprList_> expr_list expr_list_not_empty
%type <tupel_>    tupel

%type <statement_>  statement_list statement

%start file

%%

file
    : module
    | EOL module
    ;

module
    : definitions
    ;

definitions
    : /*empty*/
    | class_definition definitions  { syntaxtree->rootModule_->definitions_.append($1); }
    ;

/*
    *******
    classes
    *******
*/

class_definition
    : CLASS ID EOL
        {
            $<class_>$ = new Class($2, symtab->module_, currentLine);
            symtab->insert($<class_>$);
#ifdef SWIFT_DEBUG
            $<class_>$->meStruct_ = me::functab->newStruct(*$2);
#else // SWIFT_DEBUG
            $<class_>$->meStruct_ = me::functab->newStruct();
#endif // SWIFT_DEBUG
        }
        class_body END EOL
        {
            $$ = $<class_>4;
            $<class_>$->classMember_= $5;
            if ( !$<class_>$->hasCreate_ )
                $<class_>$->createDefaultConstructor();
        }
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

method_qualifier
    : READER    { $$ = READER; }
    | WRITER    { $$ = WRITER; }
    | ROUTINE   { $$ = ROUTINE; }
    ;

method
    : method_qualifier ID
        {
            $<method_>$ = new Method( $1, $2, symtab->class_, getKeyLine() );
            symtab->insert($<method_>$);
        }
        '(' parameter_list ')' arrow_return_type_list
        EOL statement_list END EOL
        {
            $$ = $<method_>3;
            $$->statements_ = $9;
        }
    | OPERATOR operator
        {
            $<method_>$ = new Method( OPERATOR, operatorToString($2), symtab->class_, getKeyLine() );
            symtab->insert($<method_>$);
        }
        '(' parameter_list ')' arrow_return_type_list
        EOL statement_list END EOL
        {
            $$ = $<method_>3;
            $$->statements_ = $9;
        }
    | CREATE
        {
            $<method_>$ = new Method( CREATE, new std::string("create"), symtab->class_, getKeyLine() );
            symtab->insert($<method_>$);
            symtab->class_->hasCreate_ = true;
        }
        '(' parameter_list')'
        EOL statement_list END EOL
        {
            $$ = $<method_>2;
            $$->statements_ = $7;
            symtab->class_->hasCreate_ = true;
        }
    ;

operator
    :   '+'     { $$ = '+'; }
    |   '-'     { $$ = '-'; }
    |   '*'     { $$ = '*'; }
    |   '/'     { $$ = '/'; }
    |   MOD_OP  { $$ = MOD_OP; }
    |   DIV_OP  { $$ = DIV_OP; }
    |   EQ_OP   { $$ = EQ_OP; }
    |   NE_OP   { $$ = NE_OP; }
    |   '<'     { $$ = '<'; }
    |   '>'     { $$ = '>'; }
    |   LE_OP   { $$ = LE_OP; }
    |   GE_OP   { $$ = GE_OP; }
    |   AND_OP  { $$ = AND_OP; }
    |   OR_OP   { $$ = OR_OP; }
    |   XOR_OP  { $$ = XOR_OP; }
    |   NOT_OP  { $$ = NOT_OP; }
    ;

parameter_list
    : /* emtpy */
    | parameter
    | parameter ',' parameter_list
    ;

parameter
    :       type ID { symtab->insertParam( new Param(Param::ARG,       $1, $2, currentLine) ); }
    | INOUT type ID { symtab->insertParam( new Param(Param::ARG_INOUT, $2, $3, currentLine) ); }
    ;

arrow_return_type_list
    : /* empty */
    | ARROW return_type_list
    ;

return_type_list
    : return_type
    | return_type ',' return_type_list
    ;

return_type
    : type ID       { symtab->insertRes( new Param(Param::RES, $1, $2, currentLine) ); }
    ;

/*
    ***********
    member vars
    ***********
*/

member_var
    : type ID EOL
        {
            $$ = new MemberVar($1, $2, symtab->class_, currentLine);
            symtab->insert($$);
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
    : expr EOL                          { $$ = new ExprStatement($1, currentLine-1); }
    /* TODO | tupel EOL                         { $$ = new DeclStatement($1, getKeyLine()); }*/
    | decl EOL                          { $$ = new DeclStatement($1, currentLine-1); }
    | tupel '=' expr_list_not_empty EOL { $$ = new AssignStatement('=', $1, $3, currentLine-1) }

    | WHILE expr EOL statement_list END EOL { $$ = new WhileStatement($2, $4, currentLine-1); }

    | IF expr EOL statement_list END EOL                         { $$ = new IfElStatement($2, $4,  0, currentLine-1); }
    | IF expr EOL statement_list ELSE EOL statement_list END EOL { $$ = new IfElStatement($2, $4, $7, currentLine-1); }
    
    | RETURN    EOL { $$ = new CFStatement(RETURN, currentLine);   }
    | BREAK     EOL { $$ = new CFStatement(BREAK, currentLine);    }
    | CONTINUE  EOL { $$ = new CFStatement(CONTINUE, currentLine); }

    ;

/*
    ***********
    expressions
    ***********
*/

expr
    : rel_expr              { $$ = $1; }
    | expr EQ_OP add_expr   { $$ = new BinExpr(EQ_OP, $1, $3, currentLine); }
    | expr NE_OP add_expr   { $$ = new BinExpr(NE_OP, $1, $3, currentLine); }
    ;

rel_expr
    : add_expr              { $$ = $1; }
    | expr '<' add_expr     { $$ = new BinExpr('<', $1, $3, currentLine); }
    | expr '>' add_expr     { $$ = new BinExpr('>', $1, $3, currentLine); }
    | expr LE_OP add_expr   { $$ = new BinExpr(LE_OP, $1, $3, currentLine); }
    | expr GE_OP add_expr   { $$ = new BinExpr(GE_OP, $1, $3, currentLine); }
    ;

add_expr
    : mul_expr              { $$ = $1; }
    | add_expr '+' mul_expr { $$ = new BinExpr('+', $1, $3, currentLine); }
    | add_expr '-' mul_expr { $$ = new BinExpr('-', $1, $3, currentLine); }
    ;

mul_expr
    : un_expr               { $$ = $1; }
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
    : primary_expr                          { $$ = $1; }

    /* 
        member access 
    */
    | postfix_expr '.' ID                   { $$ = new MemberAccess($1, $3, currentLine); }
    | '.' ID                                { $$ = new MemberAccess( 0, $2, currentLine); }

    /* 
        c_call 
    */
    |  C_CALL type ID '(' expr_list ')'     { $$ = new CCall(       $2, $3, $5,  C_CALL, currentLine); }
    | VC_CALL type ID '(' expr_list ')'     { $$ = new CCall(       $2, $3, $5, VC_CALL, currentLine); }
    |  C_CALL ID '(' expr_list ')'          { $$ = new CCall((Type*) 0, $2, $4,  C_CALL, currentLine); }
    | VC_CALL ID '(' expr_list ')'          { $$ = new CCall((Type*) 0, $2, $4, VC_CALL, currentLine); }

    /* 
        routines 
    */
  /*|                 ID '(' expr_list ')'  { $$ = new RoutineCall((std::string*) 0, $1, $3,       0, currentLine); }*/
    |    DOUBLE_COLON ID '(' expr_list ')'  { $$ = new RoutineCall((std::string*) 0, $2, $4, ROUTINE, currentLine); }
    | ID DOUBLE_COLON ID '(' expr_list ')'  { $$ = new RoutineCall(              $1, $3, $5, ROUTINE, currentLine); }

    /* 
        methods 
    */
    | postfix_expr '.' ID '(' expr_list ')' { $$ = new MethodCall(       $1, $3, $5, READER, currentLine); }
    | postfix_expr ':' ID '(' expr_list ')' { $$ = new MethodCall(       $1, $3, $5, WRITER, currentLine); }
    | '.' ID '(' expr_list ')'              { $$ = new MethodCall((Expr*) 0, $2, $4, READER, currentLine); }
    | ':' ID '(' expr_list ')'              { $$ = new MethodCall((Expr*) 0, $2, $4, WRITER, currentLine); }
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

expr_list
    : /* empty */           { $$ =  0; }
    | expr_list_not_empty   { $$ = $1; }
    ;

expr_list_not_empty
    :       expr                         { $$ = new ExprList(    0, $1,  0, currentLine); }
    | INOUT expr                         { $$ = new ExprList(INOUT, $2,  0, currentLine); }
    |       expr ',' expr_list_not_empty { $$ = new ExprList(    0, $1, $3, currentLine); }
    | INOUT expr ',' expr_list_not_empty { $$ = new ExprList(INOUT, $2, $4, currentLine); }
    ;

decl
    : type ID { $$ = new Decl($1, $2, currentLine); }
    ;

tupel
    : expr           { $$ = new Tupel($1,  0, currentLine); }
    | decl           { $$ = new Tupel($1,  0, currentLine); }
    | expr ',' tupel { $$ = new Tupel($1, $3, currentLine); }
    | decl ',' tupel { $$ = new Tupel($1, $3, currentLine); }
    ;

type
    : ID                       { $$ = new BaseType(    0, $1, currentLine); }
    | CONST ID                 { $$ = new BaseType(CONST, $2, currentLine); }
    | PTR         '{' type '}' { $$ = new Ptr(    0, $3, currentLine); }
    | CONST PTR   '{' type '}' { $$ = new Ptr(CONST, $4, currentLine); }
    /*| ARRAY '{' type '}' { $$ = new Array($1, $4, currentLine); }*/
    /*| CONST ARRAY '{' type '}' { $$ = new Array($1, $4, currentLine); }*/
    /*| SIMD  '{' type '}' { $$ = new Simd($1, $4, currentLine); }*/
    /*| CONST SIMD  '{' type '}' { $$ = new Simd($1, $4, currentLine); }*/
    ;

%%

/*
    FIXME better syntax error handling
*/

void swifterror(const char *s)
{
    errorf(currentLine, s);
}
