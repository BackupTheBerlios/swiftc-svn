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


/* 
    TODO remove right recursion in favour of left recursion 
*/

%define namespace "swift"
%define parser_class_name "Parser"

%{

#include <iostream>

#include "fe/auto.h"
#include "fe/class.h"
#include "fe/decl.h"
#include "fe/error.h"
#include "fe/access.h"
#include "fe/expr.h"
#include "fe/exprlist.h"
#include "fe/functioncall.h"
#include "fe/memberfunction.h"
#include "fe/simdprefix.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/syntaxtree.h"
#include "fe/tuple.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"

namespace swift {

int currentLine = 0;

std::string* operatorToString(int _operator)
{
    std::string* str = new std::string();

    switch (_operator)
    {
        case Token::AND_OP: *str = "and"; break;
        case Token::OR_OP:  *str = "or"; break;
        case Token::XOR_OP: *str = "xor"; break;
        case Token::NOT_OP: *str = "not"; break;
        case Token::EQ_OP:  *str = "eq"; break;
        case Token::NE_OP:  *str = "ne"; break;
        case Token::LE_OP:  *str = "le"; break;
        case Token::GE_OP:  *str = "ge"; break;
        case Token::MOD_OP: *str = "mod"; break;
        case Token::DIV_OP: *str = "div"; break;
        case '+': *str = "add"; break;
        case '-': *str = "sub"; break;
        case '*': *str = "mul"; break;
        case '/': *str = "div"; break;
        case '<': *str = "l"; break;
        case '>': *str = "g"; break;
        default:
            swiftAssert(false, "unreachable code");
    }

    return str;
}

} // namespace swift

void swifterror(const char *s);

using namespace swift;

%}

%union
{
    int              int_;
    bool             bool_;
    std::string*     id_;

    Class*           class_;
    ClassMember*     classMember_;
    Decl*            decl_;
    Definition*      definition_;
    Expr*            expr_;
    ExprList*        exprList_;
    MemberVar*       memberVar_;
    MemberFunction*  memberFunction_;
    Module*          module_;
    SimdPrefix*      simdPrefix_;
    Statement*       statement_;
    Tuple*           tuple_;
    Type*            type_;
};

/*
    tokens
*/

// literals
%token <expr_>  L_INDEX
%token <expr_>  L_INT  L_INT8   L_INT16  L_INT32  L_INT64  L_SAT8  L_SAT16
%token <expr_> L_UINT L_UINT8  L_UINT16 L_UINT32 L_UINT64 L_USAT8 L_USAT16
%token <expr_> L_REAL L_REAL32 L_REAL64
%token <expr_> L_TRUE L_FALSE

%token NIL

// types
%token PTR ARRAY SIMD

// type modifiers
%token VAR CONST INOUT REF CONST_REF

%token SELF

// method qualifier
%token READER WRITER ROUTINE ASSIGN OPERATOR

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
%token IF ELSE ELIF FOR WHILE REPEAT UNTIL
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
%type <bool_>        simd_modifier
%type <type_>       type bare_type
%type <decl_>       decl

%type <definition_> class_definition
%type <classMember_> class_body class_member
%type <memberFunction_> member_function
%type <memberVar_>  member_var

%type <expr_>     expr rel_expr mul_expr add_expr postfix_expr un_expr primary_expr
%type <exprList_> expr_list expr_list_not_empty
%type <tuple_>    tuple

%type <simdPrefix_> simd_prefix

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

simd_modifier
    : SIMD { $$ = true; }
    | /**/ { $$ = false; }
    ;

class_definition
    : simd_modifier CLASS ID EOL
        {
            $<class_>$ = new Class($1, $3, symtab->module_, currentLine);
            symtab->insert($<class_>$);
#ifdef SWIFT_DEBUG
            $<class_>$->meStruct_ = me::functab->newStruct(*$3);
#else // SWIFT_DEBUG
            $<class_>$->meStruct_ = me::functab->newStruct();
#endif // SWIFT_DEBUG
        }
        class_body END EOL
        {
            $$ = $<class_>5;
            $<class_>$->classMember_= $6;
        }
    ;

class_body
    : /*empty*/                 { $$ = 0; }
    | class_member class_body   { $$ = $1; $1->next_ = $2; }
    | EOL class_body            { $$ = $2; }
    ;

class_member
    : member_function { $$ = $1; }
    | member_var      { $$ = $1; }
    ;

/*
    member_functions
*/

method_qualifier
    : READER    { $$ = Token::READER; }
    | WRITER    { $$ = Token::WRITER; }
    | ROUTINE   { $$ = Token::ROUTINE; }
    ;

member_function
    : simd_modifier method_qualifier ID
        {
            switch ($2)
            {
                case Token::READER: 
                    $<memberFunction_>$ = new Reader($1, $3, symtab->class_, currentLine );
                    break;

                case Token::WRITER: 
                    $<memberFunction_>$ = new Writer($1, $3, symtab->class_, currentLine );
                    break;

                case Token::ROUTINE: 
                    $<memberFunction_>$ = new Routine($1, $3, symtab->class_, currentLine );
                    break;

                default:
                    swiftAssert(false, "unreachable code");
            }

            symtab->insert($<memberFunction_>$);
        }
        '(' parameter_list ')' arrow_return_type_list
        EOL statement_list END EOL
        {
            $$ = $<memberFunction_>4;
            $$->statements_ = $10;
        }
    | simd_modifier OPERATOR operator
        {
            $<memberFunction_>$ = new Operator( $1, operatorToString($3), symtab->class_, currentLine );
            symtab->insert($<memberFunction_>$);
        }
        '(' parameter_list ')' arrow_return_type_list
        EOL statement_list END EOL
        {
            $$ = $<memberFunction_>4;
            $$->statements_ = $10;
        }
    | simd_modifier ASSIGN 
        {
            $<memberFunction_>$ = new Assign($1, symtab->class_, currentLine );
            symtab->insert($<memberFunction_>$);
        }
        '(' parameter_list ')'
        EOL statement_list END EOL
        {
            $$ = $<memberFunction_>3;
            $$->statements_ = $8;
        }
    | simd_modifier CREATE
        {
            $<memberFunction_>$ = new Create($1, symtab->class_, currentLine );
            symtab->insert($<memberFunction_>$);
            symtab->class_->hasCreate_ = true;
        }
        '(' parameter_list')'
        EOL statement_list END EOL
        {
            $$ = $<memberFunction_>3;
            $$->statements_ = $8;
            symtab->class_->hasCreate_ = true;
        }
    ;

operator
    :   '+'     { $$ = '+'; }
    |   '-'     { $$ = '-'; }
    |   '*'     { $$ = '*'; }
    |   '/'     { $$ = '/'; }
    |   MOD_OP  { $$ = Token::MOD_OP; }
    |   DIV_OP  { $$ = Token::DIV_OP; }
    |   EQ_OP   { $$ = Token::EQ_OP;  }
    |   NE_OP   { $$ = Token::NE_OP;  }
    |   '<'     { $$ = '<'; }
    |   '>'     { $$ = '>'; }
    |   LE_OP   { $$ = Token::LE_OP;  }
    |   GE_OP   { $$ = Token::GE_OP;  }
    |   AND_OP  { $$ = Token::AND_OP; }
    |   OR_OP   { $$ = Token::OR_OP;  }
    |   XOR_OP  { $$ = Token::XOR_OP; }
    |   NOT_OP  { $$ = Token::NOT_OP; }
    |   '='     { $$ = '='; }
    ;

parameter_list
    : /* emtpy */
    | parameter
    | parameter_list ',' parameter
    ;

parameter
    :       bare_type ID { symtab->insertInParam( new InParam(false, $1, $2, currentLine) ); }
    | INOUT bare_type ID { symtab->insertInParam( new InParam( true, $2, $3, currentLine) ); }
    ;

arrow_return_type_list
    : /* empty */
    | ARROW return_type_list
    ;

return_type_list
    : bare_type ID                      { symtab->insertOutParam( new OutParam($1, $2, currentLine) ); }
    | return_type_list ',' bare_type ID { symtab->insertOutParam( new OutParam($3, $4, currentLine) ); }
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
    /*
        basic statements
    */
    : decl EOL                          { $$ = new DeclStatement($1, currentLine-1); }
    | expr EOL                          { $$ = new ExprStatement(0, $1, currentLine-1); }
    | tuple '=' expr_list_not_empty EOL { $$ = new AssignStatement(0, '=', $1, $3, currentLine-1); }

    /*
        simd statements
    */
    | simd_prefix expr EOL                          { $$ = new ExprStatement($1, $2, currentLine-1); }
    | simd_prefix tuple '=' expr_list_not_empty EOL { $$ = new AssignStatement($1, '=', $2, $4, currentLine-1); }

    /*
        control flow statements
    */
    | WHILE expr EOL statement_list END EOL { $$ = new WhileStatement($2, $4, currentLine-1); }
    | REPEAT EOL statement_list UNTIL expr EOL  { $$ = new RepeatUntilStatement($3, $5, currentLine-1); }

    | SCOPE EOL statement_list END EOL      { $$ = new ScopeStatement($3, currentLine-1); }

    | IF expr EOL statement_list END EOL                         { $$ = new IfElStatement($2, $4,  0, currentLine-1); }
    | IF expr EOL statement_list ELSE EOL statement_list END EOL { $$ = new IfElStatement($2, $4, $7, currentLine-1); }

    /* 
        jump statements
    */
    | RETURN    EOL { $$ = new CFStatement(Token::RETURN, currentLine);   }
    | BREAK     EOL { $$ = new CFStatement(Token::BREAK, currentLine);    }
    | CONTINUE  EOL { $$ = new CFStatement(Token::CONTINUE, currentLine); }
    ;

/*
    ***********
    simd prefix
    ***********
*/

simd_prefix
    : SIMD '[' expr ',' expr ']' ':' { $$ = new SimdPrefix($3, $5, currentLine); } 
    | SIMD '['      ',' expr ']' ':' { $$ = new SimdPrefix( 0, $4, currentLine); } 
    | SIMD '[' expr ','      ']' ':' { $$ = new SimdPrefix($3,  0, currentLine); } 
    | SIMD '['      ','      ']' ':' { $$ = new SimdPrefix( 0,  0, currentLine); } 
    | SIMD                       ':' { $$ = new SimdPrefix( 0,  0, currentLine); } 
    ;

/*
    ***********
    expressions
    ***********
*/

expr
    : rel_expr              { $$ = $1; }
    | expr EQ_OP add_expr   { $$ = new BinExpr(Token::EQ_OP, $1, $3, currentLine); }
    | expr NE_OP add_expr   { $$ = new BinExpr(Token::NE_OP, $1, $3, currentLine); }
    ;

rel_expr
    : add_expr              { $$ = $1; }
    | expr '<' add_expr     { $$ = new BinExpr('<', $1, $3, currentLine); }
    | expr '>' add_expr     { $$ = new BinExpr('>', $1, $3, currentLine); }
    | expr LE_OP add_expr   { $$ = new BinExpr(Token::LE_OP, $1, $3, currentLine); }
    | expr GE_OP add_expr   { $$ = new BinExpr(Token::GE_OP, $1, $3, currentLine); }
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
    | '^' un_expr           { $$ = new UnExpr(   '^', $2, currentLine); }
    | '&' un_expr           { $$ = new UnExpr(   '&', $2, currentLine); }
    | '-' un_expr           { $$ = new UnExpr(   '-', $2, currentLine); }
    | '+' un_expr           { $$ = new UnExpr(   '+', $2, currentLine); }
    | NOT_OP un_expr        { $$ = new UnExpr(Token::NOT_OP, $2, currentLine); }
    ;

postfix_expr
    : primary_expr                          { $$ = $1; }

    /* 
        accesses
    */
    | postfix_expr '.' ID                   { $$ = new MemberAccess($1, $3, currentLine); }
    | '.' ID                                { $$ = new MemberAccess( 0, $2, currentLine); }
    | postfix_expr '[' expr ']'             { $$ = new IndexExpr($1, $3, currentLine); }

    /* 
        c_call 
    */
    |  C_CALL type ID '(' expr_list ')'     { $$ = new CCall($2, Token:: C_CALL, $3, $5, currentLine); }
    | VC_CALL type ID '(' expr_list ')'     { $$ = new CCall($2, Token::VC_CALL, $3, $5, currentLine); }
    |  C_CALL ID '(' expr_list ')'          { $$ = new CCall( 0, Token:: C_CALL, $2, $4, currentLine); }
    | VC_CALL ID '(' expr_list ')'          { $$ = new CCall( 0, Token::VC_CALL, $2, $4, currentLine); }

    /* 
        routines 
    */
    /*|                 ID '(' expr_list ')'  { $$ = new RoutineCall((std::string*) 0, $1, $3, currentLine); }*/
    |    DOUBLE_COLON ID '(' expr_list ')'  { $$ = new RoutineCall((std::string*) 0, $2, $4, currentLine); }
    | ID DOUBLE_COLON ID '(' expr_list ')'  { $$ = new RoutineCall(              $1, $3, $5, currentLine); }

    /* 
        methods 
    */
    | postfix_expr ':' ID '(' expr_list ')' { $$ = new ReaderCall(       $1, $3, $5, currentLine); }
    | postfix_expr '.' ID '(' expr_list ')' { $$ = new WriterCall(       $1, $3, $5, currentLine); }
    | ':' ID '(' expr_list ')'              { $$ = new ReaderCall((Expr*) 0, $2, $4, currentLine); }
    | '.' ID '(' expr_list ')'              { $$ = new WriterCall((Expr*) 0, $2, $4, currentLine); }
    ;

primary_expr
    : ID               { $$ = new  Id($1, currentLine); }
    | NIL '{' type '}' { $$ = new Nil($3, currentLine); }
    | SELF             { $$ = new Self(currentLine); }
    | '@'              { $$ = new SimdIndex(currentLine); }
    | L_INDEX          { $$ = $1; }
    | L_INT            { $$ = $1; }
    | L_INT8           { $$ = $1; }
    | L_INT16          { $$ = $1; }
    | L_INT32          { $$ = $1; }
    | L_INT64          { $$ = $1; }
    | L_SAT8           { $$ = $1; }
    | L_SAT16          { $$ = $1; }
    | L_UINT           { $$ = $1; }
    | L_UINT8          { $$ = $1; }
    | L_UINT16         { $$ = $1; }
    | L_UINT32         { $$ = $1; }
    | L_UINT64         { $$ = $1; }
    | L_USAT8          { $$ = $1; }
    | L_USAT16         { $$ = $1; }
    | L_REAL           { $$ = $1; }
    | L_REAL32         { $$ = $1; }
    | L_REAL64         { $$ = $1; }
    | L_TRUE           { $$ = $1; }
    | L_FALSE          { $$ = $1; }
    | '(' expr ')'     { $$ = $2; }
    ;

expr_list
    : /* empty */           { $$ =  0; }
    | expr_list_not_empty   { $$ = $1; }
    ;

expr_list_not_empty
    :       expr                         { $$ = new ExprList(    0, $1,  0, currentLine); }
    | INOUT expr                         { $$ = new ExprList(Token::INOUT, $2,  0, currentLine); }
    |       expr ',' expr_list_not_empty { $$ = new ExprList(    0, $1, $3, currentLine); }
    | INOUT expr ',' expr_list_not_empty { $$ = new ExprList(Token::INOUT, $2, $4, currentLine); }
    ;

decl
    : type ID { $$ = new Decl($1, $2, currentLine); }
    ;

tuple
    : expr           { $$ = new Tuple($1,  0, currentLine); }
    | decl           { $$ = new Tuple($1,  0, currentLine); }
    | expr ',' tuple { $$ = new Tuple($1, $3, currentLine); }
    | decl ',' tuple { $$ = new Tuple($1, $3, currentLine); }
    ;

bare_type
    : ID                 { $$ = new BaseType(0, $1, currentLine); }
    | PTR   '{' type '}' { $$ = new      Ptr(0, $3, currentLine); }
    | ARRAY '{' type '}' { $$ = new    Array(0, $3, currentLine); }
    | SIMD  '{' type '}' { $$ = new     Simd(0, $3, currentLine); }
    ;

type
    : ID                       { $$ = new BaseType(Token::  VAR, $1, currentLine); }
    | CONST ID                 { $$ = new BaseType(Token::CONST, $2, currentLine); }
    | PTR         '{' type '}' { $$ = new      Ptr(Token::  VAR, $3, currentLine); }
    | CONST PTR   '{' type '}' { $$ = new      Ptr(Token::CONST, $4, currentLine); }
    | ARRAY       '{' type '}' { $$ = new    Array(Token::  VAR, $3, currentLine); }
    | CONST ARRAY '{' type '}' { $$ = new    Array(Token::CONST, $4, currentLine); }
    | SIMD        '{' type '}' { $$ = new     Simd(Token::  VAR, $3, currentLine); }
    | CONST SIMD  '{' type '}' { $$ = new     Simd(Token::CONST, $4, currentLine); }
    ;

%%

/*
    FIXME better syntax error handling
*/

void Parser::error(const location_type& loc, const std::string& str)
{
    errorf(0, str.c_str());
}
