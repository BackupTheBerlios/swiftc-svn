%{

#include "be/x64parser.h"

#include "me/ssa.h"

#include "be/x64codegen.h"
#include "be/x64codegenhelpers.h"

using namespace be;

/*
    macro magic for less verbose code emitting
*/

#define EMIT(e) *x64_ofs << '\t' << e << '\n';

%}

%union
{
    int int_;

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
%token <label_>  X64_LABEL
%token <goto_>   X64_GOTO
%token <branch_> X64_BRANCH
%token <assign_> X64_MOV X64_ADD X64_SUB X64_MUL X64_DIV
%token <assign_> X64_EQ X64_NE X64_L X64_LE X64_G X64_GE
%token X64_NOP

/* types */
%token X64_BOOL 
%token X64_INT8  X64_INT16  X64_INT32  X64_INT64   X64_SAT8  X64_SAT16
%token X64_UINT8 X64_UINT16 X64_UINT32 X64_UINT64 X64_USAT8 X64_USAT16 
%token X64_REAL32 X64_REAL64

/* operands */
%token <undef_> X64_UNDEF
%token <const_> X64_CONST X64_CST_0 X64_CST_1
%token <reg_>   X64_REG_1 X64_REG_2 X64_REG_3

%start instruction

/*
    types
*/

%type <int_> bool_type int_type sint_type uint_type real_type int_or_bool_type
%type <assign_> add_or_mul cmp
%type <reg_> any_reg

%% 

instruction
    : X64_LABEL { *x64_ofs << $1->asmName() << ":\n"; }
    | X64_NOP { /* do nothing */ }
    | jump_instruction
    | assign_instruction
    ;

jump_instruction
    : X64_GOTO 
    { 
        EMIT("jmp " << $1->label()->asmName()) 
    }
    | X64_BRANCH bool_type X64_CONST 
    {   
        if ($3->value_.bool_) 
            EMIT("jmp " << $1->trueLabel()->asmName())
        else
            EMIT("jmp " << $1->falseLabel()->asmName())
    }
    | X64_BRANCH bool_type X64_REG_2 /* test r1, r1; jz falseLabel; jmp trueLabel */
    { 
        EMIT("testb " << reg2str($3) << ", " << reg2str($3))
        EMIT("jz " << $1->trueLabel()->asmName())
        EMIT("jmp " << $1->falseLabel()->asmName()) 
    }
    ;

assign_instruction
    : int_mov
    | int_add
    | int_sub
    | int_mul
    | int_div
    | int_cmp
    | real_mov
    | real_add_mul
    | real_sub
    | real_div
    | real_cmp
    ;

int_mov
    : X64_MOV int_or_bool_type X64_UNDEF 
    { 
        /* emit no code: mov X64_UNDEF, %r1 */ 
    }
    | X64_MOV int_or_bool_type X64_CONST 
    { 
        EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg())) 
    }
    | X64_MOV int_or_bool_type X64_REG_1 
    { 
        /* emit no code: mov %r1, %r1 */ 
    }
    | X64_MOV int_or_bool_type X64_REG_2 
    { 
        EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    ;

int_add
    : X64_ADD int_type X64_CONST X64_CONST /* mov (c1 + c2), r1 */
    {
        EMIT("mov" << suffix($2) << ' ' << cst_op_cst($1, $3, $4) << ", " << reg2str($1->resReg())) 
    } 
    | X64_ADD int_type X64_CONST X64_REG_1 /* add c, r1 */
    {
        EMIT("add" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($4)) 
    }
    | X64_ADD int_type X64_CONST X64_REG_2 /* mov c, r1; add r2, r1 */ 
    {
        EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg()))
        EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_ADD int_type X64_REG_1 X64_CONST /* add c, r1 */
    {
        EMIT("add" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3)) 
    }
    | X64_ADD int_type X64_REG_1 X64_REG_1 /* add r1, r1 or: shl 2, r2 */
    { 
          EMIT("shl" << suffix($2) << " $2, " << reg2str($3)) 
    }
    | X64_ADD int_type X64_REG_1 X64_REG_2 /* add r2, r1 */
    { 
          EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_ADD int_type X64_REG_2 X64_CONST /* mov c, r1; add r2, r1 */ 
    { 
          EMIT("mov" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    | X64_ADD int_type X64_REG_2 X64_REG_1 /* add r2, r1 */
    { 
          EMIT("add" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4)) 
    }
    | X64_ADD int_type X64_REG_2 X64_REG_2 /* mov r2, r1; shl 2, r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("shl" << suffix($2) << " $2,  " << reg2str($1->resReg())) 
    }
    | X64_ADD int_type X64_REG_2 X64_REG_3 /* mov r2, r1; add r3, r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

int_sub
    : X64_SUB int_type X64_CONST X64_CONST /* mov (c1 - c2), r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << cst_op_cst($1, $3, $4) << ", " << reg2str($1->resReg())) 
    } 
    | X64_SUB int_type X64_CONST X64_REG_1 /* sub c, r1; neg r1 */
    { 
          EMIT("sub" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($4)) 
          EMIT("neg" << suffix($2) << ' ' << reg2str($4)) 
    }
    | X64_SUB int_type X64_CONST X64_REG_2 /* mov c, r1; sub r2, r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB int_type X64_REG_1 X64_CONST /* sub c, r1 */
    { 
          EMIT("sub" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_1 /* xor r1, r1 */
    { 
          EMIT("xor" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_2 /* sub r2, r1 */
    { 
          EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_2 X64_CONST /* mov r2, r1; sub c, r1 */ 
    { 
          EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB int_type X64_REG_2 X64_REG_1 /* sub r2, r1; neg r1*/
    { 
          EMIT("sub" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4))
          EMIT("neg" << suffix($2) << ' ' << reg2str($4)) 
    }
    | X64_SUB int_type X64_REG_2 X64_REG_2 /* xor r1, r1 */
    { 
          EMIT("xor" << suffix($2) << ' ' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB int_type X64_REG_2 X64_REG_3 /* mov r2, r1; sub r3, r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

int_mul
    : X64_MUL int_type X64_CONST X64_CONST /* mov (c1 * c2), r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    } 
    | X64_MUL int_type X64_CONST X64_REG_1 /* mul c */
    { 
          EMIT(mul2str($2) << suffix($2) << ' ' << mcst2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_CONST /* mul c */
    { 
          EMIT(mul2str($2) << suffix($2) << ' ' << mcst2str($4)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_1 /* mul r1 */
    { 
          EMIT(mul2str($2) << suffix($2) << ' ' << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_2 /* mul r2 */
    { 
          EMIT(mul2str($2) << suffix($2) << ' ' << reg2str($4)) 
    }
    ;

int_div
    : X64_DIV int_type X64_CONST X64_CONST /* mov 1, r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV int_type X64_REG_1 X64_CONST /* div c */
    { 
          EMIT(div2str($2) << suffix($2) << ' ' << mcst2str($4)) 
    }
    | X64_DIV int_type X64_REG_1 X64_REG_1 /* mov 1, r1 */
    { 
          EMIT("mov" << suffix($2) << "$1, " << reg2str($3)) 
    }
    | X64_DIV int_type X64_REG_1 X64_REG_2 /* div r2 */
    { 
          EMIT(div2str($2) << suffix($2) << ' ' << reg2str($4)) 
    }
    ;

int_cmp
    : cmp int_type X64_CONST X64_CONST /* mov true, r1 or mov flase, r1 */
    { 
          EMIT("movb " << cst_op_cst($1, $3, $4) << ", " << reg2str($1->resReg())) 
    }
    | cmp int_type X64_CONST any_reg /* cmp r, c */
    { 
          EMIT("cmp" << suffix($2) << ' ' << reg2str($4) << ", " << cst2str($3))
          EMIT("set" << ccsuffix($1, $2) << "b " << reg2str($1->resReg())) 
    }
    | cmp int_type any_reg X64_CONST /* cmp r, c */
    { 
          EMIT("cmp" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3))
          EMIT("set" << ccsuffix($1, $2) << "b " << reg2str($1->resReg())) 
    }
    | cmp int_type any_reg any_reg   /* cmp r, r */
    { 
          EMIT("cmp" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3))
          EMIT("set" << ccsuffix($1, $2) << "b " << reg2str($1->resReg())) 
    }
    ;
    
real_mov
    : X64_MOV real_type X64_UNDEF 
    { 
        /* emit no code: mov X64_UNDEF, %r1 */ 
    }
    | X64_MOV real_type X64_CONST 
    { 
        EMIT("mov" << suffix($2) << ' ' << mcst2str($3) << ", " << reg2str($1->resReg())) 
    }
    | X64_MOV real_type X64_REG_1 
    { 
        /* emit no code: mov %r1, %r1 */ 
    }
    | X64_MOV real_type X64_REG_2 
    { 
        EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    ;

real_add_mul
    : add_or_mul real_type X64_CONST X64_CONST /* mov (c1 + c2), r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | add_or_mul real_type X64_CONST X64_REG_1 /* add c, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << ' ' << mcst2str($3) << ", " << reg2str($4)) 
    }
    | add_or_mul real_type X64_CONST X64_REG_2 /* mov c, r1; add r2, r1 */ 
    { 
          EMIT("mov" << suffix($2) << ' ' << mcst2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | add_or_mul real_type X64_REG_1 X64_CONST /* add c, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << ' ' << mcst2str($4) << ", " << reg2str($3)) 
    }
    | add_or_mul real_type X64_REG_1 X64_REG_1 /* add r1, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($3)) 
    }
    | add_or_mul real_type X64_REG_1 X64_REG_2 /* add r2, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) 
    }
    | add_or_mul real_type X64_REG_2 X64_CONST /* mov c, r1; add r2, r1 */ 
    { 
          EMIT("mov" << suffix($2) << ' ' << mcst2str($4) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    | add_or_mul real_type X64_REG_2 X64_REG_1 /* add r2, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4)) 
    }
    | add_or_mul real_type X64_REG_2 X64_REG_2 /* mov r2, r1; add r1, r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) 
    }
    | add_or_mul real_type X64_REG_2 X64_REG_3 /* mov r2, r1; add r3, r1 */
    { 
          EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

real_sub
    : X64_SUB real_type X64_CONST X64_CONST /* mov (c1 - c2), r1 */
    {
        EMIT("mov" << suffix($2) << ' ' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB real_type X64_CONST X64_REG_1 /* sub c, r1; xor signmask, r1 */
    {
        EMIT(instr2str($1) << suffix($2) << ' ' << mcst2str($3) << ", " << reg2str($4))  
        EMIT("xor" << suffix($2) << ' ' << neg_mask($2) << ", " << reg2str($4)) 
    }
    | X64_SUB real_type X64_CONST X64_REG_2 /* mov c, r1; sub r2, r1 */ 
    {
        EMIT("mov" << suffix($2) << ' ' << mcst2str($3) << ", " << reg2str($1->resReg()))
        EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB real_type X64_REG_1 X64_CONST /* sub c, r1 */
    {
        EMIT("sub" << suffix($2) << ' ' << mcst2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB real_type X64_REG_1 X64_REG_1 /* xor r1, r1 */
    {
        EMIT("xor" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_SUB real_type X64_REG_1 X64_REG_2 /* sub r2, r1 */
    {
        EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB real_type X64_REG_2 X64_CONST /* mov r2, r1; sub c, r1 */ 
    {
        EMIT("mov" << suffix($2) << ' ' <<  reg2str($3) << ", " << reg2str($1->resReg()))
        EMIT("sub" << suffix($2) << ' ' << mcst2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB real_type X64_REG_2 X64_REG_1 /* sub r2, r1 */
    {
        EMIT("sub" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4)) 
    }
    | X64_SUB real_type X64_REG_2 X64_REG_2 /* xor r1, r1 */
    {
        EMIT("xor" << suffix($2) << ' ' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB real_type X64_REG_2 X64_REG_3 /* mov r2, r1; sub r3, r1 */
    {
        EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
        EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

real_div
    : X64_DIV real_type X64_CONST X64_CONST /* mov (c1 / c2), r1 */
    {
        EMIT("mov" << suffix($2) << ' ' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV real_type X64_CONST X64_REG_2 /* mov c, r1; div r2, r1 */ 
    {
        EMIT("mov" << suffix($2) << ' ' << mcst2str($3) << ", " << reg2str($1->resReg()))
        EMIT("div" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV real_type X64_REG_1 X64_CONST /* div c, r1 */
    {
        EMIT("div" << suffix($2) << ' ' << mcst2str($4) << ", " << reg2str($3)) 
    }
    | X64_DIV real_type X64_REG_1 X64_REG_1 /* load with 1 */
    {
        swiftAssert(false, "TODO");
    }
    | X64_DIV real_type X64_REG_1 X64_REG_2 /* div r2, r1 */
    {
        EMIT("div" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_DIV real_type X64_REG_2 X64_CONST /* mov r2, r1; div c, r1 */ 
    {
        EMIT("mov" << suffix($2) << ' ' <<  reg2str($3) << ", " << reg2str($1->resReg()))
        EMIT("div" << suffix($2) << ' ' << mcst2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV real_type X64_REG_2 X64_REG_1 /* div r2, r1 */
    {
        EMIT("div" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4)) 
    }
    | X64_DIV real_type X64_REG_2 X64_REG_2 /* load with 1 */
    {
        swiftAssert(false, "TODO");
    }
    | X64_DIV real_type X64_REG_2 X64_REG_3 /* mov r2, r1; div r3, r1 */
    {
        EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
        EMIT("div" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

real_cmp
    : cmp real_type X64_CONST X64_CONST /* mov true, r1 or mov flase, r1 */
    { 
          EMIT("movb " << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | cmp real_type any_reg X64_CONST /* cmp c, r */
    { 
          EMIT("comi" << suffix($2) << ' ' << mcst2str($4) << ", " << reg2str($3))
          EMIT("set" << ccsuffix($1, $2) << "b " << reg2str($1->resReg())) 
    }
    | cmp real_type any_reg any_reg   /* cmp r, r */
    { 
          EMIT("comi" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3))
          EMIT("set" << ccsuffix($1, $2) << "b " << reg2str($1->resReg())) 
    }
    ;

add_or_mul
    : X64_ADD { $$ = $1; }
    | X64_MUL { $$ = $1; }
    ;

cmp
    : X64_EQ { $$ = $1; }
    | X64_NE { $$ = $1; }  
    | X64_L  { $$ = $1; }  
    | X64_LE { $$ = $1; }  
    | X64_G  { $$ = $1; } 
    | X64_GE { $$ = $1; }  
    ;

bool_type
    : X64_BOOL { $$ = X64_BOOL; }
    ;

sint_type
    : X64_INT8  { $$ = X64_INT8; }
    | X64_INT16 { $$ = X64_INT16; }
    | X64_INT32 { $$ = X64_INT32; }
    | X64_INT64 { $$ = X64_INT64; }
    ;

uint_type
    : X64_UINT8  { $$ = X64_UINT8;; } 
    | X64_UINT16 { $$ = X64_UINT16; }
    | X64_UINT32 { $$ = X64_UINT32; }
    | X64_UINT64 { $$ = X64_UINT64; }
    ;

real_type
    : X64_REAL32 { $$ = X64_REAL32; }
    | X64_REAL64 { $$ = X64_REAL64; } 
    ;

int_type
    : sint_type { $$ = $1; }
    | uint_type { $$ = $1; }
    ;

int_or_bool_type
    : int_type  { $$ = $1; }
    | bool_type { $$ = $1; }
    ;

any_reg
    : X64_REG_1 { $$ = $1; }
    | X64_REG_2 { $$ = $1; }
    | X64_REG_3 { $$ = $1; }
    ;

%%

// get rid of local define
#undef EMIT

