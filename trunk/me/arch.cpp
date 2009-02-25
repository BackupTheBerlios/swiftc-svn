/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

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

int Arch::calcAlignedOffset(int offset, int size)
{
    swiftAssert(size != 0, "size is zero");
    int result = offset;
    int align = alignOf(size);
    int mod = result % align;

    // do we have to adjust the offset due to alignment?
    if (mod != 0)
        result += align - mod;

    return result;
}

int Arch::calcAlignedStackOffset(int offset, int size)
{
    swiftAssert(size != 0, "size is zero");
    int result = offset;
    int align = std::max( alignOf(size), getStackAlignment() );
    int mod = result % align;

    // do we have to adjust the offset due to alignment?
    if (mod != 0)
        result += align - mod;

    return result;
}

int Arch::nextPowerOfTwo(int n) 
{
    n--;
    for (int i = 1; i < (int) sizeof(int)*8; i <<= 1)
            n = n | n >> i;

    return n + 1;
}

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
        if (instr->res_[i].var_->typeCheck(typeMask) )
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
    VarVec dummies;
    for (int i = 0; i < diff; ++i)
    {
        Var* dummy = function_->newSSAReg(Op::R_INT32);

        // add dummy args
        instr->arg_.push_back( Arg(dummy, -1) );
        dummies.push_back(dummy);
    }
}


//------------------------------------------------------------------------------

CodeGen::CodeGen(Function* function, std::ofstream& ofs)
    : CodePass(function)
    , ofs_(ofs)
{}

} // namespace me
