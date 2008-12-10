#include "be/x64codegen.h"

#include <iostream>
#include <typeinfo>

#include "me/cfg.h"

namespace be {

/*
 * constructor
 */

X64CodeGen::X64CodeGen(me::Function* function)
    : CodeGen(function)
{}

/*
 * methods
 */

void X64CodeGen::process()
{
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        //me::InstrBase* instr = iter->value_;

    }
}

void X64CodeGen::genAsmInstr(me::InstrBase* instr)
{
    //const std::type_info& typeinfo = typeid(*instr);

}

} // namespace be
