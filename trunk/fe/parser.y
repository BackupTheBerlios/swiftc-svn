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
%expect 6

%{

#include <iostream>

#include "fe/auto.h"
#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/tnlist.h"
#include "fe/scope.h"
#include "fe/sig.h"
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
    MemberFct*   memberFct_;
    Module*      module_;
    Scope*       scope_;
    Stmnt*       stmnt_;
    Type*        type_;
};

/*
    tokens
*/

// literals
%token <expr_> L_INT  L_INT8   L_INT16  L_INT32  L_INT64  L_SAT8  L_SAT16
%token <expr_> L_UINT L_UINT8  L_UINT16 L_UINT32 L_UINT64 L_USAT8 L_USAT16
%token <expr_> L_REAL L_REAL32 L_REAL64
%token <expr_> L_INDEX L_BOOL 

%token NIL

// types
%token PTR ARRAY SIMD

// type modifiers
%token VAR CONST REF CONST_REF

%token SELF

// method qualifier
%token READER WRITER ROUTINE

%token ARROW
%token DOUBLE_COLON

// constructor / destructor
%token CREATE DESTROY

// built-in functions
%token SHL SHR ROL ROR

// operators in precedence order
%token <id_> MUL ADD EQ REL AND XOR OR
%token <id_> ASGN

%token INC DEC
%token MOVE SWAP

%token C_CALL VC_CALL

// control flow
%token IF ELSE ELIF FOR WHILE REPEAT UNTIL
%token RETURN BREAK CONTINUE

// protection
%token PUBLIC PROTECTED PACKAGE PRIVATE

// miscellaneous
%token SCOPE CLASS END EOL

%token <id_> VAR_ID TYPE_ID SIMD_INDEX

/*
    types
*/

%type <id_> fct_id op_id
%type <bool_> simd_modifier
%type <decl_> decl
%type <expr_> primary_expr postfix_expr un_expr mul_expr add_expr eq_expr rel_expr and_expr xor_expr or_expr usr_expr expr
%type <int_> method_qualifier
%type <memberFct_> member_function
%type <scope_> if_head
%type <stmnt_> stmnt
%type <type_> type var_type const_type

%destructor { } <int_>
%destructor { } <bool_>
%destructor { delete $$; } <*>

%start file

%%

eol
    : EOL
    | eol EOL
    ;

file
    : module
    | eol module
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
    | error eol
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
    : simd_modifier CLASS TYPE_ID eol
        {
            $<class_>$ = new Class(@$, $1, $3);
            ctxt_->module_->insert($<class_>$);
        }
        class_body END eol 
        {
            $<class_>5->addAssignCreate(ctxt_);
        }
    ;

class_body
    : /* empty */
    | class_body class_member 
    | class_body eol 
    | error eol 
    ;

class_member
    : member_function
    | member_var
    ;

/*
    member_functions
*/

method_qualifier
    : READER  { $$ = Token::WRITER; }
    | WRITER  { $$ = Token::WRITER; }
    | ROUTINE { $$ = Token::ROUTINE; }
    ;

member_function
    : simd_modifier method_qualifier fct_id
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
        }
        '(' param_list ')' ret_list stmnt_list END eol
        {
            $$ = $<memberFct_>4;
            $$->sig().buildTypeLists();
            ctxt_->leaveScope();
        }
    | simd_modifier CREATE
        {
            Scope* scope = ctxt_->enterScope();
            $<memberFct_>$ = new Create(@$, $1, scope);
            ctxt_->class_->insert(ctxt_, $<memberFct_>$);
        }
        '(' param_list')' eol stmnt_list END eol
        {
            $$ = $<memberFct_>3;
            $$->sig().buildTypeLists();
            ctxt_->leaveScope();
        }
    ;

param_list
    : /* emtpy */
    | param_list_not_empty
    ;

param_list_not_empty
    : param
    | param ',' param_list_not_empty
    ;

param
    : const_type VAR_ID { ctxt_->memberFct_->sig().in_.push_back( new Param(@$, $1, $2) ); }
    ;

ret_list
    : eol
    | ARROW retval_list_not_empty eol 
    ;

retval_list_not_empty
    : retval
    | retval ',' retval_list_not_empty
    ;

retval
    : var_type VAR_ID { ctxt_->memberFct_->sig().out_.push_back( new RetVal(@$, $1, $2) ); }
    ;

/*
    ***********
    member vars
    ***********
*/

member_var
    : type VAR_ID eol
        {
            ctxt_->class_->insert(ctxt_, new MemberVar(@$, $1, $2));
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
    | stmnt_list error eol
    ;

stmnt
    /*
        basic stmnts
    */
    : decl eol { $$ = new DeclStmnt(@$, $1); }
    | expr eol { $$ = new ExprStmnt(@$, $1); }
    | tuple ASGN { ctxt_->pushExprList(); } expr_list_not_empty eol { $$ = new AssignStmnt(@$, $2, ctxt_->tuple_, ctxt_->popExprList()); ctxt_->newTuple(); }

    /*
        control flow stmnts
    */
    | WHILE 
        { 
            $<scope_>$ = ctxt_->enterScope(); 
        } 
        expr eol stmnt_list END eol 
        { 
            $$ = new WhileLoop(@$, $<scope_>2, $3);
            ctxt_->leaveScope();
        }
    | REPEAT eol 
        { 
            $<scope_>$ = ctxt_->enterScope(); 
        } 
        stmnt_list UNTIL expr eol 
        { 
            $$ = new RepeatUntilLoop(@$, $<scope_>3, $6);
            ctxt_->leaveScope();
        }
    | SIMD VAR_ID ':' expr ',' expr eol
        {
            $<scope_>$ = ctxt_->enterScope();
        }
        stmnt_list END eol
        {
            $$ = new SimdLoop(@$, $<scope_>8, $2, $4, $6);
            ctxt_->leaveScope();
        }
    | SIMD ':' expr ',' expr eol
        {
            $<scope_>$ = ctxt_->enterScope();
        }
        stmnt_list END eol
        {
            $$ = new SimdLoop(@$, $<scope_>7, 0, $3, $5);
            ctxt_->leaveScope();
        }
    | SCOPE eol 
        { 
            $<scope_>$ = ctxt_->enterScope(); 
        } 
        stmnt_list END eol 
        { 
            $$ = new ScopeStmnt(@$, $<scope_>3);
            ctxt_->leaveScope();
        }

    | if_head expr eol stmnt_list END eol 
        { 
            ctxt_->leaveScope();
            $$ = new IfElStmnt(@$, $2, $1, 0); 
        }
    | if_head expr eol stmnt_list ELSE eol 
        { 
            ctxt_->leaveScope(); 
            $<scope_>$ = ctxt_->enterScope(); 
        } 
        stmnt_list END eol 
        { 
            $$ = new IfElStmnt(@$, $2, $1, $<scope_>7); 
            ctxt_->leaveScope();
        }

    /* 
        jump stmnts
    */
    | RETURN   eol { $$ = new CFStmnt(@$, Token::RETURN);   }
    | BREAK    eol { $$ = new CFStmnt(@$, Token::BREAK);    }
    | CONTINUE eol { $$ = new CFStmnt(@$, Token::CONTINUE); }
    ;


if_head
    : IF { $$ = ctxt_->enterScope(); }
    ;

/*
    ***********
    expressions
    ***********
*/

expr
    : usr_expr { $$ = $1; }
    ;

usr_expr
    : or_expr               { $$ = $1; }
    | usr_expr VAR_ID or_expr { $$ = new BinExpr(@$, $2, $1, $3); }
    ;

or_expr
    : xor_expr             { $$ = $1; }
    | or_expr OR xor_expr  { $$ = new BinExpr(@$, $2, $1, $3); }
    ;

xor_expr
    : and_expr              { $$ = $1; }
    | xor_expr XOR and_expr { $$ = new BinExpr(@$, $2, $1, $3); }
    ;

and_expr
    : rel_expr               { $$ = $1; }
    | and_expr AND rel_expr  { $$ = new BinExpr(@$, $2, $1, $3); }
    ;

rel_expr
    : eq_expr              { $$ = $1; }
    | rel_expr REL eq_expr { $$ = new BinExpr(@$, $2, $1, $3); }
    ;

eq_expr
    : add_expr              { $$ = $1; }
    | eq_expr EQ add_expr   { $$ = new BinExpr(@$, $2, $1, $3); }
    ;

add_expr
    : mul_expr              { $$ = $1; }
    | add_expr ADD mul_expr { $$ = new BinExpr(@$, $2, $1, $3); }
    ;

mul_expr
    : un_expr               { $$ = $1; }
    | mul_expr MUL un_expr  { $$ = new BinExpr(@$, $2, $1, $3); }
    ;

un_expr
    : postfix_expr { $$ = $1; }
    | ADD un_expr  { $$ = new UnExpr(@$, $1, $2); }
    | EQ  un_expr  { $$ = new UnExpr(@$, $1, $2); }
    ;

postfix_expr
    : primary_expr { $$ = $1; }

    /* 
        accesses
    */
    | postfix_expr '.' VAR_ID   { $$ = new MemberAccess(@$,           $1, $3); }
    | '.' VAR_ID                { $$ = new MemberAccess(@$, new Self(@$), $2); }
    | postfix_expr '[' expr ']' { $$ = new IndexExpr(@$, $1, $3); }
    | postfix_expr '@'          { $$ = new SimdIndexExpr(@$, $1,  0); }
    | postfix_expr SIMD_INDEX   { $$ = new SimdIndexExpr(@$, $1, $2); }

    /* 
        c_call 
    */
    |  C_CALL type VAR_ID { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new CCall(@$, $2, Token:: C_CALL, $3, ctxt_->popExprList()); }
    | VC_CALL type VAR_ID { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new CCall(@$, $2, Token::VC_CALL, $3, ctxt_->popExprList()); }
    |  C_CALL      VAR_ID { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new CCall(@$,  0, Token:: C_CALL, $2, ctxt_->popExprList()); }
    | VC_CALL      VAR_ID { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new CCall(@$,  0, Token::VC_CALL, $2, ctxt_->popExprList()); }

    /* 
        routines 
    */
    |         DOUBLE_COLON fct_id { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new RoutineCall(@$, new std::string( ctxt_->class_->cid() ), $2, ctxt_->popExprList()); }
    | TYPE_ID DOUBLE_COLON fct_id { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new RoutineCall(@$,                                      $1, $3, ctxt_->popExprList()); }

    /* 
        methods 
    */
    | postfix_expr '.' fct_id { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new MethodCall(@$,           $1, $3, ctxt_->popExprList()); }
    |              '.' fct_id { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new MethodCall(@$, new Self(@$), $2, ctxt_->popExprList()); }

    /*
        other calls
    */
    | VAR_ID { ctxt_->pushExprList(); } '(' expr_list ')' { $$ = new CreateCall(@$, $1, ctxt_->popExprList()); }
    ;

primary_expr
    : VAR_ID                  { $$ = new Id(@$, $1); }
    | CREATE VAR_ID                  { $$ = new Id(@$, $2); }
    | SELF                    { $$ = new Self(@$); }
    | L_INDEX                 { $$ = $1; }
    | L_INT                   { $$ = $1; }
    | L_INT8                  { $$ = $1; }
    | L_INT16                 { $$ = $1; }
    | L_INT32                 { $$ = $1; }
    | L_INT64                 { $$ = $1; }
    | L_SAT8                  { $$ = $1; }
    | L_SAT16                 { $$ = $1; }
    | L_UINT                  { $$ = $1; }
    | L_UINT8                 { $$ = $1; }
    | L_UINT16                { $$ = $1; }
    | L_UINT32                { $$ = $1; }
    | L_UINT64                { $$ = $1; }
    | L_USAT8                 { $$ = $1; }
    | L_USAT16                { $$ = $1; }
    | L_REAL                  { $$ = $1; }
    | L_REAL32                { $$ = $1; }
    | L_REAL64                { $$ = $1; }
    | L_BOOL                  { $$ = $1; }
    | NIL        '{' type '}' { $$ = new Nil(@$, $3); }
    | '(' expr ')'            { $$ = $2; }
    | '(' error ')'           { $$ = new ErrorExpr(@$); }
    ;

op_id
    : MUL { $$ = $1; }
    | ADD { $$ = $1; }
    | EQ  { $$ = $1; }
    | REL { $$ = $1; }
    | AND { $$ = $1; }
    | XOR { $$ = $1; }
    | OR  { $$ = $1; }
    ; 

fct_id
    : op_id  { $$ = $1; }
    | ASGN   { $$ = $1; }
    | VAR_ID { $$ = $1; }
    ;


expr_list
    : /* empty */
    | expr_list_not_empty
    ;

expr_list_not_empty
    : expr                         { ctxt_->topExprList()->append($1); }
    | expr_list_not_empty ',' expr { ctxt_->topExprList()->append($3); }
    ;

decl
    : type VAR_ID { $$ = new Decl(@$, $1, $2); }
    ;

tuple
    : expr           { ctxt_->tuple_->append($1); }
    | decl           { ctxt_->tuple_->append($1); }
    | tuple ',' expr { ctxt_->tuple_->append($3); }
    | tuple ',' decl { ctxt_->tuple_->append($3); }
    ;

var_type
    : TYPE_ID            { $$ = BaseType::create(@$, Token::VAR, $1, true); }
    | PTR   '{' type '}' { $$ = new      Ptr(@$, Token::VAR, $3); }
    | ARRAY '{' type '}' { $$ = new    Array(@$, Token::VAR, $3); }
    | SIMD  '{' type '}' { $$ = new     Simd(@$, Token::VAR, $3); }
    ;

const_type
    : TYPE_ID            { $$ = BaseType::create(@$, Token::CONST, $1, true); }
    | PTR   '{' type '}' { $$ = new      Ptr(@$, Token::CONST, $3); }
    | ARRAY '{' type '}' { $$ = new    Array(@$, Token::CONST, $3); }
    | SIMD  '{' type '}' { $$ = new     Simd(@$, Token::CONST, $3); }
    ;

type
    : TYPE_ID                  { $$ = BaseType::create(@$, Token::  VAR, $1); }
    | CONST VAR_ID             { $$ = BaseType::create(@$, Token::CONST, $2); }
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
