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
#include "be/x64lexer.h"

#include <iostream>
using namespace be;

/*
    macro magic for less verbose code emitting
*/

#define EMIT(e) *x64_ofs << '\t' << e << '\n';

%}

%union
{
    int int_;

    me::Const*  const_;
    me::MemVar* memVar_;
    me::Reg*    reg_;
    me::Undef*  undef_;

    me::AssignInstr* assign_;
    me::BranchInstr* branch_;
    me::CallInstr*   call_;
    me::GotoInstr*   goto_;
    me::LabelInstr*  label_;
    me::Load*        load_;
    me::LoadPtr*     loadPtr_;
    me::Reload*      reload_;
    me::Spill*       spill_;
    me::Store*       store_;
};

/*
    tokens
*/

/* instructions */
%token <assign_> X64_DEREF
%token <assign_> X64_EQ X64_NE X64_L X64_LE X64_G X64_GE
%token <assign_> X64_MOV X64_ADD X64_SUB X64_MUL X64_DIV X64_AND X64_ANDN X64_OR X64_XOR X64_NOT
%token <assign_> X64_UN_MINUS
%token <branch_> X64_BRANCH X64_BRANCH_TRUE X64_BRANCH_FALSE
%token <call_>   X64_CALL
%token <goto_>   X64_GOTO
%token <label_>  X64_LABEL
%token <loadPtr_> X64_LOAD_PTR
%token <load_>   X64_LOAD
%token <reload_> X64_RELOAD
%token <spill_>  X64_SPILL
%token <store_>  X64_STORE
%token X64_NOP

/* types */
%token X64_BOOL 
%token  X64_INT8  X64_INT16  X64_INT32  X64_INT64  X64_SAT8  X64_SAT16
%token X64_UINT8 X64_UINT16 X64_UINT32 X64_UINT64 X64_USAT8 X64_USAT16 
%token X64_REAL32 X64_REAL64
%token X64_STACK

/* simd types */
%token  X64_S_INT8  X64_S_INT16  X64_S_INT32  X64_S_INT64  X64_S_SAT8  X64_S_SAT16
%token X64_S_UINT8 X64_S_UINT16 X64_S_UINT32 X64_S_UINT64 X64_S_USAT8 X64_S_USAT16 
%token X64_S_REAL32 X64_S_REAL64

/* operands */
%token <undef_>  X64_UNDEF
%token <const_>  X64_CONST X64_CST_0 X64_CST_1
%token <reg_>    X64_REG_SPILLED X64_REG_1 X64_REG_2 X64_REG_3 X64_REG_4
%token <memVar_> X64_MEM_VAR

/*
    types
*/

%type <int_> bool_type int_type sint_no8_type sint_type uint_no8_type int8_type real_type simd_type real_simd_type int_or_bool_type any_type
%type <assign_> add_or_and commutative cmp
%type <reg_> any_reg

%start instruction

%% 

instruction
    : X64_LABEL { *x64_ofs << $1->asmName() << ":\n"; }
    | X64_NOP { /* do nothing */ }
    | jump_instruction
    | assign_instruction
    | spill_reload
    | load_store
    | load_ptr
    | deref
    | call
    ;

jump_instruction
    : X64_GOTO 
    { 
        EMIT("jmp\t" << $1->label()->asmName()) 
    }
    | X64_BRANCH bool_type X64_CONST 
    {   
        if ($3->box().bool_) 
            EMIT("jmp\t" << $1->trueLabel()->asmName())
        else
            EMIT("jmp\t" << $1->falseLabel()->asmName())
    }
    | X64_BRANCH bool_type X64_REG_1 /* test r1, r1; jnz trueLabel; jmp falseLabel */
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
        if ($3->box().bool_) 
            EMIT("jmp\t" << $1->trueLabel()->asmName())
    }
    | X64_BRANCH_TRUE bool_type X64_REG_1 
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
        if (!$3->box().bool_) 
            EMIT("jmp\t" << $1->falseLabel()->asmName())
    }
    | X64_BRANCH_FALSE bool_type X64_REG_1 
    { 
        if ($1->cc_ == me::BranchInstr::CC_NOT_SET)
        {
            EMIT("testb\t" << reg2str($3) << ", " << reg2str($3))
            EMIT("jz\t"  << $1->falseLabel()->asmName())
        }
        else
            EMIT("j" << jcc($1, true) << '\t' << $1->falseLabel()->asmName())
    }
    | X64_BRANCH simd_type X64_REG_1 /* movmsk r1, reg32; test r32, r32; jnz trueLabel; jmp falseLabel */
    { 
        EMIT(mnemonic("movmsk", $2) << '\t' << reg2str($3) << ", " << reg2str($1->getMask()))
        EMIT("testl\t" << reg2str($1->getMask()) << ", " << reg2str($1->getMask()))
        EMIT("jnz\t" << $1->trueLabel()->asmName())
        EMIT("jmp\t" << $1->falseLabel()->asmName()) 
    }
    | X64_BRANCH_TRUE simd_type X64_REG_1 
    { 
        EMIT(mnemonic("movmsk", $2) << '\t' << reg2str($3) << ", " << reg2str($1->getMask()))
        EMIT("testl\t" << reg2str($1->getMask()) << ", " << reg2str($1->getMask()))
        EMIT("jnz\t" << $1->trueLabel()->asmName())
    }
    | X64_BRANCH_FALSE simd_type X64_REG_1 
    { 
        EMIT(mnemonic("movmsk", $2) << '\t' << reg2str($3) << ", " << reg2str($1->getMask()))
        EMIT("testl\t" << reg2str($1->getMask()) << ", " << reg2str($1->getMask()))
        EMIT("jz\t" << $1->falseLabel()->asmName())
    }
    ;

call
    : X64_CALL
    {
        EMIT("call\t" << $1->symbol_)
    }
    ;

spill_reload
    : X64_SPILL any_type X64_REG_SPILLED X64_REG_1
    {
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
    }
    | X64_RELOAD any_type X64_REG_1 X64_REG_SPILLED
    {
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
    }
    ;

load_store
    : X64_LOAD any_type X64_REG_1 X64_MEM_VAR
    { 
        EMIT(mnemonic("mov", $2) << '\t' << memvar2str($4, $1->getOffset()) << ", " << reg2str($3)) 
    }
    | X64_LOAD any_type X64_REG_1 any_reg
    { 
        EMIT(mnemonic("mov", $2) << '\t' << ptr2str($4, $1->getOffset()) << ", " << reg2str($3))
    }
    | X64_LOAD any_type X64_REG_1 X64_MEM_VAR any_reg
    { 
        EMIT(mnemonic("mov", $2) << '\t' << memvar_index2str($4, $5, $1->getOffset()) << ", " << reg2str($3)) 
    }
    | X64_LOAD any_type X64_REG_1 any_reg any_reg
    { 
        EMIT(mnemonic("mov", $2) << '\t' << ptr_index2str($4, $5, $1->getOffset()) << ", " << reg2str($3))
    }
    | X64_STORE any_type any_reg X64_MEM_VAR
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($3) << ", " << memvar2str($4, $1->getOffset()))
    }
    | X64_STORE any_type any_reg any_reg
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($3) << ", " << ptr2str($4, $1->getOffset()))
    }
    | X64_STORE any_type any_reg X64_MEM_VAR any_reg
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($3) << ", " << memvar_index2str($4, $5, $1->getOffset()))
    }
    | X64_STORE any_type any_reg any_reg any_reg
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($3) << ", " << ptr_index2str($4, $5, $1->getOffset()))
    }
    ;

load_ptr
    : X64_LOAD_PTR X64_REG_1 X64_MEM_VAR
    { 
        EMIT("leaq\t" << memvar2str($3, $1->getOffset()) << ", " << reg2str($2))
    }
    ;

deref
    : X64_DEREF any_type X64_REG_1 any_reg
    {
        EMIT(mnemonic("mov", $2) << "\t(" << reg2str($4) << "), " << reg2str($3))
    }

assign_instruction
    : int_mov
    | int_add_and
    | int_mul
    | int_sub
    | sint_un_minus
    | sint_no8_div
    | uint_no8_div
    | int_cmp
    | int_not
    | real_simd_mov
    | real_simd_commutative 
    | real_simd_sub
    | real_simd_un_minus
    | real_simd_div
    | real_simd_andn
    | real_cmp
    | simd_cmp
    | simd_not
    ;

int_mov
    : X64_MOV int_or_bool_type X64_REG_1 X64_UNDEF 
    { 
        /* emit no code: mov X64_UNDEF, %r1 */ 
    }
    | X64_MOV int_or_bool_type X64_REG_1 X64_CONST 
    { 
        EMIT(mnemonic("mov", $2) << '\t' << cst2str($4) << ", " << reg2str($3)) 
    }
    | X64_MOV int_or_bool_type X64_REG_1 X64_REG_1 
    { 
        /* emit no code: mov %r1, %r1 */ 
    }
    | X64_MOV int_or_bool_type X64_REG_1 X64_REG_2 
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    ;

int_add_and
    : add_or_and int_type X64_REG_1 X64_CONST X64_CONST /* mov (c1 + c2), r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << cst_op_cst($1, $4, $5) << ", " << reg2str($3)) 
    } 
    | add_or_and int_type X64_REG_1 X64_CONST X64_REG_1 /* add c, r1 */
    {
        EMIT(mnemonic(instr2str($1), $2) << '\t' << cst2str($4) << ", " << reg2str($5)) 
    }
    | add_or_and int_type X64_REG_1 X64_CONST X64_REG_2 /* mov c, r1; add r2, r1 */ 
    {
        EMIT(mnemonic("mov", $2) << '\t' << cst2str($4) << ", " << reg2str($3))
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | add_or_and int_type X64_REG_1 X64_REG_1 X64_CONST /* add c, r1 */
    {
        EMIT(mnemonic(instr2str($1), $2) << '\t' << cst2str($5) << ", " << reg2str($3)) 
    }
    | add_or_and int_type X64_REG_1 X64_REG_1 X64_REG_1 /* shl 2, r2 */
    { 
        if ($1->kind_ == '+')
            EMIT(mnemonic("shl", $2) << " $2, " << reg2str($3))
        /* else -> do nothing */
    }
    | add_or_and int_type X64_REG_1 X64_REG_1 X64_REG_2 /* add r2, r1 */
    { 
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | add_or_and int_type X64_REG_1 X64_REG_2 X64_CONST /* mov c, r1; add r2, r1 */ 
    { 
        EMIT(mnemonic("mov", $2) << '\t' << cst2str($5) << ", " << reg2str($3))
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | add_or_and int_type X64_REG_1 X64_REG_2 X64_REG_1 /* add r2, r1 */
    { 
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | add_or_and int_type X64_REG_1 X64_REG_2 X64_REG_2 /* mov r2, r1; shl 2, r1 */
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        if ($1->kind_ == '+')
            EMIT(mnemonic("shl", $2) << " $2,  " << reg2str($3)) 
        /* else -> do nothing */
    }
    | add_or_and int_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; add r3, r1 */
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    ;

int_mul
    : X64_MUL int_type X64_REG_1 X64_CONST X64_CONST /* mov (c1 * c2), r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << cst_op_cst($1, $4, $5) << ", " << reg2str($3)) 
    } 
    | X64_MUL int_type X64_REG_1 X64_CONST X64_REG_1 /* mul c, r1 */
    {
        EMIT(mnemonic("imul", $2) << '\t' << cst2str($4) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_CONST X64_REG_2 /* mov c, r1; mul r2, r1 */ 
    {
        EMIT(mnemonic("mov", $2) << '\t' << cst2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("imul", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_1 X64_CONST /* mul c, r1 */
    {
        EMIT(mnemonic("imul", $2) << '\t' << cst2str($5) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_1 X64_REG_1 /* mul r1, r1 */
    { 
        EMIT(mnemonic("imul", $2) << '\t' << reg2str($3) << ", " << reg2str($3))
    }
    | X64_MUL int_type X64_REG_1 X64_REG_1 X64_REG_2 /* mul r2, r1 */
    { 
        EMIT(mnemonic("imul", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_2 X64_CONST /* mov c, r1; mul r2, r1 */ 
    { 
        EMIT(mnemonic("mov", $2) << '\t' << cst2str($5) << ", " << reg2str($3))
        EMIT(mnemonic("imul", $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_2 X64_REG_1 /* mul r2, r1 */
    { 
        EMIT(mnemonic("imul", $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_2 X64_REG_2 /* mov r2, r1; mul r2, r1 */
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("imul", $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_MUL int_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; mul r3, r1 */
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("imul", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    ;

int_sub
    : X64_SUB int_type X64_REG_1 X64_CONST X64_CONST /* mov (c1 - c2), r1 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << cst_op_cst($1, $4, $5) << ", " << reg2str($3)) 
    } 
    | X64_SUB int_type X64_REG_1 X64_CONST X64_REG_1 /* sub c, r1; neg r1 */
    { 
          EMIT(mnemonic("sub", $2) << '\t' << cst2str($4) << ", " << reg2str($3)) 
          EMIT(mnemonic("neg", $2) << '\t' << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_CONST X64_REG_2 /* mov c, r1; sub r2, r1 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << cst2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("sub", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_1 X64_CONST /* sub c, r1 */
    { 
          EMIT(mnemonic("sub", $2) << '\t' << cst2str($5) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_1 X64_REG_1 /* xor r1, r1 */
    { 
          EMIT(mnemonic("xor", $2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_1 X64_REG_2 /* sub r2, r1 */
    { 
          EMIT(mnemonic("sub", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_2 X64_CONST /* mov r2, r1; sub c, r1 */ 
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("sub", $2) << '\t' << cst2str($5) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_2 X64_REG_1 /* sub r2, r1; neg r1*/
    { 
          EMIT(mnemonic("sub", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("neg", $2) << '\t' << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_2 X64_REG_2 /* xor r1, r1 */
    { 
          EMIT(mnemonic("xor", $2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_SUB int_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; sub r3, r1 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("sub", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    ;

sint_un_minus
    : X64_UN_MINUS sint_type X64_REG_1 X64_CONST /* mov -c, r1 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << un_minus_cst($4) << ", " << reg2str($3)) 
    } 
    | X64_UN_MINUS sint_type X64_REG_1 X64_REG_1 /* neg r1 */
    { 
          EMIT(mnemonic("neg", $2) << '\t' << reg2str($3)) 
    }
    | X64_UN_MINUS sint_type X64_REG_1 X64_REG_2 /* mov r2, r1; neg r1 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("neg", $2) << '\t' << reg2str($3)) 
    }
    ;

    /*
        r1 must be RAX
        RDX must be free here

        forbidden:  r1 = c / r1
                    r1 = r2 / r1
    */
sint_no8_div
    : X64_DIV sint_no8_type X64_REG_1 X64_CONST X64_CONST /* mov 1, r1 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << cst_op_cst($1, $4, $5, true) << ", " << reg2str($3)) 
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_CONST X64_REG_2 /* mov sgn_cst(c), rdx; mov c, r1; idiv r2 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << sgn_cst2str($4) << ", " << rdx2str(($2)))
          EMIT(mnemonic("mov", $2) << '\t' << cst2str($4) << ", " << reg2str($3)) 
          EMIT(mnemonic("idiv", $2) << '\t' << reg2str($5))
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_REG_1 X64_CONST /* mov r1, RDX; sar RDX; idiv c */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($3) << ", " << rdx2str($2))
          EMIT(mnemonic("sar", $2) << '\t' << sar_cst2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("idiv", $2) << '\t' << mcst2str($5)) 
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_REG_1 X64_REG_1 /* mov 1, r1 */
    { 
          EMIT(mnemonic("mov", $2) << "$1, " << reg2str($3)) 
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_REG_1 X64_REG_2 /* mov r1, RDX; sar RDX; idiv c */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($3) << ", " << rdx2str($2))
          EMIT(mnemonic("sar", $2) << '\t' << sar_cst2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("idiv", $2) << '\t' << reg2str($5)) 
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_REG_2 X64_CONST /* mov r2, r1; mov r2, rdx; sar RDX; idiv c */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << rdx2str($2))
          EMIT(mnemonic("sar", $2) << '\t' << sar_cst2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("idiv", $2) << '\t' << mcst2str($5)) 
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_REG_2 X64_REG_2 /* mov $1, r1 */
    { 
          EMIT(mnemonic("imov", $2) << " $1, " << reg2str($3))
    }
    | X64_DIV sint_no8_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; mov r2, rdx; sar rdx; div r3 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << rdx2str($2))
          EMIT(mnemonic("sar", $2) << '\t' << sar_cst2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("idiv", $2) << '\t' << reg2str($5)) 
    }
    ;

    /*
        r1 must be RAX
        RDX must be free here

        forbidden:  r1 = c / r1
                    r1 = r2 / r1
    */
uint_no8_div
    : X64_DIV uint_no8_type X64_REG_1 X64_CONST X64_CONST /* mov 1, r1 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << cst_op_cst($1, $4, $5, true) << ", " << reg2str($3)) 
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_CONST X64_REG_2 /* xor rdx, rdx; mov c, r1; div r2 */
    { 
          EMIT(mnemonic("xor", $2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("mov", $2) << '\t' << cst2str($4) << ", " << reg2str($3)) 
          EMIT(mnemonic("div", $2) << '\t' << reg2str($5))
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_REG_1 X64_CONST /* xor rdx, rdx; div c */
    { 
          EMIT(mnemonic("xor", $2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("div", $2) << '\t' << mcst2str($5)) 
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_REG_1 X64_REG_1 /* mov 1, r1 */
    { 
          EMIT(mnemonic("mov", $2) << "$1, " << reg2str($3)) 
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_REG_1 X64_REG_2 /* xor rdx, rdx; div r2 */
    { 
          EMIT(mnemonic("xor", $2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("div", $2) << '\t' << reg2str($5)) 
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_REG_2 X64_CONST /* mov r2, r1; xor rdx, rdx; div c */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("xor", $2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("div", $2) << '\t' << mcst2str($5)) 
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_REG_2 X64_REG_2 /* mov $1, r1 */
    { 
          EMIT(mnemonic("mov", $2) << " $1, " << reg2str($3))
    }
    | X64_DIV uint_no8_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; xor rdx , rdx; div r2 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("xor", $2) << '\t' << rdx2str($2) << ", " << rdx2str($2))
          EMIT(mnemonic("div", $2) << '\t' << reg2str($5)) 
    }
    ;

int_cmp /* TODO any_reg */
    : cmp int_type X64_REG_1 X64_CONST X64_CONST /* mov true, r1 or mov flase, r1 */
    { 
          EMIT("movb\t" << cst_op_cst($1, $4, $5) << ", " << reg2str($3)) 
    }
    | cmp int_type X64_REG_1 X64_CONST any_reg /* cmp r, c */
    { 
          EMIT(mnemonic("cmp", $2) << '\t' << reg2str($5) << ", " << cst2str($4))
          if ($3->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1) << "b\t" << reg2str($3)) 
    }
    | cmp int_type X64_REG_1 any_reg X64_CONST /* cmp r, c */
    { 
          EMIT(mnemonic("cmp", $2) << '\t' << cst2str($5) << ", " << reg2str($4))
          if ($3->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1) << "b\t" << reg2str($3)) 
    }
    | cmp int_type X64_REG_1 any_reg any_reg   /* cmp r, r */
    { 
          EMIT(mnemonic("cmp", $2) << '\t' << reg2str($5) << ", " << reg2str($4))
          if ($3->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1) << "b\t" << reg2str($3)) 
    }
    ;
    
real_simd_mov
    : X64_MOV real_simd_type X64_REG_1 X64_UNDEF 
    { 
        /* emit no code: mov X64_UNDEF, %r1 */ 
    }
    | X64_MOV real_simd_type X64_REG_1 X64_CONST 
    { 
        EMIT(mnemonic("mov", $2) << '\t' << mcst2str($4) << ", " << reg2str($3)) 
    }
    | X64_MOV real_simd_type X64_REG_1 X64_REG_1 
    { 
        /* emit no code: mov %r1, %r1 */ 
    }
    | X64_MOV real_simd_type X64_REG_1 X64_REG_2 
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    ;

real_simd_commutative
    : commutative real_simd_type X64_REG_1 X64_CONST X64_CONST /* mov (c1 + c2), r1 */
    { 
        EMIT(mnemonic("mov", $2) << '\t' << cst_op_cst($1, $4, $5, true) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_CONST X64_REG_1 /* add c, r1 */
    { 
        EMIT(mnemonic(instr2str($1), $2) << '\t' << mcst2str($4) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_CONST X64_REG_2 /* mov c, r1; add r2, r1 */ 
    { 
        EMIT(mnemonic("mov", $2) << '\t' << mcst2str($4) << ", " << reg2str($3))
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_REG_1 X64_CONST /* add c, r1 */
    { 
        EMIT(mnemonic(instr2str($1), $2) << '\t' << mcst2str($5) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_REG_1 X64_REG_1 /* add r1, r1 */
    { 
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_REG_1 X64_REG_2 /* add r2, r1 */
    { 
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_REG_2 X64_CONST /* mov c, r1; add r2, r1 */ 
    { 
        EMIT(mnemonic("mov", $2) << '\t' << mcst2str($5) << ", " << reg2str($3))
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_REG_2 X64_REG_1 /* add r2, r1 */
    { 
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_REG_2 X64_REG_2 /* mov r2, r1; add r1, r1 */
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | commutative real_simd_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; add r3, r1 */
    { 
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic(instr2str($1), $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    ;

real_simd_sub
    : X64_SUB real_simd_type X64_REG_1 X64_CONST X64_CONST /* mov (c1 - c2), r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << cst_op_cst($1, $4, $5, true) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_CONST X64_REG_1 /* sub c, r1; xor signmask, r1 */
    {
        EMIT(mnemonic("sub", $2) << '\t' << mcst2str($4) << ", " << reg2str($3))  
        EMIT(mnemonic("xor", $2) << '\t' << neg_mask($2, true) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_CONST X64_REG_2 /* mov c, r1; sub r2, r1 */ 
    {
        EMIT(mnemonic("mov", $2) << '\t' << mcst2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("sub", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_REG_1 X64_CONST /* sub c, r1 */
    {
        EMIT(mnemonic("sub", $2) << '\t' << mcst2str($5) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_REG_1 X64_REG_1 /* xor r1, r1 */
    {
        EMIT(mnemonic("xor", $2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_REG_1 X64_REG_2 /* sub r2, r1 */
    {
        EMIT(mnemonic("sub", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_REG_2 X64_CONST /* mov r2, r1; sub c, r1 */ 
    {
        EMIT(mnemonic("mov", $2) << '\t' <<  reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("sub", $2) << '\t' << mcst2str($5) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_REG_2 X64_REG_1 /* sub r2, r1 */
    {
        EMIT(mnemonic("sub", $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_REG_2 X64_REG_2 /* xor r1, r1 */
    {
        EMIT(mnemonic("xor", $2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_SUB real_simd_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; sub r3, r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("sub", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    ;

real_simd_un_minus
    : X64_UN_MINUS real_simd_type X64_REG_1 X64_CONST /* mov -c, r1 */
    { 
          EMIT(mnemonic("mov", $2) << '\t' << un_minus_cst($4, true) << ", " << reg2str($3)) 
    } 
    | X64_UN_MINUS real_simd_type X64_REG_1 X64_REG_1 /* xor neg_mask, r1 */
    { 
          EMIT(mnemonic("xor", $2) << '\t' << neg_mask($2, true) << ", " << reg2str($3)) 
    }
    | X64_UN_MINUS real_simd_type X64_REG_1 X64_REG_2 /* mov r2, r1; xor neg_mask, r1 */ 
    { 
          EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
          EMIT(mnemonic("xor", $2) << '\t' << neg_mask($2, true) << ", " << reg2str($3)) 
    }
    ;

    /*
        forbidden:  r1 = c / r1
                    r1 = r2 / r1 
    */
real_simd_div
    : X64_DIV real_simd_type X64_REG_1 X64_CONST X64_CONST /* mov (c1 / c2), r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << cst_op_cst($1, $4, $5, true) << ", " << reg2str($3)) 
    }
    | X64_DIV real_simd_type X64_REG_1 X64_CONST X64_REG_2 /* mov c, r1; div r2, r1 */ 
    {
        EMIT(mnemonic("mov", $2) << '\t' << mcst2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("div", $2) << '\t' <<  reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_DIV real_simd_type X64_REG_1 X64_REG_1 X64_CONST /* div c, r1 */
    {
        EMIT(mnemonic("div", $2) << '\t' << mcst2str($5) << ", " << reg2str($3)) 
    }
    | X64_DIV real_simd_type X64_REG_1 X64_REG_1 X64_REG_1 /* load with 1 */
    {
        swiftAssert(false, "TODO");
    }
    | X64_DIV real_simd_type X64_REG_1 X64_REG_1 X64_REG_2 /* div r2, r1 */
    {
        EMIT(mnemonic("div", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_DIV real_simd_type X64_REG_1 X64_REG_2 X64_CONST /* mov r2, r1; div c, r1 */ 
    {
        EMIT(mnemonic("mov", $2) << '\t' <<  reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("div", $2) << '\t' << mcst2str($5) << ", " << reg2str($3)) 
    }
    | X64_DIV real_simd_type X64_REG_1 X64_REG_2 X64_REG_2 /* load with 1 */
    {
        swiftAssert(false, "TODO");
    }
    | X64_DIV real_simd_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; div r3, r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("div", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    ;

    /*
        forbidden
            - r1 = andn(r2, r1)
    */
real_simd_andn
    : X64_ANDN real_simd_type X64_REG_1 X64_CONST X64_CONST /* TODO */
    {
        swiftAssert(false, "TODO");
    }
    | X64_ANDN real_simd_type X64_REG_1 X64_CONST X64_REG_1 /* and neg(c), r1 */
    {
        EMIT(mnemonic("and", $2) << '\t' << neg_cst($4, true) << ", " << reg2str($3)) 
    }
    | X64_ANDN real_simd_type X64_REG_1 X64_CONST X64_REG_2 /* mov r2, r1; and neg(c), r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' <<       reg2str($5) << ", " << reg2str($3))
        EMIT(mnemonic("and", $2) << '\t' << neg_cst($4, true) << ", " << reg2str($3)) 
    }
    | X64_ANDN real_simd_type X64_REG_1 X64_REG_1 X64_CONST /* andn c, r1 */
    {
        EMIT(mnemonic("andn", $2) << '\t' << mcst2str($5) << ", " << reg2str($3)) 
    }
    | X64_ANDN real_simd_type X64_REG_1 X64_REG_1 X64_REG_1 /* andn r1, r1 */
    {
        EMIT(mnemonic("andn", $2) << '\t' << reg2str($3) << ", " << reg2str($3)) 
    }
    | X64_ANDN real_simd_type X64_REG_1 X64_REG_1 X64_REG_2 /* andn r2, r1 */
    {
        EMIT(mnemonic("andn", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    | X64_ANDN real_simd_type X64_REG_1 X64_REG_2 X64_CONST /* mov r2, r1; andn c, r1 */ 
    {
        EMIT(mnemonic( "mov", $2) << '\t' <<  reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("andn", $2) << '\t' << mcst2str($5) << ", " << reg2str($3)) 
    }
    | X64_ANDN real_simd_type X64_REG_1 X64_REG_2 X64_REG_2 /* mov r2, r1; andn r2, r1 */
    {
        EMIT(mnemonic( "mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("andn", $2) << '\t' << reg2str($4) << ", " << reg2str($3)) 
    }
    | X64_ANDN real_simd_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; andn r3, r1 */
    {
        EMIT(mnemonic( "mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("andn", $2) << '\t' << reg2str($5) << ", " << reg2str($3)) 
    }
    ;

real_cmp
    : cmp real_type X64_REG_1 X64_CONST X64_CONST /* mov true, r1 or mov flase, r1 */
    { 
        EMIT("movb\t" << cst_op_cst($1, $4, $5, true) << ", " << reg2str($3)) 
    }
    | cmp real_type X64_REG_1 any_reg X64_CONST /* cmp c, r */
    { 
        EMIT(mnemonic("ucomi", $2) << '\t' << mcst2str($5) << ", " << reg2str($4))
        if ($3->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1) << "b\t" << reg2str($3)) 
    }
    | cmp real_type X64_REG_1 X64_CONST any_reg /* cmpn c, r */
    { 
        EMIT(mnemonic("ucomi", $2) << '\t' << mcst2str($4) << ", " << reg2str($5))
        if ($3->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1, true) << "b\t" << reg2str($3)) 
    }
    | cmp real_type X64_REG_1 any_reg any_reg   /* cmp r, r */
    { 
        EMIT(mnemonic("ucomi", $2) << '\t' << reg2str($5) << ", " << reg2str($4))
        if ($3->color_ != me::Var::DONT_COLOR)
            EMIT("set" << ccsuffix($1) << "b\t" << reg2str($3))
    }
    ;

simd_cmp
    : cmp simd_type X64_REG_1 X64_CONST X64_CONST /* TODO */
    {
        swiftAssert(false, "TODO");
    }
    | cmp simd_type X64_REG_1 X64_REG_1 X64_CONST /* cmp c, r */
    {
        EMIT(mnemonic("cmp" + simdcc($1), $2) << '\t' << mcst2str($5) << ", " << reg2str($4))
    }
    | cmp simd_type X64_REG_1 X64_CONST X64_REG_1 /* cmpn c, r */
    {
        EMIT(mnemonic("cmp" + simdcc($1, true), $2) << '\t' << mcst2str($4) << ", " << reg2str($5))
    }
    | cmp simd_type X64_REG_1 X64_REG_2 X64_CONST /* mov r2, r1; cmp c, r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("cmp" + simdcc($1), $2) << '\t' << mcst2str($5) << ", " << reg2str($3))
    }
    | cmp simd_type X64_REG_1 X64_CONST X64_REG_2 /* mov r2, r1; cmpn c, r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($5) << ", " << reg2str($3))
        EMIT(mnemonic("cmp" + simdcc($1, true), $2) << '\t' << mcst2str($4) << ", " << reg2str($3))
    }
    | cmp simd_type X64_REG_1 X64_REG_1 X64_REG_1 /* TODO */
    {
        swiftAssert(false, "TODO");
    }
    | cmp simd_type X64_REG_1 X64_REG_1 X64_REG_2 /* cmp r2, r1 */
    {
        EMIT(mnemonic("cmp" + simdcc($1), $2) << '\t' << reg2str($5) << ", " << reg2str($4))
    }
    | cmp simd_type X64_REG_1 X64_REG_2 X64_REG_1 /* cmpn r2, r1 */
    {
        EMIT(mnemonic("cmp" + simdcc($1, true), $2) << '\t' << reg2str($4) << ", " << reg2str($5))
    }
    | cmp simd_type X64_REG_1 X64_REG_2 X64_REG_3 /* mov r2, r1; cmp r3, r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("cmp" + simdcc($1), $2) << '\t' << reg2str($5) << ", " << reg2str($3))
    }
    ;

int_not
    : X64_NOT int_type X64_REG_1 X64_CONST /* mov neg(c), r1 */
    {
        EMIT(mnemonic("mov", $2) << 't' << neg_cst($4) << ", " << reg2str($3))
    }
    | X64_NOT int_type X64_REG_1 X64_REG_1 /* not r1 */
    {
        EMIT(mnemonic("not", $2) << '\t' << reg2str($3))
    }
    | X64_NOT int_type X64_REG_1 X64_REG_2 /* mov r2, r1; not r1 */
    {
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT(mnemonic("not", $2) << '\t' << reg2str($3))
    }
    ;

simd_not
    : X64_NOT simd_type X64_REG_1 X64_CONST /* mov neg(c), r1 */
    {
        EMIT(mnemonic("mov", $2) << 't' << neg_cst($4) << ", " << reg2str($3))
    }
    | X64_NOT simd_type X64_REG_1 X64_REG_1 /* pcmpeqb tmp, tmp; andn tmp, r1 */
    {
        swiftAssert( $1->res_.size() == 2, "must exactly have two args" );
        me::Reg* tmp = (me::Reg*) $1->res_[1].var_;
        EMIT("pcmpeqb" << '\t' << reg2str(tmp) << ", " << reg2str(tmp))
        EMIT(mnemonic("andn", $2) << '\t' << reg2str(tmp) << ", " << reg2str($3))
    }
    | X64_NOT simd_type X64_REG_1 X64_REG_2 /* mov r2, r1; pcmpeqb tmp, tmp; andn tmp r1 */
    {
        swiftAssert( $1->res_.size() == 2, "must exactly have two args" );
        me::Reg* tmp = (me::Reg*) $1->res_[1].var_;
        EMIT(mnemonic("mov", $2) << '\t' << reg2str($4) << ", " << reg2str($3))
        EMIT("pcmpeqb" << '\t' << reg2str(tmp) << ", " << reg2str(tmp))
        EMIT(mnemonic("andn", $2) << '\t' << reg2str(tmp) << ", " << reg2str($3))
    }
    ;

commutative
    : X64_ADD  { $$ = $1; }
    | X64_MUL  { $$ = $1; }
    | X64_AND  { $$ = $1; }
    | X64_OR   { $$ = $1; }
    | X64_XOR  { $$ = $1; }
    ;

add_or_and
    : X64_ADD { $$ = $1; }
    | X64_AND { $$ = $1; }
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

sint_type
    : X64_INT8 { $$ = X64_INT8; }
    | sint_no8_type
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

real_simd_type
    : real_type { $$ = $1; }
    | simd_type { $$ = $1; }
    ;

real_type
    : X64_REAL32 { $$ = X64_REAL32; }
    | X64_REAL64 { $$ = X64_REAL64; } 
    ;

simd_type
    : X64_S_REAL32 { $$ = X64_S_REAL32; }
    | X64_S_REAL64 { $$ = X64_S_REAL64; } 
    | X64_S_INT8   { $$ = X64_S_INT8;   }
    | X64_S_INT16  { $$ = X64_S_INT16;  }
    | X64_S_INT32  { $$ = X64_S_INT32;  }
    | X64_S_INT64  { $$ = X64_S_INT64;  }
    | X64_S_SAT8   { $$ = X64_S_SAT8;   } 
    | X64_S_SAT16  { $$ = X64_S_SAT16;  }
    | X64_S_UINT8  { $$ = X64_S_UINT8;  }
    | X64_S_UINT16 { $$ = X64_S_UINT16; }
    | X64_S_UINT32 { $$ = X64_S_UINT32; }
    | X64_S_UINT64 { $$ = X64_S_UINT64; }
    | X64_S_USAT8  { $$ = X64_S_USAT8;  }
    | X64_S_USAT16 { $$ = X64_S_USAT16; }
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
    | real_simd_type        { $$ = $1; }
    ;

any_reg
    : X64_REG_1 { $$ = $1; }
    | X64_REG_2 { $$ = $1; }
    | X64_REG_3 { $$ = $1; }
    | X64_REG_4 { $$ = $1; }
    ;

%%

// get rid of local define
#undef EMIT

