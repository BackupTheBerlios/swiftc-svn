#include "be/arch.h"

namespace be {

/*
 * init global
 */

Arch* arch = 0;

//------------------------------------------------------------------------------

RegAlloc::RegAlloc(me::Function* function)
    : CodePass(function)
{}

CodeGen::CodeGen(me::Function* function)
    : CodePass(function)
{}

} // namespace be
