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
        R00,
        R01, 
        R02, 
        R03, 
        R04,
        R05, 
        R06, 
        R07, 

        RAX = R00, 
        RBX = R01, 
        RCX = R02, 
        RDX = R03, 
        RBP = R04, 
        RSI = R05, 
        RDI = R06, 
        RSP = R07, 

        R08, 
        R09, 
        R10, 
        R11, 
        R12, 
        R13, 
        R14, 
        R15, 

        XMM00,
        XMM01, 
        XMM02, 
        XMM03, 
        XMM04, 
        XMM05, 
        XMM06, 
        XMM07, 
        XMM08, 
        XMM09, 
        XMM10, 
        XMM11, 
        XMM12, 
        XMM13, 
        XMM14, 
        XMM15,
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
            = me::Op::R_REAL32| me::Op::R_REAL64,

        V_TYPE_MASK
            = me::Op::R_BOOL
            | me::Op::R_INT8  | me::Op::R_INT16  | me::Op::R_INT32  | me::Op::R_INT64 
            | me::Op::R_UINT8 | me::Op::R_UINT16 | me::Op::R_UINT32 | me::Op::R_UINT64
            | me::Op::R_SAT8  | me::Op::R_SAT16
            | me::Op::R_USAT8 | me::Op::R_USAT16
            | me::Op::R_REAL32| me::Op::R_REAL64
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
    void insertNOP(me::InstrNode* instrNode);

    static std::string reg2String(int reg);
};

} // namespace be

#endif // BE_X64_REG_ALLOC_H
