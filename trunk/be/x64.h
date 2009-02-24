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

#ifndef BE_X86_64_H
#define BE_X86_64_H

#include "me/arch.h"

namespace be {

class X64 : public me::Arch
{
public:

    enum
    {
        MEM, 
        R, 
        XMM,
        NUM_STACK_PLACES
    };

    virtual me::Op::Type getPreferedInt() const;
    virtual me::Op::Type getPreferedUInt() const;
    virtual me::Op::Type getPreferedReal() const;
    virtual me::Op::Type getPreferedIndex() const;

    virtual int getPtrSize() const;
    virtual int alignOf(int size) const;

    virtual void regAlloc(me::Function* function);
    virtual void dumpConstants(std::ofstream& ofs);
    virtual void codeGen(me::Function* function, std::ofstream& ofs);

    virtual size_t getNumStackPlaces() const;

    virtual std::string reg2String(const me::Reg* reg) const;
};

} // namespace be

#endif // BE_X86_64_H
