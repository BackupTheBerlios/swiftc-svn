#include "me/arch.h"

#include "me/functab.h"

#include <algorithm>
#include <typeinfo>

namespace me {

//------------------------------------------------------------------------------

/*
 * init global
 */

Arch* arch = 0;

//------------------------------------------------------------------------------

RegAlloc::RegAlloc(Function* function)
    : CodePass(function)
{}

void RegAlloc::faithfulFix(InstrNode* instrNode, int typeMask, int numRegs)
{
    /*
     * TODO TODO TODO TODO TODO TODO
     */
    InstrBase* instr = instrNode->value_;

    int numRegsNeeded = 0;

    // count number of results with proper type
    int numLhs = 0;
    for (size_t i = 0; i < instr->res_.size(); ++i)
    {
        if (instr->res_[i].reg_->typeCheck(typeMask) )
            ++numLhs;
    }

    int numLiveThrough = 0;

    // count number of args with proper type and number of live-through vars
    int numRhs = 0;
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        if ( reg->typeCheck(typeMask) )
            ++numRhs;

        if ( instr->livesThrough(reg) )
            ++numLiveThrough;
    }

    int numRegsAllowed = std::max(numLhs + numLiveThrough, numRhs);

    int diff = numRegsAllowed - numRegsNeeded;
    RegVec dummies;
    for (int i = 0; i < diff; ++i)
    {
        Reg* dummy = function_->newSSA(Op::R_INT32);

        // add dummy args
        instr->arg_.push_back( Arg(dummy, -1) );
        dummies.push_back(dummy);
    }
}


//------------------------------------------------------------------------------

CodeGen::CodeGen(Function* function)
    : CodePass(function)
{}

} // namespace me
