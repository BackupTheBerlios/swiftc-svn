#ifndef BE_X64_CODE_GEN_H
#define BE_X64_CODE_GEN_H

#include "me/codepass.h"

namespace be {

class X64CodeGen : public me::CodePass
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
};

} // namespace be

#endif // BE_X64_CODE_GEN_H

