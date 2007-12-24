#ifndef SWIFT_X86_64_SPILLER_H
#define SWIFT_X86_64_SPILLER_H

#include "be/spiller.h"

struct X86_64Spiller : public Spiller
{
    X86_64Spiller() {}
    virtual ~X86_64Spiller() {}

    virtual void spill();
};

#endif // SWIFT_AMD64_SPILLER_H
