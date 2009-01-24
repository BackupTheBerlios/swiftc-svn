#include "be/x64.h"

#include "me/constpool.h"

#include "be/x64codegen.h"
#include "be/x64regalloc.h"

namespace be {

me::Op::Type X64::getPreferedInt() const 
{
    return me::Op::R_INT32;
}

me::Op::Type X64::getPreferedUInt() const
{
    return me::Op::R_UINT32;
}

me::Op::Type X64::getPreferedReal() const
{
    return me::Op::R_REAL32;
}

me::Op::Type X64::getPreferedIndex() const
{
    return me::Op::R_UINT64;
}

void X64::regAlloc(me::Function* function) 
{
    X64RegAlloc(function).process();
}

void X64::dumpConstants(std::ofstream& ofs)
{
    UINT8MAP_EACH(iter)
    {
        ofs << ".LC" << iter->second << ":\n";
        ofs << ".byte " << iter->first << '\n';
    }
    UINT16MAP_EACH(iter)
    {
        ofs << ".LC" << iter->second << ":\n";
        ofs << ".short " << iter->first << '\n';
    }
    UINT32MAP_EACH(iter)
    {
        ofs << ".LC" << iter->second << ":\n";
        ofs << ".long " << iter->first << '\n';
    }

    // sign mask for real32
    ofs << ".LCS32:\n";
    ofs << ".long " << 0x80000000 << '\n';

    UINT64MAP_EACH(iter)
    {
        ofs << ".LC" << iter->second << ":\n";
        ofs << ".quad " << iter->first << '\n';
    }

    // sign mask for real64
    ofs << ".LCS64:\n";
    ofs << ".quad " << 0x8000000000000000ULL << '\n';

    ofs << '\n';
}

void X64::codeGen(me::Function* function, std::ofstream& ofs)
{
    X64CodeGen(function, ofs).process();
}

std::string X64::reg2String(const me::Reg* reg) const
{
    return X64RegAlloc::reg2String(reg);
}

} // namespace be
