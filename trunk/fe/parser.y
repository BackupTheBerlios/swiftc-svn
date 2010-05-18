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


%debug
%define namespace "swift"
%define parser_class_name "Parser"
%parse-param {Context* ctxt_}

%{

#include <iostream>

#include "fe/auto.h"
#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/scope.h"
#include "fe/sig.h"
#include "fe/simdprefix.h"
#include "fe/stmnt.h"
#include "fe/type.h"
#include "fe/typenode.h"
#include "fe/var.h"

using namespace swift;

%}

%union
{
    int          int_;
    bool         bool_;
    std::string* id_;

    Class*       class_;
    Decl*        decl_;
    Def*         def_;
    Expr*        expr_;
    ExprList*    exprList_;
    MemberVar*   memberVar_;
    MemberFct*   memberFct_;
    Module*      module_;
    InOut*       io_;
    Scope*       scope_;
    SimdPrefix*  simdPrefix_;
    Stmnt*       stmnt_;
    Tuple*       tuple_;
    Type*        type_;
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
%token VAR CONST REF CONST_REF

%token SELF

// method qualifier
%token READER WRITER ROUTINE OPERATOR ASSIGN

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
// logical operators
%token L_AND L_OR L_XOR L_NOT

%token INC DEC
%token MOVE SWAP

%token C_CALL VC_CALL

%token ASGN ASGN_ADD ASGN_SUB ASGN_MUL ASGN_DIV ASGN_MOD ASGN_AND ASGN_OR ASGN_XOR

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

%type <bool_> simd_modifier
%type <decl_> decl
%type <exprList_> expr_list expr_list_not_empty
%type <expr_> expr rel_expr mul_expr add_expr postfix_expr un_expr primary_expr
%type <int_> method_qualifier operator assign
%type <memberFct_> member_function
%type <memberVar_> member_var
%type <scope_> if_head
%type <simdPrefix_> simd_prefix
%type <stmnt_>  stmnt
%type <tuple_> tuple
%type <type_> type bare_type

%destructor { } <int_>
%destructor { } <bool_>
%destructor { delete $$; } <*>

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
    | definitions definition
    ;

definition
    : class
    | error EOL
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

class
    : simd_modifier CLASS ID EOL
        {
            $<class_>$ = new Class(@$, $1, $3);
            ctxt_->module_->insert(ctxt_, $<class_>$);
        }
        class_body END EOL 
        {
            $<class_>5->addAssignCreate(ctxt_);
        }
    ;

class_body
    : /* empty */
    | class_body class_member 
    | class_body EOL 
    | error EOL 
    ;

class_member
    : member_function
    | member_var
    ;

/*
    member_functions
*/

method_qualifier
    : READER  { $$ = Token::READER; }
    | WRITER  { $$ = Token::WRITER; }
    | ROUTINE { $$ = Token::ROUTINE; }
    ;

member_function
    : simd_modifier method_qualifier ID
        {
            Scope* scope = ctxt_->enterScope();

            switch ($2)
            {
                case Token::READER:  $<memberFct_>$ = new Reader (@$, $1, $3, scope); break;
                case Token::WRITER:  $<memberFct_>$ = new Writer (@$, $1, $3, scope); break;
                case Token::ROUTINE: $<memberFct_>$ = new Routine(@$, $1, $3, scope); break;
                default: swiftAssert(false, "unreachable code");
            }

            ctxt_->class_->insert(ctxt_, $<memberFct_>$);
            ctxt_->memberFct_->sig_.setInList(ctxt_);
        }
        '(' io_list ')' 
        {
            ctxt_->memberFct_->sig_.setOutList(ctxt_);
        }
        ret_list stmnt_list END EOL
        {
            $$ = $<memberFct_>4;
            $$->sig_.buildTypeLists();
            ctxt_->leaveScope();
        }
    | simd_modifier OPERATOR operator
        {
            Scope* scope = ctxt_->enterScope();
            $<memberFct_>$ = new Operator(@$,  $1, $3, scope);
            ctxt_->class_->insert(ctxt_, $<memberFct_>$);
            ctxt_->memberFct_->sig_.setInList(ctxt_);
        }
        '(' io_list ')' 
        {
            ctxt_->memberFct_->sig_.setOutList(ctxt_);
        }
        ret_list stmnt_list END EOL
        {
            $$ = $<memberFct_>4;
            $$->sig_.buildTypeLists();
            ctxt_->leaveScope();
        }
    | simd_modifier ASSIGN assign
        {
            Scope* scope = ctxt_->enterScope();
            $<memberFct_>$ = new Assign(@$,  $1, $3, scope);
            ctxt_->class_->insert(ctxt_, $<memberFct_>$);
            ctxt_->memberFct_->sig_.setInList(ctxt_);
        }
        '(' io_list ')' EOL stmnt_list END EOL
        {
            $$ = $<memberFct_>4;
            $$->sig_.buildTypeLists();
            ctxt_->leaveScope();
        }
    | simd_modifier CREATE
        {
            Scope* scope = ctxt_->enterScope();
            $<memberFct_>$ = new Create(@$, $1, scope);
            ctxt_->class_->insert(ctxt_, $<memberFct_>$);
            ctxt_->memberFct_->sig_.setInList(ctxt_);
        }
        '(' io_list')' EOL stmnt_list END EOL
        {
            $$ = $<memberFct_>3;
            $$->sig_.buildTypeLists();
            ctxt_->leaveScope();
        }
    ;

operator
    :   ADD    { $$ = Token::ADD; }
    |   SUB    { $$ = Token::SUB; }
    |   MUL    { $$ = Token::MUL; }
    |   DIV    { $$ = Token::DIV; }
    |   MOD    { $$ = Token::MOD; }
    |   EQ     { $$ = Token::EQ;  }
    |   NE     { $$ = Token::NE;  }
    |   LT     { $$ = Token::LT;  }
    |   LE     { $$ = Token::LE;  }
    |   GT     { $$ = Token::GT;  }
    |   GE     { $$ = Token::GE;  }
    |   AND    { $$ = Token::AND; }
    |   OR     { $$ = Token::OR;  }
    |   XOR    { $$ = Token::XOR; }
    |   NOT    { $$ = Token::NOT; }
    |   L_AND  { $$ = Token::L_AND; }
    |   L_OR   { $$ = Token::L_OR;  }
    |   L_XOR  { $$ = Token::L_XOR; }
    |   L_NOT  { $$ = Token::L_NOT; }
    ;

assign
    : ASGN  { $$ = Token::ASGN; }
    ;

io_list_not_empty
    : io
    | io_list_not_empty ',' io
    ;

io_list
    : /* emtpy */
    | io_list_not_empty
    ;

ret_list
    : EOL
    | ARROW io_list_not_empty EOL 
    ;

io
    : bare_type ID { ctxt_->ios_->push_back( new InOut(@$, $1, $2) ); }
    ;

/*
    ***********
    member vars
    ***********
*/

member_var
    : type ID EOL
        {
            $$ = new MemberVar(@$, $1, $2);
            ctxt_->class_->insert(ctxt_, $$);
        }
    ;

/*
    **********
    statements
    **********
*/

stmnt_list
    : /* empty */
    | stmnt_list stmnt { ctxt_->scope()->appendStmnt($2); }
    | stmnt_list error EOL
    ;

stmnt
    /*
        basic stmnts
    */
    : decl EOL { $$ = new DeclStmnt(@$, $1); }

    |             expr EOL { $$ = new ExprStmnt(@$,  0, $1); }
    | simd_prefix expr EOL { $$ = new ExprStmnt(@$, $1, $2); }

    |             tuple ASGN expr_list_not_empty EOL { $$ = new AssignStmnt(@$,  0, Token::ASGN, $1, $3); }
    | simd_prefix tuple ASGN expr_list_not_empty EOL { $$ = new AssignStmnt(@$, $1, Token::ASGN, $2, $4); }

    /*
        control flow stmnts
    */
    | WHILE 
        { 
            $<scope_>$ = ctxt_->enterScope(); 
        } 
        expr EOL stmnt_list END EOL 
        { 
            ctxt_->leaveScope();
            $$ = new WhileStmnt(@$, $3, $<scope_>2);
        }
    | REPEAT EOL 
        { 
            $<scope_>$ = ctxt_->enterScope(); 
        } 
        stmnt_list UNTIL expr EOL 
        { 
            ctxt_->leaveScope();
            $$ = new RepeatUntilStmnt(@$, $6, $<scope_>3);
        }
    | SCOPE EOL 
        { 
            $<scope_>$ = ctxt_->enterScope(); 
        } 
        stmnt_list END EOL 
        { 
            ctxt_->leaveScope();
            $$ = new ScopeStmnt(@$, $<scope_>3);
        }

    | if_head expr EOL stmnt_list END EOL 
        { 
            ctxt_->leaveScope();
            $$ = new IfElStmnt(@$, $2, $1, 0); 
        }
    | if_head expr EOL stmnt_list ELSE EOL 
        { 
            ctxt_->leaveScope(); 
            $<scope_>$ = ctxt_->enterScope(); 
        } 
        stmnt_list END EOL 
        { 
            ctxt_->leaveScope();
            $$ = new IfElStmnt(@$, $2, $1, $<scope_>7); 
        }

    /* 
        jump stmnts
    */
    | RETURN   EOL { $$ = new CFStmnt(@$, Token::RETURN);   }
    | BREAK    EOL { $$ = new CFStmnt(@$, Token::BREAK);    }
    | CONTINUE EOL { $$ = new CFStmnt(@$, Token::CONTINUE); }
    ;


if_head
    : IF { $$ = ctxt_->enterScope(); }
    ;

/*
    ***********
    simd prefix
    ***********
*/

simd_prefix
    : SIMD '[' expr ',' expr ']' ':' { $$ = new SimdPrefix(@$, $3, $5); } 
    | SIMD '['      ',' expr ']' ':' { $$ = new SimdPrefix(@$,  0, $4); } 
    | SIMD '[' expr ','      ']' ':' { $$ = new SimdPrefix(@$, $3,  0); } 
    | SIMD '['      ','      ']' ':' { $$ = new SimdPrefix(@$,  0,  0); } 
    | SIMD                       ':' { $$ = new SimdPrefix(@$,  0,  0); } 
    ;

/*
    ***********
    expressions
    ***********
*/

expr
    : rel_expr           { $$ = $1; }
    | expr EQ add_expr   { $$ = new BinExpr(@$, Token::EQ, $1, $3); }
    | expr NE add_expr   { $$ = new BinExpr(@$, Token::NE, $1, $3); }
    ;

rel_expr
    : add_expr           { $$ = $1; }
    | expr LT add_expr   { $$ = new BinExpr(@$, Token::LT, $1, $3); }
    | expr LE add_expr   { $$ = new BinExpr(@$, Token::LE, $1, $3); }
    | expr GT add_expr   { $$ = new BinExpr(@$, Token::GT, $1, $3); }
    | expr GE add_expr   { $$ = new BinExpr(@$, Token::GE, $1, $3); }
    ;

add_expr
    : mul_expr              { $$ = $1; }
    | add_expr ADD mul_expr { $$ = new BinExpr(@$, Token::ADD, $1, $3); }
    | add_expr SUB mul_expr { $$ = new BinExpr(@$, Token::SUB, $1, $3); }
    ;

mul_expr
    : un_expr               { $$ = $1; }
    | mul_expr MUL un_expr  { $$ = new BinExpr(@$, Token::MUL, $1, $3); }
    | mul_expr DIV un_expr  { $$ = new BinExpr(@$, Token::DIV, $1, $3); }
    ;

un_expr
    : postfix_expr  { $$ = $1; }
    | SUB   un_expr { $$ = new UnExpr(@$, Token::SUB, $2); }
    | ADD   un_expr { $$ = new UnExpr(@$, Token::ADD, $2); }
    | NOT   un_expr { $$ = new UnExpr(@$, Token::NOT, $2); }
    | L_NOT un_expr { $$ = new UnExpr(@$, Token::NOT, $2); }
    ;

postfix_expr
    : primary_expr { $$ = $1; }

    /* 
        accesses
    */
    | postfix_expr '.' ID       { $$ = new MemberAccess(@$, $1, $3); }
    | '.' ID                    { $$ = new MemberAccess(@$,  0, $2); }
    | postfix_expr '[' expr ']' { $$ = new IndexExpr(@$, $1, $3); }

    /* 
        c_call 
    */
    |  C_CALL type ID '(' expr_list ')' { $$ = new CCall(@$, $2, Token:: C_CALL, $3, $5); }
    | VC_CALL type ID '(' expr_list ')' { $$ = new CCall(@$, $2, Token::VC_CALL, $3, $5); }
    |  C_CALL ID '(' expr_list ')'      { $$ = new CCall(@$,  0, Token:: C_CALL, $2, $4); }
    | VC_CALL ID '(' expr_list ')'      { $$ = new CCall(@$,  0, Token::VC_CALL, $2, $4); }

    /* 
        routines 
    */
    |    DOUBLE_COLON ID '(' expr_list ')' { $$ = new RoutineCall(@$, (std::string*) 0, $2, $4); }
    | ID DOUBLE_COLON ID '(' expr_list ')' { $$ = new RoutineCall(@$,               $1, $3, $5); }

    /* 
        methods 
    */
    | postfix_expr ':' ID '(' expr_list ')' { $$ = new ReaderCall(@$,        $1, $3, $5); }
    | postfix_expr '.' ID '(' expr_list ')' { $$ = new WriterCall(@$,        $1, $3, $5); }
    | ':' ID '(' expr_list ')'              { $$ = new ReaderCall(@$, (Expr*) 0, $2, $4); }
    | '.' ID '(' expr_list ')'              { $$ = new WriterCall(@$, (Expr*) 0, $2, $4); }
    ;

primary_expr
    : ID               { $$ = new  Id(@$, $1); }
    | NIL '{' type '}' { $$ = new Nil(@$, $3); }
    | SELF             { $$ = new Self(@$); }
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
    | '(' error ')'    { $$ =  0; }
    ;

expr_list
    : /* empty */           { $$ =  0; }
    | expr_list_not_empty   { $$ = $1; }
    ;

expr_list_not_empty
    : expr                         { $$ = new ExprList(@$, $1,  0); }
    | expr ',' expr_list_not_empty { $$ = new ExprList(@$, $1, $3); }
    ;

decl
    : type ID { $$ = new Decl(@$, $1, $2); }
    ;

tuple
    : expr           { $$ = new Tuple(@$, $1,  0); }
    | decl           { $$ = new Tuple(@$, $1,  0); }
    | expr ',' tuple { $$ = new Tuple(@$, $1, $3); }
    | decl ',' tuple { $$ = new Tuple(@$, $1, $3); }
    ;

bare_type
    : ID                 { $$ = new BaseType(@$, ctxt_->var_ ? Token::VAR : Token::CONST, $1); }
    | PTR   '{' type '}' { $$ = new      Ptr(@$, ctxt_->var_ ? Token::VAR : Token::CONST, $3); }
    | ARRAY '{' type '}' { $$ = new    Array(@$, ctxt_->var_ ? Token::VAR : Token::CONST, $3); }
    | SIMD  '{' type '}' { $$ = new     Simd(@$, ctxt_->var_ ? Token::VAR : Token::CONST, $3); }
    ;

type
    : ID                       { $$ = new BaseType(@$, Token::  VAR, $1); }
    | CONST ID                 { $$ = new BaseType(@$, Token::CONST, $2); }
    | PTR         '{' type '}' { $$ = new      Ptr(@$, Token::  VAR, $3); }
    | CONST PTR   '{' type '}' { $$ = new      Ptr(@$, Token::CONST, $4); }
    | ARRAY       '{' type '}' { $$ = new    Array(@$, Token::  VAR, $3); }
    | CONST ARRAY '{' type '}' { $$ = new    Array(@$, Token::CONST, $4); }
    | SIMD        '{' type '}' { $$ = new     Simd(@$, Token::  VAR, $3); }
    | CONST SIMD  '{' type '}' { $$ = new     Simd(@$, Token::CONST, $4); }
    ;

%%

/*
    FIXME better syntax error handling
*/

void Parser::error(const location_type& loc, const std::string& str)
{
    ctxt_->result_ = false;
    errorf(loc, "parse error");
}
