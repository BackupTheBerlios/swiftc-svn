#ifndef BE_X86_64_H
#define BE_X86_64_H

#include "me/arch.h"

namespace be {

class X64 : public me::Arch
{
public:

    virtual me::Op::Type getPreferedInt() const;
    virtual me::Op::Type getPreferedUInt() const;
    virtual me::Op::Type getPreferedReal() const;
    virtual me::Op::Type getPreferedIndex() const;

    virtual void regAlloc(me::Function* function);
    virtual void dumpConstants(std::ofstream& ofs);
    virtual void codeGen(me::Function* function, std::ofstream& ofs);

    virtual std::string reg2String(const me::Reg* reg) const;
};

} // namespace be

#endif // BE_X86_64_H
