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

std::string* operatorToString(int _operator)
{
    std::string* str = new std::string();

    switch (_operator)
    {
        case Token::ADD: *str = "add"; break;
        case Token::SUB: *str = "sub"; break;
        case Token::MUL: *str = "mul"; break;
        case Token::DIV: *str = "div"; break;
        case Token::MOD: *str = "mod"; break;

        case Token::AND: *str = "and"; break;
        case Token::OR:  *str = "or"; break;
        case Token::XOR: *str = "xor"; break;
        case Token::NOT: *str = "not"; break;

        case Token::EQ:  *str = "eq"; break;
        case Token::NE:  *str = "ne"; break;
        case Token::LE:  *str = "le"; break;
        case Token::LT:  *str = "lt"; break;
        case Token::GE:  *str = "ge"; break;
        case Token::GT:  *str = "gt"; break;
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

// arithmetic operators
%token ADD SUB MUL DIV MOD
// comparision operators
%token EQ NE LT LE GT GE
// bitwise operators
%token AND OR XOR NOT

%token INC DEC
%token MOVE SWAP

%token C_CALL VC_CALL

%token ASSIGN_ADD ASSIGN_SUB ASSIGN_MUL ASSIGN_DIV ASSIGN_MOD ASSIGN_AND ASSIGN_OR ASSIGN_XOR

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
            $<class_>$ = new Class($1, $3, symtab->module_, @$);
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
                    $<memberFunction_>$ = new Reader($1, $3, symtab->class_, @$ );
                    break;

                case Token::WRITER: 
                    $<memberFunction_>$ = new Writer($1, $3, symtab->class_, @$ );
                    break;

                case Token::ROUTINE: 
                    $<memberFunction_>$ = new Routine($1, $3, symtab->class_, @$ );
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
            $<memberFunction_>$ = new Operator( $1, operatorToString($3), symtab->class_, @$ );
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
            $<memberFunction_>$ = new Assign($1, symtab->class_, @$ );
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
            $<memberFunction_>$ = new Create($1, symtab->class_, @$ );
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
    :   ADD  { $$ = Token::ADD; }
    |   SUB  { $$ = Token::SUB; }
    |   MUL  { $$ = Token::MUL; }
    |   DIV  { $$ = Token::DIV; }
    |   MOD  { $$ = Token::MOD; }
    |   EQ   { $$ = Token::EQ;  }
    |   NE   { $$ = Token::NE;  }
    |   LT   { $$ = Token::LT;  }
    |   LE   { $$ = Token::LE;  }
    |   GT   { $$ = Token::GT;  }
    |   GE   { $$ = Token::GE;  }
    |   AND  { $$ = Token::AND; }
    |   OR   { $$ = Token::OR;  }
    |   XOR  { $$ = Token::XOR; }
    |   NOT  { $$ = Token::NOT; }
    |   '='     { $$ = '='; }
    ;

parameter_list
    : /* emtpy */
    | parameter
    | parameter_list ',' parameter
    ;

parameter
    :       bare_type ID { symtab->insertInParam( new InParam(false, $1, $2, @$) ); }
    | INOUT bare_type ID { symtab->insertInParam( new InParam( true, $2, $3, @$) ); }
    ;

arrow_return_type_list
    : /* empty */
    | ARROW return_type_list
    ;

return_type_list
    : bare_type ID                      { symtab->insertOutParam( new OutParam($1, $2, @$) ); }
    | return_type_list ',' bare_type ID { symtab->insertOutParam( new OutParam($3, $4, @$) ); }
    ;

/*
    ***********
    member vars
    ***********
*/

member_var
    : type ID EOL
        {
            $$ = new MemberVar($1, $2, symtab->class_, @$);
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
    : decl EOL                          { $$ = new DeclStatement($1, @$); }
    | expr EOL                          { $$ = new ExprStatement(0, $1, @$); }
    | tuple '=' expr_list_not_empty EOL { $$ = new AssignStatement(0, '=', $1, $3, @$); }

    /*
        simd statements
    */
    | simd_prefix expr EOL                          { $$ = new ExprStatement($1, $2, @$); }
    | simd_prefix tuple '=' expr_list_not_empty EOL { $$ = new AssignStatement($1, '=', $2, $4, @$); }

    /*
        control flow statements
    */
    | WHILE expr EOL statement_list END EOL { $$ = new WhileStatement($2, $4, @$); }
    | REPEAT EOL statement_list UNTIL expr EOL  { $$ = new RepeatUntilStatement($3, $5, @$); }

    | SCOPE EOL statement_list END EOL      { $$ = new ScopeStatement($3, @$); }

    | IF expr EOL statement_list END EOL                         { $$ = new IfElStatement($2, $4, 0, @$); }
    | IF expr EOL statement_list ELSE EOL statement_list END EOL { $$ = new IfElStatement($2, $4, $7, @$); }

    /* 
        jump statements
    */
    | RETURN    EOL { $$ = new CFStatement(Token::RETURN, @$);   }
    | BREAK     EOL { $$ = new CFStatement(Token::BREAK, @$);    }
    | CONTINUE  EOL { $$ = new CFStatement(Token::CONTINUE, @$); }
    ;

/*
    ***********
    simd prefix
    ***********
*/

simd_prefix
    : SIMD '[' expr ',' expr ']' ':' { $$ = new SimdPrefix($3, $5, @$); } 
    | SIMD '['      ',' expr ']' ':' { $$ = new SimdPrefix( 0, $4, @$); } 
    | SIMD '[' expr ','      ']' ':' { $$ = new SimdPrefix($3,  0, @$); } 
    | SIMD '['      ','      ']' ':' { $$ = new SimdPrefix( 0,  0, @$); } 
    | SIMD                       ':' { $$ = new SimdPrefix( 0,  0, @$); } 
    ;

/*
    ***********
    expressions
    ***********
*/

expr
    : rel_expr              { $$ = $1; }
    | expr EQ add_expr   { $$ = new BinExpr(Token::EQ, $1, $3, @$); }
    | expr NE add_expr   { $$ = new BinExpr(Token::NE, $1, $3, @$); }
    ;

rel_expr
    : add_expr              { $$ = $1; }
    | expr LT add_expr   { $$ = new BinExpr(Token::LT, $1, $3, @$); }
    | expr LE add_expr   { $$ = new BinExpr(Token::LE, $1, $3, @$); }
    | expr GT add_expr   { $$ = new BinExpr(Token::GT, $1, $3, @$); }
    | expr GE add_expr   { $$ = new BinExpr(Token::GE, $1, $3, @$); }
    ;

add_expr
    : mul_expr                 { $$ = $1; }
    | add_expr ADD mul_expr { $$ = new BinExpr(Token::ADD, $1, $3, @$); }
    | add_expr SUB mul_expr { $$ = new BinExpr(Token::SUB, $1, $3, @$); }
    ;

mul_expr
    : un_expr               { $$ = $1; }
    | mul_expr MUL un_expr  { $$ = new BinExpr(Token::MUL, $1, $3, @$); }
    | mul_expr DIV un_expr  { $$ = new BinExpr(Token::DIV, $1, $3, @$); }
    ;

un_expr
    : postfix_expr          { $$ = $1; }
    | '^' un_expr           { $$ = new UnExpr(   '^', $2, @$); }
    | '&' un_expr           { $$ = new UnExpr(   '&', $2, @$); }
    | SUB un_expr           { $$ = new UnExpr(   '-', $2, @$); }
    | ADD un_expr           { $$ = new UnExpr(   '+', $2, @$); }
    | NOT un_expr        { $$ = new UnExpr(Token::NOT, $2, @$); }
    ;

postfix_expr
    : primary_expr                          { $$ = $1; }

    /* 
        accesses
    */
    | postfix_expr '.' ID                   { $$ = new MemberAccess($1, $3, @$); }
    | '.' ID                                { $$ = new MemberAccess( 0, $2, @$); }
    | postfix_expr '[' expr ']'             { $$ = new IndexExpr($1, $3, @$); }

    /* 
        c_call 
    */
    |  C_CALL type ID '(' expr_list ')'     { $$ = new CCall($2, Token:: C_CALL, $3, $5, @$); }
    | VC_CALL type ID '(' expr_list ')'     { $$ = new CCall($2, Token::VC_CALL, $3, $5, @$); }
    |  C_CALL ID '(' expr_list ')'          { $$ = new CCall( 0, Token:: C_CALL, $2, $4, @$); }
    | VC_CALL ID '(' expr_list ')'          { $$ = new CCall( 0, Token::VC_CALL, $2, $4, @$); }

    /* 
        routines 
    */
    /*|                 ID '(' expr_list ')'  { $$ = new RoutineCall((std::string*) 0, $1, $3, @$); }*/
    |    DOUBLE_COLON ID '(' expr_list ')'  { $$ = new RoutineCall((std::string*) 0, $2, $4, @$); }
    | ID DOUBLE_COLON ID '(' expr_list ')'  { $$ = new RoutineCall(              $1, $3, $5, @$); }

    /* 
        methods 
    */
    | postfix_expr ':' ID '(' expr_list ')' { $$ = new ReaderCall(       $1, $3, $5, @$); }
    | postfix_expr '.' ID '(' expr_list ')' { $$ = new WriterCall(       $1, $3, $5, @$); }
    | ':' ID '(' expr_list ')'              { $$ = new ReaderCall((Expr*) 0, $2, $4, @$); }
    | '.' ID '(' expr_list ')'              { $$ = new WriterCall((Expr*) 0, $2, $4, @$); }
    ;

primary_expr
    : ID               { $$ = new  Id($1, @$); }
    | NIL '{' type '}' { $$ = new Nil($3, @$); }
    | SELF             { $$ = new Self(@$); }
    | '@'              { $$ = new SimdIndex(@$); }
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
    :       expr                         { $$ = new ExprList(    0, $1,  0, @$); }
    | INOUT expr                         { $$ = new ExprList(Token::INOUT, $2,  0, @$); }
    |       expr ',' expr_list_not_empty { $$ = new ExprList(    0, $1, $3, @$); }
    | INOUT expr ',' expr_list_not_empty { $$ = new ExprList(Token::INOUT, $2, $4, @$); }
    ;

decl
    : type ID { $$ = new Decl($1, $2, @$); }
    ;

tuple
    : expr           { $$ = new Tuple($1,  0, @$); }
    | decl           { $$ = new Tuple($1,  0, @$); }
    | expr ',' tuple { $$ = new Tuple($1, $3, @$); }
    | decl ',' tuple { $$ = new Tuple($1, $3, @$); }
    ;

bare_type
    : ID                 { $$ = new BaseType(0, $1, @$); }
    | PTR   '{' type '}' { $$ = new      Ptr(0, $3, @$); }
    | ARRAY '{' type '}' { $$ = new    Array(0, $3, @$); }
    | SIMD  '{' type '}' { $$ = new     Simd(0, $3, @$); }
    ;

type
    : ID                       { $$ = new BaseType(Token::  VAR, $1, @$); }
    | CONST ID                 { $$ = new BaseType(Token::CONST, $2, @$); }
    | PTR         '{' type '}' { $$ = new      Ptr(Token::  VAR, $3, @$); }
    | CONST PTR   '{' type '}' { $$ = new      Ptr(Token::CONST, $4, @$); }
    | ARRAY       '{' type '}' { $$ = new    Array(Token::  VAR, $3, @$); }
    | CONST ARRAY '{' type '}' { $$ = new    Array(Token::CONST, $4, @$); }
    | SIMD        '{' type '}' { $$ = new     Simd(Token::  VAR, $3, @$); }
    | CONST SIMD  '{' type '}' { $$ = new     Simd(Token::CONST, $4, @$); }
    ;

%%

/*
    FIXME better syntax error handling
*/

void Parser::error(const location_type& loc, const std::string& str)
{
    errorf(loc, "parse error");
}
