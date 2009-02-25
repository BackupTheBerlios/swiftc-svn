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

#ifndef ME_ARCH_H
#define ME_ARCH_H

#include <fstream>

#include "me/codepass.h"
#include "me/op.h"

namespace me {

class StackLayout;

//------------------------------------------------------------------------------

class RegAlloc : public CodePass
{
public:

    RegAlloc(Function* function);

    void faithfulFix(InstrNode* instrNode, int typeMask, int numRegs);
};

//------------------------------------------------------------------------------

class CodeGen : public CodePass
{
protected:

    std::ofstream& ofs_;

public:

    CodeGen(Function* function, std::ofstream& ofs);
};

//------------------------------------------------------------------------------

class Arch
{
public:

    /*
     * prefered types and pointer size
     */

    virtual Op::Type getPreferedInt() const = 0;
    virtual Op::Type getPreferedUInt() const = 0;
    virtual Op::Type getPreferedReal() const = 0;
    virtual Op::Type getPreferedIndex() const = 0;
    virtual int getPtrSize() const = 0;

    /*
     * alignment and stack layout
     */

    virtual int alignOf(int size) const = 0;
    virtual int getStackAlignment() const = 0;
    virtual size_t getNumStackPlaces() const = 0;
    virtual int calcStackOffset(StackLayout* sl, size_t place, int color) const = 0;

    /** 
     * @brief Calulates the aligned offset of a \a Member based on its unaligned
     * \p offset, \a Arch::alignOf and its \p size.
     * 
     * @param offset The unaligned offset.
     * @param size The size of the \a Member item.
     * 
     * @return The aligned offset.
     */
    int calcAlignedOffset(int offset, int size);

    /** 
     * @brief Calulates the aligned offset of a place on the stack.
     * 
     * @param offset The unaligned offset.
     * @param size The size of the item in question.
     * 
     * @return The aligned offset.
     */
    int calcAlignedStackOffset(int offset, int size);

    /** 
     * @brief Calculates the next power of two of \p n if \p n is not already a
     * power of two.
     * 
     * Have a look at:
     * http://en.wikipedia.org/wiki/Power_of_two#Algorithm_to_find_the_next-highest_power_of_two 
     *
     * @param n The integer to convert.
     * 
     * @return The next power of two.
     */
    static int nextPowerOfTwo(int n);

    /*
     * CodePass wrappers and the like
     */

    virtual void regAlloc(Function* function) = 0;
    virtual void dumpConstants(std::ofstream& ofs) = 0;
    virtual void codeGen(Function* function, std::ofstream& ofs) = 0;

    /*
     * dump helper
     */

    virtual std::string reg2String(const Reg* reg) const = 0;
};

//------------------------------------------------------------------------------

extern Arch* arch;

} // namespace me

#endif // ME_ARCH_H
