#ifndef ME_STRUCT_H
#define ME_STRUCT_H

// TODO

#include <string>

#include "pseudoreg.h"

namespace me {

struct Struct
{
    struct Member
    {
        union
        {
            Op::Type* type_;
            Struct* struct_;
        };

        bool simpleType_;
    };

    std::string* id_;
};

} // namespace me

#endif ME_STRUCT_H
