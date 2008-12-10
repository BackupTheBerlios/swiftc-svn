#ifndef BE_X64_CODE_GEN_H
#define BE_X64_CODE_GEN_H

#include "me/arch.h"
#include "me/codepass.h"

namespace be {

class X64CodeGen : public me::CodeGen
{
public:

    /*
     * constructor
     */

    X64CodeGen(me::Function* function);

    /*
     * methods
     */

    virtual void process();

    void genAsmInstr(me::InstrBase* instr);
};

} // namespace be

#endif // BE_X64_CODE_GEN_H

