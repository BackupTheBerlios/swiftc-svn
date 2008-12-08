#include "be/arch.h"

namespace be {

RegAlloc::RegAlloc(me::Function* function)
    : CodePass(function)
{}

CodeGen::CodeGen(me::Function* function)
    : CodePass(function)
{}

} // namespace be
