/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SWIFT_BOX_H
#define SWIFT_BOX_H

#include <ostream>

#include "utils/assert.h"
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

class UInt128
{
public:

    UInt128(Box* boxes, size_t numElems)
    {
        if (numElems == 2)
        {
            uint64_[0] = boxes[0].uint64_;
            uint64_[1] = boxes[1].uint64_;
        }
        else if (numElems == 4)
        {
            for (size_t i = 0; i < 4; ++i)
                uint32_[i] = boxes[i].uint32_;
        }
        else if (numElems == 8)
        {
            for (size_t i = 0; i < 8; ++i)
                uint16_[i] = boxes[i].uint16_;
        }
        else
        {
            swiftAssert(numElems == 16, "must be 16 here");
            for (size_t i = 0; i < 16; ++i)
                uint8_[i] = boxes[i].uint8_;
        }
    }

    bool operator < (const UInt128& ui128) const
    {
        if (uint64_[0] != ui128.uint64_[0])
            return uint64_[0] < ui128.uint64_[0];
        else
            return uint64_[1] < ui128.uint64_[1];
    }

    /*
     * data
     */

    union 
    {
        uint64_t uint64_[2];
        uint32_t uint32_[4];
        uint16_t uint16_[8];
        uint8_t  uint8_[16];
    };
};

//------------------------------------------------------------------------------

#endif // SWIFT_BOX_H
