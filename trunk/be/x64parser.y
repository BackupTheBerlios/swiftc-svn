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
    me::MemVar* memVar_;
    me::Reg*   reg_;

    me::LabelInstr*  label_;
    me::GotoInstr*   goto_;
    me::BranchInstr* branch_;
    me::AssignInstr* assign_;
    me::Spill*       spill_;
    me::Reload*      reload_;
    me::Load*        load_;
    me::Store*       store_;
}

/*
    tokens
*/

/* instructions */
%token <label_>  X64_LABEL
%token <goto_>   X64_GOTO
%token <branch_> X64_BRANCH X64_BRANCH_TRUE X64_BRANCH_FALSE
%token <assign_> X64_MOV X64_ADD X64_SUB X64_MUL X64_DIV
%token <assign_> X64_EQ X64_NE X64_L X64_LE X64_G X64_GE
%token <spill_>  X64_SPILL
%token <reload_> X64_RELOAD
%token <load_>   X64_LOAD
%token <store_>  X64_STORE
%token X64_NOP

/* types */
%token X64_BOOL 
%token X64_INT8  UINT8
%token X64_INT16  X64_INT32  X64_INT64   X64_SAT8  X64_SAT16
%token X64_UINT8 X64_UINT16 X64_UINT32 X64_UINT64 X64_USAT8 X64_USAT16 
%token X64_REAL32 X64_REAL64
%token X64_PTR X64_STACK

/* operands */
%token <undef_>  X64_UNDEF
%token <const_>  X64_CONST X64_CST_0 X64_CST_1
%token <reg_>    X64_REG_1 X64_REG_2 X64_REG_3 
%token <memVar_> X64_MEM_VAR

%start instruction

/*
    types
*/

%type <int_> bool_type int_type sint_no8_type uint_no8_type int8_type real_type int_or_bool_type any_type
%type <assign_> add_or_mul cmp
%type <reg_> any_reg

%% 

instruction
    : X64_LABEL { *x64_ofs << $1->asmName() << ":\n"; }
    | X64_NOP { /* do nothing */ }
    | jump_instruction
    | assign_instruction
    | spill_reload
    | load_restore
    ;

jump_instruction
    : X64_GOTO 
    { 
        EMIT("jmp\t" << $1->label()->asmName()) 
    }
    | X64_BRANCH bool_type X64_CONST 
    {   
        if ($3->value_.bool_) 
            EMIT("jmp\t" << $1->trueLabel()->asmName())
        else
            EMIT("jmp\t" << $1->falseLabel()->asmName())
    }
    | X64_BRANCH bool_type X64_REG_2 /* test r1, r1; jz falseLabel; jmp trueLabel */
    { 
        if ($1->cc_ == me::BranchInstr::CC_NOT_SET)
        {
            EMIT("testb\t" << reg2str($3) << ", " << reg2str($3))
            EMIT("jnz\t" << $1->trueLabel()->asmName())
            EMIT("jmp\t" << $1->falseLabel()->asmName()) 
        }
        else
        {
            EMIT("j" << jcc($1) << '\t' << $1->trueLabel()->asmName())
            EMIT("jmp\t" << $1->falseLabel()->asmName()) 
        }
    }
    | X64_BRANCH_TRUE bool_type X64_CONST 
    {   
        if ($3->value_.bool_) 
            EMIT("jmp\t" << $1->trueLabel()->asmName())
    }
    | X64_BRANCH_TRUE bool_type X64_REG_2 
    { 
        if ($1->cc_ == me::BranchInstr::CC_NOT_SET)
        {
            EMIT("testb\t" << reg2str($3) << ", " << reg2str($3))
            EMIT("jnz\t"  << $1->trueLabel()->asmName())
        }
        else
            EMIT("j" << jcc($1) << '\t' << $1->trueLabel()->asmName())
    }
    | X64_BRANCH_FALSE bool_type X64_CONST 
    {   
        if (!$3->value_.bool_) 
            EMIT("jmp\t" << $1->falseLabel()->asmName())
    }
    | X64_BRANCH_FALSE bool_type X64_REG_2 
    { 
        if ($1->cc_ == me::BranchInstr::CC_NOT_SET)
        {
            EMIT("testb\t" << reg2str($3) << ", " << reg2str($3))
            EMIT("jz\t"  << $1->falseLabel()->asmName())
        }
        else
            EMIT("j" << jcc($1, true) << '\t' << $1->falseLabel()->asmName())
    }
    ;

spill_reload
    : X64_SPILL any_type X64_REG_2
    {
        EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
    }
    | X64_RELOAD any_type X64_REG_2
    {
        EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
    }
    ;

load_restore
    : X64_LOAD any_type X64_MEM_VAR
    { 
        EMIT("mov" << suffix($2) << '\t' << memvar2str($3, $1->getOffset()) << ", " << reg2str($1->resReg())) 
    }
    | X64_STORE any_type any_reg X64_MEM_VAR
    { 
        EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << memvar2str($4, $1->getOffset()))
    }
    ;

assign_instruction
    : int_mov
    | int_add
    | int_mul
    | int_sub
    | sint_no8_div
    | uint_no8_div
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
        EMIT("mov" << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($1->resReg())) 
    }
    | X64_MOV int_or_bool_type X64_REG_1 
    { 
        /* emit no code: mov %r1, %r1 */ 
    }
    | X64_MOV int_or_bool_type X64_REG_2 
    { 
        EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    ;

int_add
    : X64_ADD int_type X64_CONST X64_CONST /* mov (c1 + c2), r1 */
    {
        EMIT("mov" << suffix($2) << '\t' << cst_op_cst($1, $3, $4) << ", " << reg2str($1->resReg())) 
    } 
    | X64_ADD int_type X64_CONST X64_REG_1 /* add c, r1 */
    {
        EMIT("add" << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($4)) 
    }
    | X64_ADD int_type X64_CONST X64_REG_2 /* mov c, r1; add r2, r1 */ 
    {
        EMIT("mov" << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($1->resReg()))
        EMIT("add" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_ADD int_type X64_REG_1 X64_CONST /* add c, r1 */
    {
        EMIT("add" << suffix($2) << '\t' << cst2str($4) << ", " << reg2str($3)) 
    }
    | X64_ADD int_type X64_REG_1 X64_REG_1 /* shl 2, r2 */
    { 
          EMIT("shl" << suffix($2) << " $2, " << reg2str($3)) 
    }
    | X64_ADD int_type X64_REG_1 X64_REG_2 /* add r2, r1 */
    { 
          EMIT("add" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_ADD int_type X64_REG_2 X64_CONST /* mov c, r1; add r2, r1 */ 
    { 
          EMIT("mov" << suffix($2) << '\t' << cst2str($4) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    | X64_ADD int_type X64_REG_2 X64_REG_1 /* add r2, r1 */
    { 
          EMIT("add" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($4)) 
    }
    | X64_ADD int_type X64_REG_2 X64_REG_2 /* mov r2, r1; shl 2, r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("shl" << suffix($2) << " $2,  " << reg2str($1->resReg())) 
    }
    | X64_ADD int_type X64_REG_2 X64_REG_3 /* mov r2, r1; add r3, r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

int_mul
    : X64_MUL int_type X64_CONST X64_CONST /* mov (c1 * c2), r1 */
    {
        EMIT("mov" << suffix($2) << '\t' << cst_op_cst($1, $3, $4) << ", " << reg2str($1->resReg())) 
    } 
    | X64_MUL int_type X64_CONST X64_REG_1 /* mul c, r1 */
    {
        EMIT("imul" << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($4)) 
    }
    | X64_MUL int_type X64_CONST X64_REG_2 /* mov c, r1; mul r2, r1 */ 
    {
        EMIT("mov"  << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($1->resReg()))
        EMIT("imul" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_MUL int_type X64_REG_1 X64_CONST /* mul c, r1 */
    {
        EMIT("imul" << suffix($2) << '\t' << cst2str($4) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_1 /* mul r1, r1 */
    { 
          EMIT("imul" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($3))
    }
    | X64_MUL int_type X64_REG_1 X64_REG_2 /* mul r2, r1 */
    { 
          EMIT("imul" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_2 X64_CONST /* mov c, r1; mul r2, r1 */ 
    { 
          EMIT("mov"  << suffix($2) << '\t' << cst2str($4) << ", " << reg2str($1->resReg()))
          EMIT("imul" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    | X64_MUL int_type X64_REG_2 X64_REG_1 /* mul r2, r1 */
    { 
          EMIT("imul" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($4)) 
    }
    | X64_MUL int_type X64_REG_2 X64_REG_2 /* mov r2, r1; mul r2, r1 */
    { 
          EMIT("mov"  << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("imul" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    | X64_MUL int_type X64_REG_2 X64_REG_3 /* mov r2, r1; mul r3, r1 */
    { 
          EMIT("mov"  << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("imul" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

int_sub
    : X64_SUB int_type X64_CONST X64_CONST /* mov (c1 - c2), r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << cst_op_cst($1, $3, $4) << ", " << reg2str($1->resReg())) 
    } 
    | X64_SUB int_type X64_CONST X64_REG_1 /* sub c, r1; neg r1 */
    { 
          EMIT("sub" << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($4)) 
          EMIT("neg" << suffix($2) << '\t' << reg2str($4)) 
    }
    | X64_SUB int_type X64_CONST X64_REG_2 /* mov c, r1; sub r2, r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB int_type X64_REG_1 X64_CONST /* sub c, r1 */
    { 
          EMIT("sub" << suffix($2) << '\t' << cst2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_1 /* xor r1, r1 */
    { 
          EMIT("xor" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_2 /* sub r2, r1 */
    { 
          EMIT("sub" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_2 X64_CONST /* mov r2, r1; sub c, r1 */ 
    { 
          EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << '\t' << cst2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB int_type X64_REG_2 X64_REG_1 /* sub r2, r1; neg r1*/
    { 
          EMIT("sub" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($4))
          EMIT("neg" << suffix($2) << '\t' << reg2str($4)) 
    }
    | X64_SUB int_type X64_REG_2 X64_REG_2 /* xor r1, r1 */
    { 
          EMIT("xor" << suffix($2) << '\t' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB int_type X64_REG_2 X64_REG_3 /* mov r2, r1; sub r3, r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

    /*
        r1 must be RAX
        RDX must be free here

        forbidden:  r1 = c / r1
                    r1 = r2 / r1
    */
sint_no8_div
    : X64_DIV sint_no8_type X64_CONST X64_CONST /* mov 1, r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV sint_no8_type X64_CONST X64_REG_2 /* mov sg_cst(c), rdx; mov c, r1; idiv r2 */
    { 
          EMIT("mov"  << suffix($2) << '\t' << sgn_cst2str($3) << ", " << rdx2str(($2)))
          EMIT("mov"  << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($1->resReg())) 
          EMIT("idiv" << suffix($2) << '\t' << reg2str($4))
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_CONST /* mov r1, RDX; sar RDX; idiv c */
    { 
          EMIT("mov"  << suffix($2) << '\t' << reg2str($3) << ", " << rdx2str($2))
          EMIT("sar"  << suffix($2) << '\t' << sar_cst2str($2) << ", " << rdx2str($2))
          EMIT("idiv" << suffix($2) << '\t' << mcst2str($4)) 
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_REG_1 /* mov 1, r1 */
    { 
          EMIT("mov" << suffix($2) << "$1, " << reg2str($3)) 
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_REG_2 /* mov r1, RDX; sar RDX; idiv c */
    { 
          EMIT("mov"  << suffix($2) << '\t' << reg2str($3) << ", " << rdx2str($2))
          EMIT("sar"  << suffix($2) << '\t' << sar_cst2str($2) << ", " << rdx2str($2))
          EMIT("idiv" << suffix($2) << '\t' << reg2str($4)) 
    }
    | X64_DIV sint_no8_type X64_REG_2 X64_CONST /* mov r2, r1; mov r2, rdx; sar RDX; idiv c */
    { 
          EMIT("mov"  << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("mov"  << suffix($2) << '\t' << reg2str($3) << ", " << rdx2str($2))
          EMIT("sar"  << suffix($2) << '\t' << sar_cst2str($2) << ", " << rdx2str($2))
          EMIT("idiv" << suffix($2) << '\t' << mcst2str($4)) 
    }
    | X64_DIV sint_no8_type X64_REG_2 X64_REG_2 /* mov $1, r1 */
    { 
          EMIT("imov" << suffix($2) << " $1, " << reg2str($1->resReg()))
    }
    | X64_DIV sint_no8_type X64_REG_2 X64_REG_3 /* mov r2, r1; mov r2, rdx; sar rdx; div r2 */
    { 
          EMIT("mov"  << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("mov"  << suffix($2) << '\t' << reg2str($3) << ", " << rdx2str($2))
          EMIT("sar"  << suffix($2) << '\t' << sar_cst2str($2) << ", " << rdx2str($2))
          EMIT("idiv" << suffix($2) << '\t' << reg2str($4)) 
    }
    ;

    /*
        r1 must be RAX
        RDX must be free here

        forbidden:  r1 = c / r1
                    r1 = r2 / r1
    */
uint_no8_div
    : X64_DIV uint_no8_type X64_CONST X64_CONST /* mov 1, r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV uint_no8_type X64_CONST X64_REG_2 /* xor rdx, rdx; mov c, r1; div r2 */
    { 
          EMIT("xor" << suffix($2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT("mov" << suffix($2) << '\t' << cst2str($3) << ", " << reg2str($1->resReg())) 
          EMIT("div" << suffix($2) << '\t' << reg2str($4))
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_CONST /* xor rdx, rdx; div c */
    { 
          EMIT("xor" << suffix($2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT("div" << suffix($2) << '\t' << mcst2str($4)) 
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_REG_1 /* mov 1, r1 */
    { 
          EMIT("mov" << suffix($2) << "$1, " << reg2str($3)) 
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_REG_2 /* xor rdx, rdx; div c */
    { 
          EMIT("xor" << suffix($2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT("div" << suffix($2) << '\t' << reg2str($4)) 
    }
    | X64_DIV uint_no8_type X64_REG_2 X64_CONST /* mov r2, r1; xor rdx, rdx; div c */
    { 
          EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("xor" << suffix($2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT("div" << suffix($2) << '\t' << mcst2str($4)) 
    }
    | X64_DIV uint_no8_type X64_REG_2 X64_REG_2 /* mov $1, r1 */
    { 
          EMIT("mov" << suffix($2) << " $1, " << reg2str($1->resReg()))
    }
    | X64_DIV uint_no8_type X64_REG_2 X64_REG_3 /* mov r2, r1; xor rdx , rdx; div r2 */
    { 
          EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("xor" << suffix($2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT("div" << suffix($2) << '\t' << reg2str($4)) 
    }
    ;

int_cmp
    : cmp int_type X64_CONST X64_CONST /* mov true, r1 or mov flase, r1 */
    { 
          EMIT("movb\t" << cst_op_cst($1, $3, $4) << ", " << reg2str($1->resReg())) 
    }
    | cmp int_type X64_CONST any_reg /* cmp r, c */
    { 
          EMIT("cmp" << suffix($2) << '\t' << reg2str($4) << ", " << cst2str($3))
          if ($1->resReg()->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1, $2) << "b\t" << reg2str($1->resReg())) 
    }
    | cmp int_type any_reg X64_CONST /* cmp r, c */
    { 
          EMIT("cmp" << suffix($2) << '\t' << cst2str($4) << ", " << reg2str($3))
          if ($1->resReg()->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1, $2) << "b\t" << reg2str($1->resReg())) 
    }
    | cmp int_type any_reg any_reg   /* cmp r, r */
    { 
          EMIT("cmp" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($3))
          if ($1->resReg()->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1, $2) << "b\t" << reg2str($1->resReg())) 
    }
    ;
    
real_mov
    : X64_MOV real_type X64_UNDEF 
    { 
        /* emit no code: mov X64_UNDEF, %r1 */ 
    }
    | X64_MOV real_type X64_CONST 
    { 
        EMIT("mov" << suffix($2) << '\t' << mcst2str($3) << ", " << reg2str($1->resReg())) 
    }
    | X64_MOV real_type X64_REG_1 
    { 
        /* emit no code: mov %r1, %r1 */ 
    }
    | X64_MOV real_type X64_REG_2 
    { 
        EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    ;

real_add_mul
    : add_or_mul real_type X64_CONST X64_CONST /* mov (c1 + c2), r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | add_or_mul real_type X64_CONST X64_REG_1 /* add c, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << '\t' << mcst2str($3) << ", " << reg2str($4)) 
    }
    | add_or_mul real_type X64_CONST X64_REG_2 /* mov c, r1; add r2, r1 */ 
    { 
          EMIT("mov" << suffix($2) << '\t' << mcst2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | add_or_mul real_type X64_REG_1 X64_CONST /* add c, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << '\t' << mcst2str($4) << ", " << reg2str($3)) 
    }
    | add_or_mul real_type X64_REG_1 X64_REG_1 /* add r1, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | add_or_mul real_type X64_REG_1 X64_REG_2 /* add r2, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | add_or_mul real_type X64_REG_2 X64_CONST /* mov c, r1; add r2, r1 */ 
    { 
          EMIT("mov" << suffix($2) << '\t' << mcst2str($4) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg())) 
    }
    | add_or_mul real_type X64_REG_2 X64_REG_1 /* add r2, r1 */
    { 
          EMIT(instr2str($1) << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($4)) 
    }
    | add_or_mul real_type X64_REG_2 X64_REG_2 /* mov r2, r1; add r1, r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << '\t' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) 
    }
    | add_or_mul real_type X64_REG_2 X64_REG_3 /* mov r2, r1; add r3, r1 */
    { 
          EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

real_sub
    : X64_SUB real_type X64_CONST X64_CONST /* mov (c1 - c2), r1 */
    {
        EMIT("mov" << suffix($2) << '\t' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB real_type X64_CONST X64_REG_1 /* sub c, r1; xor signmask, r1 */
    {
        EMIT(instr2str($1) << suffix($2) << '\t' << mcst2str($3) << ", " << reg2str($4))  
        EMIT("xor" << suffix($2) << '\t' << neg_mask($2) << ", " << reg2str($4)) 
    }
    | X64_SUB real_type X64_CONST X64_REG_2 /* mov c, r1; sub r2, r1 */ 
    {
        EMIT("mov" << suffix($2) << '\t' << mcst2str($3) << ", " << reg2str($1->resReg()))
        EMIT("sub" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB real_type X64_REG_1 X64_CONST /* sub c, r1 */
    {
        EMIT("sub" << suffix($2) << '\t' << mcst2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB real_type X64_REG_1 X64_REG_1 /* xor r1, r1 */
    {
        EMIT("xor" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_SUB real_type X64_REG_1 X64_REG_2 /* sub r2, r1 */
    {
        EMIT("sub" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB real_type X64_REG_2 X64_CONST /* mov r2, r1; sub c, r1 */ 
    {
        EMIT("mov" << suffix($2) << '\t' <<  reg2str($3) << ", " << reg2str($1->resReg()))
        EMIT("sub" << suffix($2) << '\t' << mcst2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB real_type X64_REG_2 X64_REG_1 /* sub r2, r1 */
    {
        EMIT("sub" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($4)) 
    }
    | X64_SUB real_type X64_REG_2 X64_REG_2 /* xor r1, r1 */
    {
        EMIT("xor" << suffix($2) << '\t' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) 
    }
    | X64_SUB real_type X64_REG_2 X64_REG_3 /* mov r2, r1; sub r3, r1 */
    {
        EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
        EMIT("sub" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

    /*
        forbidden:  r1 = c / r1
                    r1 = r2 / r1 
    */
real_div
    : X64_DIV real_type X64_CONST X64_CONST /* mov (c1 / c2), r1 */
    {
        EMIT("mov" << suffix($2) << '\t' << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV real_type X64_CONST X64_REG_2 /* mov c, r1; div r2, r1 */ 
    {
        EMIT("mov" << suffix($2) << '\t' << mcst2str($3) << ", " << reg2str($1->resReg()))
        EMIT("div" << suffix($2) << '\t' <<  reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV real_type X64_REG_1 X64_CONST /* div c, r1 */
    {
        EMIT("div" << suffix($2) << '\t' << mcst2str($4) << ", " << reg2str($3)) 
    }
    | X64_DIV real_type X64_REG_1 X64_REG_1 /* load with 1 */
    {
        swiftAssert(false, "TODO");
    }
    | X64_DIV real_type X64_REG_1 X64_REG_2 /* div r2, r1 */
    {
        EMIT("div" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_DIV real_type X64_REG_2 X64_CONST /* mov r2, r1; div c, r1 */ 
    {
        EMIT("mov" << suffix($2) << '\t' <<  reg2str($3) << ", " << reg2str($1->resReg()))
        EMIT("div" << suffix($2) << '\t' << mcst2str($4) << ", " << reg2str($1->resReg())) 
    }
    | X64_DIV real_type X64_REG_2 X64_REG_2 /* load with 1 */
    {
        swiftAssert(false, "TODO");
    }
    | X64_DIV real_type X64_REG_2 X64_REG_3 /* mov r2, r1; div r3, r1 */
    {
        EMIT("mov" << suffix($2) << '\t' << reg2str($3) << ", " << reg2str($1->resReg()))
        EMIT("div" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($1->resReg())) 
    }
    ;

real_cmp
    : cmp real_type X64_CONST X64_CONST /* mov true, r1 or mov flase, r1 */
    { 
        EMIT("movb\t" << cst_op_cst($1, $3, $4, true) << ", " << reg2str($1->resReg())) 
    }
    | cmp real_type any_reg X64_CONST /* cmp c, r */
    { 
        EMIT("ucomi" << suffix($2) << '\t' << mcst2str($4) << ", " << reg2str($3))
        if ($1->resReg()->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1, $2) << "b\t" << reg2str($1->resReg())) 
    }
    | cmp real_type X64_CONST any_reg /* cmpn c, r */
    { 
        EMIT("ucomi" << suffix($2) << '\t' << mcst2str($3) << ", " << reg2str($4))
        if ($1->resReg()->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1, $2, true) << "b\t" << reg2str($1->resReg())) 
    }
    | cmp real_type any_reg any_reg   /* cmp r, r */
    { 
        EMIT("ucomi" << suffix($2) << '\t' << reg2str($4) << ", " << reg2str($3))
        if ($1->resReg()->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1, $2) << "b\t" << reg2str($1->resReg())) 
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

sint_no8_type
    : X64_INT16 { $$ = X64_INT16; }
    | X64_INT32 { $$ = X64_INT32; }
    | X64_INT64 { $$ = X64_INT64; }
    ;

uint_no8_type
    : X64_UINT16 { $$ = X64_UINT16; }
    | X64_UINT32 { $$ = X64_UINT32; }
    | X64_UINT64 { $$ = X64_UINT64; }
    ;

int8_type
    : X64_INT8  { $$ = X64_INT8;  }
    | X64_UINT8 { $$ = X64_UINT8; }
    ;


real_type
    : X64_REAL32 { $$ = X64_REAL32; }
    | X64_REAL64 { $$ = X64_REAL64; } 
    ;

int_type
    : sint_no8_type { $$ = $1; }
    | uint_no8_type { $$ = $1; }
    | int8_type     { $$ = $1; }
    ;

int_or_bool_type
    : int_type  { $$ = $1; }
    | bool_type { $$ = $1; }
    ;

any_type
    : int_or_bool_type { $$ = $1; }
    | real_type        { $$ = $1; }
    ;

any_reg
    : X64_REG_1 { $$ = $1; }
    | X64_REG_2 { $$ = $1; }
    | X64_REG_3 { $$ = $1; }
    ;

%%

// get rid of local define
#undef EMIT

