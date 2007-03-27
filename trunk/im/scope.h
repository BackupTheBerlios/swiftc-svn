#ifndef SWIFT_SCOPE_H
#define SWIFT_SCOPE_H

#include "../utils/list.h"

// -----------------------------------------------------------------------------

struct Scope
{
    List<Scope*> childScopes_;
};

// -----------------------------------------------------------------------------

struct Function : public Scope
{
    List<Location*> in_;
    List<Location*> inout_;
    List<Location*> out_;
};

#endif // SWIFT_SCOPE_H
