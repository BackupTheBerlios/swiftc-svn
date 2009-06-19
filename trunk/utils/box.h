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
