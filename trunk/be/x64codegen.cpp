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
        me::InstrBase* instr = iter->value_;

    }
}

void X64CodeGen::genAsmInstr(me::InstrBase* instr)
{
    const std::type_info& typeinfo = typeid(*instr);

}

void X64CodeGen::assignInstr2Asm(me::AssignInstr* ab)
{
    swiftAssert( ab->numRhs_ > 0, "no args" );
    me::Op::Type type = ab->rhs_[0]->type_;

#ifdef SWIFT_DEBUG

    for (size_t i = 0; i < ab->numLhs_; ++i)
        swiftAssert( ab->lhs_[i]->type_ == type, "not same type here" );

    for (size_t i = 0; i < ab->numRhs_; ++i)
        swiftAssert( ab->rhs_[i]->type_ == type, "not same type here" );

#endif // SWIFT_DEBUG

}

} // namespace be
