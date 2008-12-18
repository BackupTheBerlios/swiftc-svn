#ifndef ME_ARCH_H
#define ME_ARCH_H

#include "me/codepass.h"
#include "me/op.h"

namespace me {

//------------------------------------------------------------------------------

class RegAlloc : public CodePass
{
public:

    RegAlloc(Function* function);

    void faithfulFix(InstrNode* instrNode, int typeMask, int numRegs);
};

//------------------------------------------------------------------------------

class CodeGen : public CodePass
{
public:

    CodeGen(Function* function);
};

//------------------------------------------------------------------------------

class Arch
{
public:

    virtual Op::Type getPreferedInt() const = 0;
    virtual Op::Type getPreferedUInt() const = 0;
    virtual Op::Type getPreferedReal() const = 0;
    virtual Op::Type getPreferedIndex() const = 0;

    virtual void regAlloc(Function* function) = 0;
    virtual void codeGen(Function* function) = 0;

    virtual std::string color2String(int color) const = 0;
};

//------------------------------------------------------------------------------

extern Arch* arch;

} // namespace me

#endif // ME_ARCH_H
