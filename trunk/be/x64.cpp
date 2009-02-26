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

#include "be/x64.h"

#include "me/constpool.h"
#include "me/stacklayout.h"

#include "be/x64codegen.h"
#include "be/x64regalloc.h"

namespace be {

/*
 * prefered types and pointer size
 */

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

int X64::getPtrSize() const
{
    return 8;
}

/*
 * alignment and stack layout
 */

int X64::alignOf(int size) const
{
    return std::min( nextPowerOfTwo(size), 16 );
}

int X64::getStackAlignment() const
{
    return 8;
}

size_t X64::getNumStackPlaces() const
{
    return NUM_STACK_PLACES;
}

int X64::calcStackOffset(me::StackLayout* sl, size_t place, int color) const
{
    const int globalOffset = 0;
    
    if (place == MEM)
        return sl->color2MemSlot_[color].offset_ + globalOffset;

    int result = sl->memSlotsSize_ + globalOffset;

    //if (place == XMM)
        //result += sl->places_[R].color2Slot_.size() * 8;

    result += sl->places_[place].color2Slot_[color] * 8;

    return result;
}

int X64::getItemSize(size_t place) const
{
    swiftAssert(place < NUM_STACK_PLACES, "index out of range");
    if (place == QUADWORDS)
        return 8;

    // for OCTWORDS:
    swiftAssert(place == OCTWORDS, "wrong index here");
    return 16;
}

/*
 * CodePass wrappers and the like
 */

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

/*
 * dump helper
 */

std::string X64::reg2String(const me::Reg* reg) const
{
    return X64RegAlloc::reg2String(reg);
}

} // namespace be
