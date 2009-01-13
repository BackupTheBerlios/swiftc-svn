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
std::string const_add_or_mul_const(int instr, me::Const* cst1, me::Const* cst2);

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
%token <label_>  X64_LABEL
%token <goto_>   X64_GOTO
%token <branch_> X64_BRANCH
%token <assign_> X64_MOV X64_ADD X64_SUB X64_MUL X64_DIV
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

%type <int_> bool_type int_type sint_type uint_type int_or_bool_type real_type any_type
%type <assign_> add_or_mul

%% 

instruction
    : X64_LABEL { *x64_ofs << $1->toString() << ":\n"; }
    | X64_NOP { /* do nothing */ }
    | jump_instruction
    | assign_instruction
    ;

jump_instruction
    : X64_GOTO { EMIT("jmp " << $1->label()->toString()) }
    | X64_BRANCH bool_type X64_CONST 
        {   
            if ($3->value_.bool_) 
                EMIT("jmp " << $1->trueLabel()->toString())
            else
                EMIT("jmp " << $1->falseLabel()->toString())
        }
    | X64_BRANCH bool_type X64_REG_2 /* test r1, r1; jz falseLabel; jmp trueLabel */
        { EMIT("testb " << reg2str($3) << ", " << reg2str($3))
          EMIT("jz " << $1->trueLabel()->toString())
          EMIT("jmp " << $1->falseLabel()->toString()) }
    ;

assign_instruction
    : move
    | int_add
    | int_sub
    /*| int_mul*/
    | real_add_mul
    ;

move
    : X64_MOV any_type X64_UNDEF { /* emit no code: move X64_UNDEF, %r1 */ }
    | X64_MOV any_type X64_CONST { EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg())) }
    | X64_MOV any_type X64_REG_1 { /* emit no code: mov %r1, %r1 */ }
    | X64_MOV any_type X64_REG_2 { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) }
    ;

int_add
    : X64_ADD int_type X64_CONST X64_CONST /* mov (c1 + c2), r1 */
        { EMIT("mov" << suffix($2) << ' ' << ($3->value_.uint64_ + $4->value_.uint64_) << ", " << reg2str($1->resReg())) } 
    | X64_ADD int_type X64_CONST X64_REG_1 /* add c, r1 */
        { EMIT("add" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($4)) }
    | X64_ADD int_type X64_CONST X64_REG_2 /* mov c, r1; add r2, r1 */ 
        { EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    | X64_ADD int_type X64_REG_1 X64_CONST /* add c, r1 */
        { EMIT("add" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3)) }
    | X64_ADD int_type X64_REG_1 X64_REG_1 /* add r1, r1 or: shl 2, r2 */
        { EMIT("shl" << suffix($2) << " $2, " << reg2str($3)) }
    | X64_ADD int_type X64_REG_1 X64_REG_2 /* add r2, r1 */
        { EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) }
    | X64_ADD int_type X64_REG_2 X64_CONST /* mov c, r1; add r2, r1 */ 
        { EMIT("mov" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) }
    | X64_ADD int_type X64_REG_2 X64_REG_1 /* add r2, r1 */
        { EMIT("add" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4)) }
    | X64_ADD int_type X64_REG_2 X64_REG_2 /* mov r2, r1; shl 2, r1 */
        { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("shl" << suffix($2) << " $2,  " << reg2str($1->resReg())) }
    | X64_ADD int_type X64_REG_2 X64_REG_3 /* mov r2, r1; add r3, r1 */
        { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("add" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    ;

int_sub
    : X64_SUB int_type X64_CONST X64_CONST /* mov (c1 - c2), r1 */
        { EMIT("mov" << suffix($2) << ' ' << ($3->value_.uint64_ - $4->value_.uint64_) << ", " << reg2str($1->resReg())) } 
    | X64_SUB int_type X64_CONST X64_REG_1 /* sub c, r1; neg r1 */
        { EMIT("sub" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($4)) 
          EMIT("neg" << suffix($2) << ' ' << reg2str($4)) }
    | X64_SUB int_type X64_CONST X64_REG_2 /* mov c, r1; sub r2, r1 */
        { EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    | X64_SUB int_type X64_REG_1 X64_CONST /* sub c, r1 */
        { EMIT("sub" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3)) }
    | X64_SUB int_type X64_REG_1 X64_REG_1 /* xor r1, r1 */
        { EMIT("xor" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($3)) }
    | X64_SUB int_type X64_REG_1 X64_REG_2 /* sub r2, r1 */
        { EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) }
    | X64_SUB int_type X64_REG_2 X64_CONST /* mov c, r1; sub r2, r1 */ 
        { EMIT("mov" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) }
    | X64_SUB int_type X64_REG_2 X64_REG_1 /* sub r2, r1; neg r1*/
        { EMIT("sub" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4))
          EMIT("neg" << suffix($2) << ' ' << reg2str($4)) }
    | X64_SUB int_type X64_REG_2 X64_REG_2 /* xor r1, r1 */
        { EMIT("xor" << suffix($2) << ' ' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) }
    | X64_SUB int_type X64_REG_2 X64_REG_3 /* mov r2, r1; sub r3, r1 */
        { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    ;

/*int_mul*/
    /*: X64_mul int_type X64_CONST X64_CONST [> mov (c1 - c2), r1 <]*/
        /*[>{ EMIT("mov" << suffix($2) << ' ' << ($3->value_.uint64_ - $4->value_.uint64_) << ", " << reg2str($1->resReg())) } <]*/
        /*{ [> TODO <] }*/
    /*| X64_MUL int_type X64_CONST X64_REG_1 [> mul c <]*/
        /*{ EMIT(mul2str($2) << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($4)) }*/
    /*| X64_MUL int_type X64_CONST X64_REG_2 [> mov c, r1; sub r2, r1 <]*/
        /*{ EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg()))*/
          /*EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }*/
    /*| X64_MUL int_type X64_REG_1 X64_CONST [> sub c, r1 <]*/
        /*{ EMIT("sub" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3)) }*/
    /*| X64_MUL int_type X64_REG_1 X64_REG_1 [> xor r1, r1 <]*/
        /*{ EMIT("xor" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($3)) }*/
    /*| X64_MUL int_type X64_REG_1 X64_REG_2 [> sub r2, r1 <]*/
        /*{ EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) }*/
    /*| X64_MUL int_type X64_REG_2 X64_CONST [> mov c, r1; sub r2, r1 <] */
        /*{ EMIT("mov" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($1->resReg()))*/
          /*EMIT("sub" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) }*/
    /*| X64_MUL int_type X64_REG_2 X64_REG_1 [> sub r2, r1; neg r1<]*/
        /*{ EMIT("sub" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4))*/
          /*EMIT("neg" << suffix($2) << ' ' << reg2str($4)) }*/
    /*| X64_MUL int_type X64_REG_2 X64_REG_2 [> xor r1, r1 <]*/
        /*{ EMIT("xor" << suffix($2) << ' ' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) }*/
    /*| X64_MUL int_type X64_REG_2 X64_REG_3 [> mov r2, r1; sub r3, r1 <]*/
        /*{ EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))*/
          /*EMIT("sub" << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }*/
    /*;*/
    
real_add_mul
    : add_or_mul real_type X64_CONST X64_CONST /* mov (c1 + c2), r1 */
        { swiftAssert(false, "not allowed code sequence"); }
    | add_or_mul real_type X64_CONST X64_REG_1 /* add c, r1 */
        { EMIT(instr2str($1) << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($4)) }
    | add_or_mul real_type X64_CONST X64_REG_2 /* mov c, r1; add r2, r1 */ 
        { EMIT("mov" << suffix($2) << ' ' << cst2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    | add_or_mul real_type X64_REG_1 X64_CONST /* add c, r1 */
        { EMIT(instr2str($1) << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($3)) }
    | add_or_mul real_type X64_REG_1 X64_REG_1 /* add r1, r1 */
        { EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($3)) }
    | add_or_mul real_type X64_REG_1 X64_REG_2 /* add r2, r1 */
        { EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($3)) }
    | add_or_mul real_type X64_REG_2 X64_CONST /* mov c, r1; add r2, r1 */ 
        { EMIT("mov" << suffix($2) << ' ' << cst2str($4) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg())) }
    | add_or_mul real_type X64_REG_2 X64_REG_1 /* add r2, r1 */
        { EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($4)) }
    | add_or_mul real_type X64_REG_2 X64_REG_2 /* mov r2, r1; add r1, r1 */
        { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($1->resReg()) << ", " << reg2str($1->resReg())) }
    | add_or_mul real_type X64_REG_2 X64_REG_3 /* mov r2, r1; add r3, r1 */
        { EMIT("mov" << suffix($2) << ' ' << reg2str($3) << ", " << reg2str($1->resReg()))
          EMIT(instr2str($1) << suffix($2) << ' ' << reg2str($4) << ", " << reg2str($1->resReg())) }
    ;

add_or_mul
    : X64_ADD { $$ = $1; }
    | X64_MUL { $$ = $1; }
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

any_type
    : int_or_bool_type { $$ = $1; }
    | real_type { $$ = $1; }
    ;

%%

std::string suffix(int type)
{
    switch (type)
    {
        case X64_BOOL:
        case X64_INT8:
        case X64_UINT8:
            return "b"; // byte

        case X64_INT16:
        case X64_UINT16:
            return "s"; // short

        case X64_INT32:
        case X64_UINT32:
            return "l"; // long

        case X64_INT64:
        case X64_UINT64:
            return "q"; // quad

        case X64_REAL32:
            return "ss";// scalar single
            
        case X64_REAL64:
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

std::string const_add_or_mul_const(int instr, me::Const* cst1, me::Const* cst2)
{
    swiftAssert(cst1->type_ == cst2->type_, "types must be equal" );
    swiftAssert(cst1->type_ == me::Op::R_REAL32 
             || cst2->type_ == me::Op::R_REAL64 , "type must be a real");
    swiftAssert(instr == X64_ADD || instr == X64_MUL, "must be X64_ADD or X64_MUL");

    std::ostringstream oss;
    if (cst1->type_ == me::Op::R_REAL32)
    {
        if (instr == X64_ADD)
            oss << (cst1->value_.real32_ + cst2->value_.real32_);
        else
            oss << (cst1->value_.real32_ * cst2->value_.real32_);
    }
    else
    {
        if (instr == X64_ADD)
            oss << (cst1->value_.real64_ + cst2->value_.real64_);
        else
            oss << (cst1->value_.real64_ * cst2->value_.real64_);
    }

    return oss.str();
}

std::string mul2str(int type)
{
    switch (type)
    {
        case X64_INT8:
        case X64_INT16:
        case X64_INT32:
        case X64_INT64:
            return "imul";
        case X64_UINT8:
        case X64_UINT16:
        case X64_UINT32:
        case X64_UINT64:
            return "mul";
        default:
            swiftAssert( false, "unreachable code" ); 
    }

    return "error";
}

// get rid of local define
#undef EMIT

