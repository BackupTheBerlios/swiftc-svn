#include "be/x64regalloc.h"

#include "me/coloring.h"
#include "me/defusecalc.h"
#include "me/functab.h"
#include "me/livenessanalysis.h"
#include "me/spiller.h"

namespace be {

/*
 * constructor
 */

X64RegAlloc::X64RegAlloc(me::Function* function)
    : CodeGen(function)
{}

/*
 * methods
 */

void X64RegAlloc::process()
{
    /*
     * alloc general purpose registers
     */

    me::Colors rColors;
    for (int i = R00; i <= R15; ++i)
        rColors.insert(i);

    me::Spiller( function_, rColors.size(), R_TYPE_MASK ).process();

    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    me::Coloring(function_, R_TYPE_MASK, rColors).process();

    /*
     * alloc general purpose registers
     */

    me::Colors fColors;
    for (int i = XMM00; i <= XMM15; ++i)
        fColors.insert(i);

    me::Spiller( function_, fColors.size(), F_TYPE_MASK ).process();

    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    me::Coloring(function_, F_TYPE_MASK, fColors).process();
}

} // namespace be
