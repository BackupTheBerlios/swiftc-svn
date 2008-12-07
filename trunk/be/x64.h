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
};

} // namespace be

#endif // BE_X86_64_H
