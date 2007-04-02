#ifndef SWIFT_STRUCT_H
#define SWIFT_STRUCT_H

#include <string>

#inlude "pseudoreg.h"

struct Struct
{
    struct Member
    {
        union
        {
            PseudoReg::RegType* regType_;
            Struct* struct_;
        };

        bool simpleType_;
    };

    std::string* id_;
};

#endif SWIFT_STRUCT_H
