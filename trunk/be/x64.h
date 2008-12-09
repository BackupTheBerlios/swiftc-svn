#ifndef BE_X86_64_H
#define BE_X86_64_H

#include "be/arch.h"

namespace be {

class X64 : public Arch
{
public:

    virtual me::Op::Type getPreferedInt() const;
    virtual me::Op::Type getPreferedUInt() const;
    virtual me::Op::Type getPreferedReal() const;
    virtual me::Op::Type getPreferedIndex() const;

    virtual void regAlloc(me::Function* function);
    virtual void codeGen(me::Function* function);

    virtual std::string color2String(int color) const;
};

} // namespace be

#endif // BE_X86_64_H
