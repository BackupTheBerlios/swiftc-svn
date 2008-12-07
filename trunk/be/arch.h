#ifndef BE_ARCH_H
#define BE_ARCH_H

#include "me/codepass.h"
#include "me/op.h"

namespace be {

class RegAlloc : public me::CodePass
{};

class CodeGen : public me::CodePass
{};

class Arch
{
public:

    static RegAlloc* regAlloc_;
    static CodeGen* codeGen_;

    virtual me::Op::Type getPreferedInt() const = 0;
    virtual me::Op::Type getPreferedUInt() const = 0;
    virtual me::Op::Type getPreferedReal() const = 0;
    virtual me::Op::Type getPreferedIndex() const = 0;
};

} // namespace be

#endif // BE_ARCH_H
