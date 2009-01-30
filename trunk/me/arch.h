#ifndef ME_ARCH_H
#define ME_ARCH_H

#include <fstream>

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
protected:

    std::ofstream& ofs_;

public:

    CodeGen(Function* function, std::ofstream& ofs);
};

//------------------------------------------------------------------------------

class Arch
{
public:

    virtual Op::Type getPreferedInt() const = 0;
    virtual Op::Type getPreferedUInt() const = 0;
    virtual Op::Type getPreferedReal() const = 0;
    virtual Op::Type getPreferedIndex() const = 0;
    virtual int getPtrSize() const = 0;

    virtual void regAlloc(Function* function) = 0;
    virtual void dumpConstants(std::ofstream& ofs) = 0;
    virtual void codeGen(Function* function, std::ofstream& ofs) = 0;

    virtual std::string reg2String(const Reg* reg) const = 0;
};

//------------------------------------------------------------------------------

extern Arch* arch;

} // namespace me

#endif // ME_ARCH_H
