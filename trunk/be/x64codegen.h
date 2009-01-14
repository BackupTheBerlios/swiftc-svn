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
};

} // namespace be

int x64lex();
void x64error(char* s);

#endif // BE_X64_CODE_GEN_H

