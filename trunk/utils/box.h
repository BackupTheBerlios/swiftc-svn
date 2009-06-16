#ifndef SWIFT_BOX_H
#define SWIFT_BOX_H

#include "utils/types.h"

union Box
{
    size_t   size_;

    int      int_;
    int8_t   int8_;
    int16_t  int16_;
    int32_t  int32_;
    int64_t  int64_;
    int8_t   sat8_;
    int16_t  sat16_;

    uint     uint_;
    uint8_t  uint8_;
    uint16_t uint16_;
    uint32_t uint32_;
    uint64_t uint64_;
    uint8_t  usat8_;
    uint16_t usat16_;

    float    float_;
    double   double_;

    bool     bool_;

    void*    ptr_;
};

//------------------------------------------------------------------------------

template<class From, class To>
To convert(From from)
{
    union Converter
    {
        From from_;
        To to_;
    };

    Converter conv;
    conv.from_ = from;

    return conv.to_;
}

//------------------------------------------------------------------------------

#endif // SWIFT_BOX_H
