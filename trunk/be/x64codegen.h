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
    void genPhiInstr(me::BBNode* prevNode, me::BBNode* nextNode);
    void genMove(me::Op::Type type, int r1, int r2);
};

} // namespace be

int x64lex();
void x64error(char* s);

#endif // BE_X64_CODE_GEN_H

