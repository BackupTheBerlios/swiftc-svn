#ifndef SWIFT_AMD64_SPILLER_H
#define SWIFT_AMD64_SPILLER_H

#include "be/spiller.h"

struct Amd64Spiller : public Spiller
{
    Amd64Spiller() {}
    virtual ~Amd64Spiller() {}

    virtual void spill();
};

#endif // SWIFT_AMD64_SPILLER_H
