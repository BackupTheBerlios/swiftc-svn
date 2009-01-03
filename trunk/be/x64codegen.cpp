#include "be/x64codegen.h"

#include <iostream>
#include <typeinfo>

#include "me/cfg.h"

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
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;
        genAsmInstr(instr);

    }
}

void X64CodeGen::genFunctionEntry()
{
    ofs_ << "\tenter;\n";
    ofs_ << "\tmov rsp, rbp;\n" << std::endl;
}

void X64CodeGen::genFunctionExit()
{
    ofs_ << "\tret" << std::endl;
}

void X64CodeGen::genAsmInstr(me::InstrBase* instr)
{
    const std::type_info& type = typeid(*instr);

    if ( type == typeid(me::LabelInstr) )
    {
        ofs_ << instr->toString() << ":\n";
        return;
    }
    else if ( type == typeid(me::AssignInstr) )
    {
        me::AssignInstr* ai = (me::AssignInstr*) instr;

        me::Reg* res = instr->res_[0].reg_;
        me::Op*  op1 = instr->arg_[0].op_;
        me::Op*  op2 = instr->arg_[1].op_;

        switch (ai->kind_)
        {
            case '=':
                ofs_ << "\tmov" << type2postfix(res->type_) << ' ';
                //ofs_ << op1->col
                break;

            default:
                // TODO
                break;
        }
    }
    
    // append ';' and new line in all cases except LabelInstr
    ofs_ << ";\n";
}

char X64CodeGen::type2postfix(me::Op::Type type)
{
    switch (type)
    {
        case me::Op::R_BOOL:
        case me::Op::R_INT8:
        case me::Op::R_UINT8:
            return 'b';
        case me::Op::R_INT16:
        case me::Op::R_UINT16:
            return 's';
        case me::Op::R_INT32:
        case me::Op::R_UINT32:
            return 'l';
        case me::Op::R_INT64:
        case me::Op::R_UINT64:
            return 'q';

        default:
            return 'T';
    }
}

} // namespace be
