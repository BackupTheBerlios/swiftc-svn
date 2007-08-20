#ifndef SWIFT_CFG_H
#define SWIFT_CFG_H

#include "utils/graph"

#include "me/basicblock.h"

struct CFG : public Graph<BB>
{
    virtual std::string name() const
    {
        return std::string("hallo");
    }
};

#endif // SWIFT_CFG_H
