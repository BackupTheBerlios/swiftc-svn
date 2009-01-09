#include "be/x64codegen.h"

#include <iostream>
#include <typeinfo>

#include "me/cfg.h"
#include "me/constpool.h"
#include "me/functab.h"

#include "be/x64regalloc.h"

namespace be {

/*
 * constructor
 */

X64CodeGen::X64CodeGen(me::Function* function, std::ofstream& ofs)
    : CodeGen(function, ofs)
{}

/*
 * further methods
 */

void X64CodeGen::process()
{
    ofs_ << '\n';
    ofs_ << *function_->id_ << ":\n";
    ofs_ << "\tenter $0, $32\n";

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;
        genAsmInstr(instr);

    }

    ofs_ << "\tleave\n\n";
    ofs_ << "\tret\n";
    ofs_ << '\n';
}

void X64CodeGen::genAsmInstr(me::InstrBase* instr)
{
    const std::type_info& instrTypeId = typeid(*instr);

    if ( instrTypeId == typeid(me::LabelInstr) )
        ofs_ << instr->toString() << ":\n";
    else if (instrTypeId == typeid(me::GotoInstr) )
    {
        me::GotoInstr* gi = (me::GotoInstr*) instr;
        ofs_ << "\tjmp " << gi->label()->toString() << "\n";
    }
    else if ( instrTypeId == typeid(me::AssignInstr) )
    {
        me::Op::Type resType = instr->res_[0].reg_->type_;

        if (resType == me::Op::R_REAL32 || resType == me::Op::R_REAL64)
            genFloatingPointInstr( (me::AssignInstr*) instr );
        else
            genGeneralPurposeInstr( (me::AssignInstr*) instr );
    }
}

enum
{
    CONST,      // a = CONST
    EQUAL,   // r1 = x # x
    SIMPLE, // r1 = r1/const # x
    COMMUTATIVE_SIMPLE // r1 = x # r1/const

};

//void X64CodeGen::checkInstrComplexity(me::Op* op1, me::Op2)
//{
    //me::InstrBase::OpType opType1 = ai->getOpType(0);
    //me::InstrBase::OpType opType2 = ai->getOpType(1);

    //if (opType1 == me::InstrBase::CONST && opType2 == me::InstrBase::CONST)
        //return CONST;

    //if (op1 == op2)
        //return SIMPLE;

//}

void X64CodeGen::genGeneralPurposeInstr(me::InstrBase* instr)
{
    me::AssignInstr* ai = (me::AssignInstr*) instr;

    me::Reg* res = instr->res_[0].reg_;
    me::Op*  op1 = instr->arg_[0].op_;
    me::Op*  op2 = instr->arg_[1].op_;

    switch (ai->kind_)
    {
        case '=':
            ofs_ << "\tmov" << type2postfix(res->type_) << ' ';
            ofs_ << op2string(op1) << ", ";
            ofs_ << X64RegAlloc::reg2String(res) << "\n";
            break;

        case '+':
            ofs_ << "\tadd" << type2postfix(res->type_) << " %rax, %rax\n";
            break;

        default:
            // TODO
            break;
    }
}

void X64CodeGen::genFloatingPointInstr(me::InstrBase* instr)
{
    me::AssignInstr* ai = (me::AssignInstr*) instr;

    me::Reg* res = instr->res_[0].reg_;
    me::Op*  op1 = instr->arg_[0].op_;
    me::Op*  op2 = instr->arg_[1].op_;

    switch (ai->kind_)
    {
        case '=':
            ofs_ << "\tmov" << type2postfix(res->type_) << ' ';
            ofs_ << op2string(op1) << ", ";
            ofs_ << X64RegAlloc::reg2String(res) << "\n";
            break;

        case '+':
            ofs_ << "\tadd" << type2postfix(res->type_) << " %xmm1, %xmm0\n";
            break;

        default:
            // TODO
            break;
    }
}

std::string X64CodeGen::type2postfix(me::Op::Type type)
{
    switch (type)
    {
        case me::Op::R_BOOL:
        case me::Op::R_INT8:
        case me::Op::R_UINT8:
            return "b"; // byte

        case me::Op::R_INT16:
        case me::Op::R_UINT16:
            return "s"; // short

        case me::Op::R_INT32:
        case me::Op::R_UINT32:
            return "l"; // long

        case me::Op::R_INT64:
        case me::Op::R_UINT64:
            return "q"; // quad

        case me::Op::R_REAL32:
            return "ss";// scalar single
            
        case me::Op::R_REAL64:
            return "sd";// scalar double

        default:
            return "TODO";
    }
}

std::string X64CodeGen::op2string(me::Op* op)
{
    std::ostringstream oss;

    swiftAssert( typeid(*op) != typeid(me::Undef), "must not be Undef here" );

    if ( typeid(*op) == typeid(me::Reg) )
    {
        me::Reg* reg = (me::Reg*) op;

        if ( reg->isMem() )
            oss << "TODO";
        else
            oss << X64RegAlloc::reg2String(reg);
    }
    else
    {
        swiftAssert( typeid(*op) == typeid(me::Literal), "must be a Literal here" );
        me::Literal* literal = (me::Literal*) op;

        if (op->type_ == me::Op::R_REAL32)
            oss << ".LC" << me::constpool->uint32_[literal->value_.uint32_];
        else if (op->type_ == me::Op::R_REAL64)
            oss << ".LC" << me::constpool->uint64_[literal->value_.uint64_];
        else
            oss << '$' << literal->value_.uint64_;
    }

    return oss.str();
}

} // namespace be
