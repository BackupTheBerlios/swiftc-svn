#ifndef BE_X86_64_SPILLER_H
#define BE_X86_64_SPILLER_H

#include "be/spiller.h"

namespace be {

struct X86_64Spiller : public Spiller
{
    X86_64Spiller() {}
    virtual ~X86_64Spiller() {}

    virtual void spill();
};

} // namespace be

#endif // BE_AMD64_SPILLER_H
