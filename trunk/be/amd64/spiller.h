#ifndef BE_AMD64_SPILLER_H
#define BE_AMD64_SPILLER_H

#include "be/spiller.h"

namespace be {

struct AMD64Spiller : public Spiller
{
    AMD64Spiller() {}
    virtual ~AMD64Spiller() {}

    virtual void spill();
};

} // namespace be

#endif // BE_AMD64_SPILLER_H
