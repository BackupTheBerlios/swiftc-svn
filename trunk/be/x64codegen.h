#ifndef BE_X64_CODE_GEN_H
#define BE_X64_CODE_GEN_H

#include <fstream>

#include "me/arch.h"
#include "me/codepass.h"

namespace be {

class X64CodeGen : public me::CodeGen
{
public:

    /*
     * constructor
     */

    X64CodeGen(me::Function* function, std::ofstream& ofs);

    /*
     * further methods
     */

    virtual void process();

private:

    void genFunctionEntry();

    void genFunctionExit();

    void genAsmInstr(me::InstrBase* instr);

    void genPhiInstr();

    void genMemPhiInstr();

    static char type2postfix(me::Op::Type type);
};

} // namespace be

#endif // BE_X64_CODE_GEN_H

