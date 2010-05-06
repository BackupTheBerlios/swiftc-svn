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

#include "be/x64phiimpl.h"

#include "be/x64codegenhelpers.h"
#include "be/x64parser.h"
#include "be/x64regalloc.h"
#include "be/x64typeconv.h"

namespace be {

/*
 * constructor
 */

X64PhiImpl::X64PhiImpl(Kind kind,
                       me::BBNode* prevNode,
                       me::BBNode* nextNode,
                       const me::Colors& usedColors,
                       std::ofstream& ofs)
    : me::PhiImpl(prevNode, nextNode)
    , kind_(kind)
    , usedColors_(usedColors)
    , ofs_(ofs)
    , scratchColor_(-1)
    , restoreScratchReg_(false)
{
    switch (kind_)
    {
        case INT_REG:
            typeMask_ = X64RegAlloc::INT_TYPE_MASK;
            freeRegTypeMask_ = X64RegAlloc::INT_TYPE_MASK;
            break;
        case XMM_REG:
            typeMask_ = X64RegAlloc::XMM_TYPE_MASK;
            freeRegTypeMask_ = X64RegAlloc::XMM_TYPE_MASK;
            break;
        case QUADWORD_SPILL_SLOTS:
            typeMask_ = X64RegAlloc::QUADWORD_TYPE_MASK;
            freeRegTypeMask_ = X64RegAlloc::INT_TYPE_MASK;
            break;
        case OCTWORD_SPILL_SLOTS:
            typeMask_ = X64RegAlloc::OCTWORD_TYPE_MASK;
            freeRegTypeMask_ = X64RegAlloc::XMM_TYPE_MASK;
            break;
    }
}

/*
 * virtual methods
 */

void X64PhiImpl::fillColors()
{
    if ( kind_ == XMM_REG || kind_ == OCTWORD_SPILL_SLOTS )
        free_ = *X64RegAlloc::getXmmColors();
    else // -> INT_REG or QUADWORD_SPILL_SLOTS
        free_ = *X64RegAlloc::getIntColors();
}

void X64PhiImpl::eraseColors()
{
    if (kind_ == XMM_REG || kind_ == OCTWORD_SPILL_SLOTS)
        return;
    // else
    // -> INT_REG or QUADWORD_SPILL_SLOTS

    // use these ones only if they are used anyway
    if ( !usedColors_.contains(X64RegAlloc::RBX) ) free_.erase(X64RegAlloc::RBX);
    if ( !usedColors_.contains(X64RegAlloc::RBP) ) free_.erase(X64RegAlloc::RBP);
    if ( !usedColors_.contains(X64RegAlloc::R12) ) free_.erase(X64RegAlloc::R12);
    if ( !usedColors_.contains(X64RegAlloc::R13) ) free_.erase(X64RegAlloc::R13);
    if ( !usedColors_.contains(X64RegAlloc::R14) ) free_.erase(X64RegAlloc::R14);
    if ( !usedColors_.contains(X64RegAlloc::R15) ) free_.erase(X64RegAlloc::R15);
}

void X64PhiImpl::genMove(me::Op::Type type, int r1, int r2)
{
    switch (kind_)
    {
        case INT_REG: 
        case XMM_REG:
            // mov r1, r2
            ofs_ << '\t' << mnemonic("mov", meType2beType(type)) << '\t';
            ofs_ << reg2str(r1, type) << ", " << reg2str(r2, type) << '\n';
            break;
        case QUADWORD_SPILL_SLOTS:
            // push r1; pop r2
            ofs_ << "\tpushq\t" << spilledReg2str(r1, type) << '\n';
            ofs_ << "\tpopq\t"  << spilledReg2str(r2, type) << '\n';
            break;
        case OCTWORD_SPILL_SLOTS:
            if (scratchColor_ == -1)
                getScratchReg();

            // mov r1, scratch
            ofs_ << '\t' << mnemonic("mov", meType2beType(type)) << '\t';
            ofs_ << spilledReg2str(r1, type) << ", " << reg2str(scratchColor_, type) << '\n';

            // mov scratch, r2
            ofs_ << '\t' << mnemonic("mov", meType2beType(type)) << '\t';
            ofs_ << reg2str(scratchColor_, type) << ", " << spilledReg2str(r2, type) << '\n';
    }
}

void X64PhiImpl::genReg2Tmp(me::Op::Type type, int r)
{
    switch (kind_)
    {
        case INT_REG:
            if ( free_.empty() )
            {
                // mov r1, mem
                ofs_ << "\tpushq\t" << reg2str(r, type) << '\n';
            }
            else
            {
                // mov r, tmp
                tmpRegColor_ = *free_.begin();
                swiftAssert(r != tmpRegColor_, "must be different");
                genMove(type, r, tmpRegColor_);
            }
            break;

        case XMM_REG:
            if ( free_.empty() )
            {
                // mov r, mem: store in the 128 byte red zone area at -16(%rsp)
                ofs_ << "\tmovdqa\t" << reg2str(r, type) << ", -16(%rsp)\n";
            }
            else
            {
                // mov r, tmp
                tmpRegColor_ = *free_.begin();
                swiftAssert(r != tmpRegColor_, "must be different");
                genMove(type, r, tmpRegColor_);
            }
            break;

        case QUADWORD_SPILL_SLOTS:
            if ( free_.empty() )
            {
                // simply take %rax and save old value
                ofs_ << "\tpushq\t%rax\n";

                // mov r, %rax
                ofs_ << "\tmovq\t" << spilledReg2str(r, type) << ", %rax\n";
            }
            else
            {
                // mov r, tmp
                tmpRegColor_ = *free_.begin();
                swiftAssert(r != tmpRegColor_, "must be different");
                ofs_ << "\tmovq\t" << spilledReg2str(r, type) << ", " 
                     << reg2str(tmpRegColor_, type) << '\n';
            }
            break;

        case OCTWORD_SPILL_SLOTS:
            if ( free_.empty() )
            {
                // simply use %xmm1 and store old value at -32(%rsp) at the red zone
                ofs_ << "\tmovdqa\t%xmm1, -32(%rsp)\n";

                // mov r, %xmm1
                ofs_ << "\tmovq\t" << spilledReg2str(r, type) << ", %xmm1\n";
            }
            else
            {
                // mov r, tmp
                tmpRegColor_ = *free_.begin();
                swiftAssert(r != tmpRegColor_, "must be different");
                ofs_ << "\tmovq\t" << spilledReg2str(r, type) << ", " 
                     << reg2str(tmpRegColor_, type) << '\n';
            }
    }
}

void X64PhiImpl::genTmp2Reg(me::Op::Type type, int r)
{
    switch (kind_)
    {
        case INT_REG:
            if ( free_.empty() )
                ofs_ << "\tpopq\t" << reg2str(r, type) << '\n'; // mov mem, r
            else
            {
                // mov tmp, r
                swiftAssert(r != tmpRegColor_, "must be different");
                genMove(type, tmpRegColor_, r);
            }
            break;

        case XMM_REG:
            if ( free_.empty() )
            {
                // mov mem, r: reload from the 128 byte red zone area at -16(%rsp)
                ofs_ << "\tmovdqa\t-16(%rsp), " << reg2str(r, type) << '\n';
            }
            else
            {
                // mov tmp, r
                swiftAssert(r != tmpRegColor_, "must be different");
                genMove(type, tmpRegColor_, r);
            }
            break;

        case QUADWORD_SPILL_SLOTS:
            if ( free_.empty() )
            {
                // mov %rax, r 
                ofs_ << "\tmovq\t%rax, " << spilledReg2str(r, type) << '\n';

                // restore %rax
                ofs_ << "\tpopq\t%rax\n";
            }
            else
            {
                // mov tmp, r
                swiftAssert(r != tmpRegColor_, "must be different");
                ofs_ << "\tmovq\t" << reg2str(tmpRegColor_, type) << ", " 
                     << spilledReg2str(r, type) << '\n';
            }
            break;

        case OCTWORD_SPILL_SLOTS:
            if ( free_.empty() )
            {
                // mov %xmm1, r
                ofs_ << "\tmovq\t%xmm1, " << spilledReg2str(r, type) << '\n';

                // restore %xmm1
                ofs_ << "\tmovdqa\t-32(%rsp), %xmm1\n";
            }
            else
            {
                // mov tmp, r
                tmpRegColor_ = *free_.begin();
                swiftAssert(r != tmpRegColor_, "must be different");
                ofs_ << "\tmovq\t" << reg2str(tmpRegColor_, type )<< ", " 
                     << spilledReg2str(r, type) << '\n';
            }
    }
}

void X64PhiImpl::getScratchReg()
{
    if (kind_ == OCTWORD_SPILL_SLOTS)
    {
        if ( free_.size() < 2 )
        {
            // simply use %xmm0 and store old value at -16(%rsp) at the red zone
            ofs_ << "\tmovdqa\t%xmm0, -16(%rsp)\n";
            scratchColor_ = X64RegAlloc::XMM0;
            restoreScratchReg_ = true;
        }
        else
            scratchColor_ = *++free_.begin(); // use second free reg
    }
}

void X64PhiImpl::cleanUp()
{
    if (restoreScratchReg_)
    {
        swiftAssert(kind_ == OCTWORD_SPILL_SLOTS, "must be OCTWORD_SPILL_SLOTS");
        // restore %xmm0
        ofs_ << "\tmovdqa\t-16(%rsp), %xmm0\n";
    }
}

/*
 * further methods
 */

bool X64PhiImpl::isSpilled() const
{
    return kind_ == QUADWORD_SPILL_SLOTS || kind_ == OCTWORD_SPILL_SLOTS;
}

} // namespace be
