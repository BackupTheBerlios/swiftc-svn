#ifndef BE_X64_CODE_GEN_HELPERS_H
#define BE_X64_CODE_GEN_HELPERS_H

#include <string>

#include "be/x64regalloc.h"

// forward declarations
namespace me {
    struct Op;
    struct Undef;
    struct Const;
    struct Reg;

    struct AssignInstr;
}

namespace be {

std::string suffix(int);

inline std::string reg2str(me::Reg* reg)
{
    return X64RegAlloc::reg2String(reg);
}

std::string ccsuffix(me::AssignInstr* ai, int type, bool neg = false);

std::string cst2str(me::Const* cst);
std::string mcst2str(me::Const* cst);
std::string op2str(me::Op* op);

std::string instr2str(me::AssignInstr* ai);
std::string mul2str(int type);
std::string div2str(int type);

std::string const_add_or_mul_const(int instr, me::Const* cst1, me::Const* cst2);
std::string const_cmp_const(me::AssignInstr* ai, me::Const* cst1, me::Const* cst2);

std::string neg_mask(int type);

}

#endif // BE_X64_CODE_GEN_HELPERS_H
