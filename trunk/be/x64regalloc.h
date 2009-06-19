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

#ifndef BE_X64_REG_ALLOC_H
#define BE_X64_REG_ALLOC_H

#include <string>

#include "utils/set.h"

#include "me/arch.h"
#include "me/codepass.h"

/*
 * forward declarations
 */

namespace me {
    typedef Set<int> Colors;
}

namespace be {

class X64RegAlloc : public me::RegAlloc
{
public:

    enum Regs
    {
        /*
         * general purpose registers
         */
        R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, 

        /*
         * deprefer these registers since they must be preserved across
         * function calls, but if needed prefer R3 since it can be encoded
         * shorter
         */
        R12, R13, R14, R15, 

        /*
         * XMM registers
         */
        XMM0, XMM1, XMM2,  XMM3,  XMM4,  XMM5,  XMM6,  XMM7, 
        XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,

        // yes, this order is correct
        RAX = R0, 
        RCX = R1, 
        RDX = R2, 
        RBX = R3,
        RSP = R4, 
        RBP = R5, 
        RSI = R6, 
        RDI = R7 
    };

    /*
     * condition codes
     */
    enum CC
    {
        C_EQ, C_NE,
        C_L,  C_LE,
        C_G,  C_GE,
        C_B,  C_BE,
        C_A,  C_AE
    };

    enum
    {
        R_TYPE_MASK 
            = me::Op::R_BOOL
            | me::Op::R_INT8  | me::Op::R_INT16  | me::Op::R_INT32  | me::Op::R_INT64 
            | me::Op::R_UINT8 | me::Op::R_UINT16 | me::Op::R_UINT32 | me::Op::R_UINT64
            | me::Op::R_SAT8  | me::Op::R_SAT16
            | me::Op::R_USAT8 | me::Op::R_USAT16
            | me::Op::R_PTR,

        F_TYPE_MASK
            = me::Op::R_REAL32| me::Op::R_REAL64
    };

    enum
    {
        NUM_INT_REGS = 6,
        NUM_XMM_REGS = 8,
        NUM_INT_RETURN_REGS = 2,
        NUM_XMM_RETURN_REGS = 2,
        NUM_INT_CLOBBERED_REGS = 9, 
        NUM_XMM_CLOBBERED_REGS = 16,
    };

    static int intRegs[NUM_INT_REGS]; 
    static int xmmRegs[NUM_XMM_REGS];
    static int intReturnRegs[NUM_INT_RETURN_REGS]; 
    static int xmmReturnRegs[NUM_XMM_RETURN_REGS];
    static int intClobberedRegs[NUM_INT_CLOBBERED_REGS]; 
    static int xmmClobberedRegs[NUM_XMM_CLOBBERED_REGS];

    static me::Colors* intColors_;
    static me::Colors* xmmColors_;

    /*
     * constructor and destructor
     */

    X64RegAlloc(me::Function* function);
    static void destroyColors();

    /*
     * further methods
     */

    static const me::Colors* getIntColors();
    static const me::Colors* getXmmColors();

    virtual void process();

    bool arg2Reg(me::InstrNode* iter, size_t i);

    void registerTargeting();

    void targetAssignInstr(me::InstrNode* iter, me::BBNode* currentBB);
    void targetBranchInstr(me::InstrNode* iter, me::BBNode* currentBB);
    void targetStore(me::InstrNode* iter, me::BBNode* currentBB);
    void targetSetParams(me::InstrNode* iter, me::BBNode* currentBB);
    void targetSetResults(me::InstrNode* iter, me::BBNode* currentBB);
    void targetCallInstr(me::InstrNode* iter, me::BBNode* currentBB);

    static std::string reg2String(const me::Reg* reg);
};

} // namespace be

#endif // BE_X64_REG_ALLOC_H
