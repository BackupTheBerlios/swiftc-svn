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

#ifndef BE_X64_PHI_IMPL_H
#define BE_X64_PHI_IMPL_H

#include <fstream>

#include "me/forward.h"
#include "me/phiimpl.h"

namespace be {

class X64PhiImpl : public me::PhiImpl
{
public:

    enum Kind
    {
        XMM_REG,
        INT_REG,
        QUADWORD_SPILL_SLOTS,
        OCTWORD_SPILL_SLOTS,
    };

    /*
     * constructor
     */

    X64PhiImpl(Kind kind,
               me::BBNode* prevNode,
               me::BBNode* nextNode,
               const me::Colors& usedColors,
               std::ofstream& ofs);

    /*
     * virtual methods
     */

    virtual bool isSpilled() const;

    virtual void fillColors();
    virtual void eraseColors();

    virtual void genMove(me::Op::Type type, int r1, int r2);
    virtual void genReg2Tmp(me::Op::Type type, int r);
    virtual void genTmp2Reg(me::Op::Type type, int r);

    virtual void cleanUp();

    /*
     * further methods
     */

    void getScratchReg();

private:

    /*
     * data
     */

    Kind kind_;
    const me::Colors& usedColors_;
    std::ofstream& ofs_;
    int scratchColor_;
    int tmpRegColor_;
    bool restoreScratchReg_;
};

} // namespace be

#endif // BE_X64_PHI_IMPL_H
