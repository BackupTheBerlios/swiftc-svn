#include "be/x64.h"

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
}

void X64::codeGen(me::Function* function) 
{
}

} // namespace be
