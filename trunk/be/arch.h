#ifndef BE_ARCH_H
#define BE_ARCH_H

#include "me/codepass.h"
#include "me/op.h"

// forward declarations
namespace me {
    class Function;
} // namespace me

namespace be {

class RegAlloc : public me::CodePass
{
public:
    RegAlloc(me::Function* function);
};

class CodeGen : public me::CodePass
{
public:
    CodeGen(me::Function* function);
};

class Arch
{
public:

    virtual me::Op::Type getPreferedInt() const = 0;
    virtual me::Op::Type getPreferedUInt() const = 0;
    virtual me::Op::Type getPreferedReal() const = 0;
    virtual me::Op::Type getPreferedIndex() const = 0;

    virtual void regAlloc(me::Function* function) = 0;
    virtual void codeGen(me::Function* function) = 0;
};

} // namespace be

#endif // BE_ARCH_H
