#ifndef BE_X64_REG_ALLOC_H
#define BE_X64_REG_ALLOC_H

#include <set>
#include <string>

#include "me/arch.h"
#include "me/codepass.h"

namespace be {

class X64RegAlloc : public me::RegAlloc
{
private:

    bool omitFramePointer_;

public:

    enum Regs
    {
        /*
         * general purpose registers
         */
        R0, R1, R2,  R3,  R4,  R5,  R6,  R7, 
        R8, R9, R10, R11, R12, R13, R14, R15, 

        // yes, this order is correct
        RAX = R0, 
        RCX = R1, 
        RDX = R2, 
        RBX = R3,
        RSP = R4, 
        RBP = R5, 
        RSI = R6, 
        RDI = R7, 

        /*
         * XMM registers
         */
        XMM0, XMM1, XMM2,  XMM3,  XMM4,  XMM5,  XMM6,  XMM7, 
        XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15
    };

    /*
     * condition codes
     */
    enum CC
    {
        C_EQ, C_NE,
        C_L,  C_LE,
        C_G,  C_GE
    };

    enum
    {
        R_TYPE_MASK 
            = me::Op::R_BOOL
            | me::Op::R_INT8  | me::Op::R_INT16  | me::Op::R_INT32  | me::Op::R_INT64 
            | me::Op::R_UINT8 | me::Op::R_UINT16 | me::Op::R_UINT32 | me::Op::R_UINT64
            | me::Op::R_SAT8  | me::Op::R_SAT16
            | me::Op::R_USAT8 | me::Op::R_USAT16,

        F_TYPE_MASK
            = me::Op::R_REAL32| me::Op::R_REAL64
    };

    /*
     * constructor
     */

    X64RegAlloc(me::Function* function);

    /*
     * methods
     */

    virtual void process();

    void registerTargeting();

    static std::string reg2String(const me::Reg* reg);
};

} // namespace be

#endif // BE_X64_REG_ALLOC_H
