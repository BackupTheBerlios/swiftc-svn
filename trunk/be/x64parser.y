%{

#include <iostream>
#include <string>
#include <typeinfo>

#include "me/constpool.h"

#include "be/x64codegen.h"
#include "be/x64parser.h"
#include "be/x64regalloc.h"

using namespace be;

void x64error(char* s);

std::string suffix(int);
std::string reg2str(me::Reg* reg);
inline std::string cst2str(me::Const* cst);
std::string op2str(me::Op* op);
std::string instr2str(me::AssignInstr* ai);

/*
    macro magic for less verbose code emitting
*/

#define EMIT(e) *x64_ofs << '\t' << e << '\n';

%}

%union
{
    int int_;

    me::Op*    op_;
    me::Undef* undef_;
    me::Const* const_;
    me::Reg*   reg_;

    me::LabelInstr*  label_;
    me::GotoInstr*   goto_;
    me::BranchInstr* branch_;
    me::AssignInstr* assign_;
}

/*
    tokens
*/

/* instructions */
%token <label_>  LABEL
%token <goto_>   GOTO BRANCH
%token <assign_> MOV ADD SUB MUL DIV
%token NOP

/* types */
%token BOOL 
%token INT8  INT16  INT32  INT64   SAT8  SAT16
%token UINT8 UINT16 UINT32 UINT64 USAT8 USAT16 
%token REAL32 REAL64

/* operands */
%token <undef_> UNDEF
%token <const_> CONST CST_0 CST_1
%token <reg_>   REG_1 REG_2 REG_3

/* special tokens */
%token ERROR

%start instruction

/*
    types
*/

%type <int_> bool_type int_type sint_type uint_type int_or_bool_type real_type any_type
%type <op_> const_or_reg // TODO
%type <assign_> real_op

%% 

instruction
    : LABEL { *x64_ofs << $1->toString() << ":\n"; }
    | NOP { /* do nothing */ }
    | jump_instruction
    | assign_instruction
    ;

jump_instruction
    : GOTO { EMIT("jmp " << $1->label()->toString()) }
    /*| BRANCH CONST { }*/
    /*| BRANCH REG_2 { */
        /*EMIT("orb " << reg2str($2) ", " << reg2str($2))*/
        /*EMIT("jz " << $1)*/
    /*}*/
    ;

assign_instruction
    : move
    | int_add
    | int_sub
    | real_instr
    ;

move
    : MOV any_type UNDEF { /* emit no code: move UNDEF, %r1 */ }
    | MOV any_type CONST { EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg())) }
    | MOV any_type REG_1 { /* emit no code: mov %r1, %r1 */ }
    | MOV any_type REG_2 { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) }
    ;

int_add
    : ADD int_type CONST CONST /* mov (c1 + c2), r1 */
        { EMIT("mov" << suffix($2) << ' ' << ($3->value_.uint64_ + $4->value_.uint64_) << ", " << reg2str($1->resReg())) } 
    | ADD int_type CONST REG_1 /* add c, r1 */
        { EMIT("add" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($4)) }
    | ADD int_type CONST REG_2 /* mov c, r1; add r2, r1 */ 
        { EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    | ADD int_type REG_1 CONST /* add c, r1 */
        { EMIT("add" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3)) }
    | ADD int_type REG_1 REG_1 /* add r1, r1 or: shl 2, r2 */
        { EMIT("shl" << suffix($2) << ' ' << reg2str($3)) }
    | ADD int_type REG_1 REG_2 /* add r2, r1 */
        { EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) }
    | ADD int_type REG_2 CONST /* mov c, r1; add r2, r1 */ 
        { EMIT("mov" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) }
    | ADD int_type REG_2 REG_1 /* add r2, r1 */
        { EMIT("add" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4)) }
    | ADD int_type REG_2 REG_2 /* mov r2, r1; shl 2, r1 */
        { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("shl" << suffix($2) << ' ' << reg2str($1->resReg())) }
    | ADD int_type REG_2 REG_3 /* mov r2, r1; add r3, r1 */
        { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    ;

int_sub
    : SUB int_type CONST CONST /* mov (c1 - c2), r1 */
        { EMIT("mov" << suffix($2) << ' ' << ($3->value_.uint64_ - $4->value_.uint64_) << ", " << reg2str($1->resReg())) } 
    | SUB int_type CONST REG_1 /* sub c, r1; neg r1 */
        { EMIT("sub" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($4)) 
          EMIT("neg" << suffix($2) << ' ' << reg2str($4)) }
    | SUB int_type CONST REG_2 /* mov c, r1; sub r2, r1 */
        { EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    | SUB int_type REG_1 CONST /* sub c, r1 */
        { EMIT("sub" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3)) }
    | SUB int_type REG_1 REG_1 /* xor r1, r1 */
        { EMIT("xor" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($3)) }
    | SUB int_type REG_1 REG_2 /* sub r2, r1 */
        { EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) }
    | SUB int_type REG_2 CONST /* mov c, r1; sub r2, r1 */ 
        { EMIT("mov" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) }
    | SUB int_type REG_2 REG_1 /* sub r2, r1; neg r1*/
        { EMIT("sub" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4))
          EMIT("neg" << suffix($2) << ' ' << reg2str($4)) }
    | SUB int_type REG_2 REG_2 /* xor r1, r1 */
        { EMIT("xor" << suffix($2) << ' ' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) }
    | SUB int_type REG_2 REG_3 /* mov r2, r1; sub r3, r1 */
        { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    ;
    
real_instr
    : real_op real_type const_or_reg const_or_reg /* mov r2, r1; add r3, r1 */
        { EMIT("mov" << suffix($2) << ' ' << op2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << op2str($4) << ", " << reg2str($1->resReg())) }
    ;

real_op
    : ADD { $$ = $1; }
    | SUB { $$ = $1; }
    | MUL { $$ = $1; }
    | DIV { $$ = $1; }
    ;

bool_type
    : BOOL { $$ = BOOL; }
    ;

sint_type
    : INT8  { $$ = INT8; }
    | INT16 { $$ = INT16; }
    | INT32 { $$ = INT32; }
    | INT64 { $$ = INT64; }
    ;

uint_type
    : UINT8  { $$ = UINT8;; } 
    | UINT16 { $$ = UINT16; }
    | UINT32 { $$ = UINT32; }
    | UINT64 { $$ = UINT64; }
    ;

real_type
    : REAL32 { $$ = REAL32; }
    | REAL64 { $$ = REAL64; } 
    ;

int_type
    : sint_type { $$ = $1; }
    | uint_type { $$ = $1; }
    ;

int_or_bool_type
    : int_type  { $$ = $1; }
    | bool_type { $$ = $1; }
    ;

any_type
    : int_or_bool_type { $$ = $1; }
    | real_type { $$ = $1; }
    ;

const_or_reg
    : CONST { $$ = $1; }
    | REG_1 { $$ = $1; }
    | REG_2 { $$ = $1; }
    | REG_3 { $$ = $1; }
    ;

%%

std::string suffix(int type)
{
    switch (type)
    {
        case BOOL:
        case INT8:
        case UINT8:
            return "b"; // byte

        case INT16:
        case UINT16:
            return "s"; // short

        case INT32:
        case UINT32:
            return "l"; // long

        case INT64:
        case UINT64:
            return "q"; // quad

        case REAL32:
            return "ss";// scalar single
            
        case REAL64:
            return "sd";// scalar double

        default:
            return "TODO";
    }
}

std::string cst2str(me::Const* cst)
{
    std::ostringstream oss;
    
    if (cst->type_ == me::Op::R_REAL32)
        oss << ".LC" << me::constpool->uint32_[cst->value_.uint32_];
    else if (cst->type_ == me::Op::R_REAL64)
        oss << ".LC" << me::constpool->uint64_[cst->value_.uint64_];
    else
        oss << '$' << cst->value_.uint64_;

    return oss.str();
}

inline std::string reg2str(me::Reg* reg) 
{
    return X64RegAlloc::reg2String(reg);
}

std::string op2str(me::Op* op)
{
    if ( typeid(*op) == typeid(me::Const) )
        return cst2str( (me::Const*) op );
    else
    {
        swiftAssert( typeid(*op) == typeid(me::Reg), "must be a Reg" );
        return reg2str( (me::Reg*) op );
    }

    // avoid warning
    return "";
}

std::string instr2str(me::AssignInstr* ai)
{
    switch (ai->kind_)
    {
        case '+': return "add";
        case '-': return "sub";
        case '*': return "mul";
        case '/': return "div";
        default:
            swiftAssert( false, "unreachable code" );
    }

    return "";
}

