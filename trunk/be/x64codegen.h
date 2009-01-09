#ifndef BE_X64_CODE_GEN_H
#define BE_X64_CODE_GEN_H

#include <fstream>
#include <string>

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

    void genAsmInstr(me::InstrBase* instr);
    void genGeneralPurposeInstr(me::InstrBase* instr);
    void genFloatingPointInstr(me::InstrBase* instr);

    void genPhiInstr();

    void genMemPhiInstr();

    static std::string type2postfix(me::Op::Type type);
    static std::string op2string(me::Op* op);
};

} // namespace be

#endif // BE_X64_CODE_GEN_H

