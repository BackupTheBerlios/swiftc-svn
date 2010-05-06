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

#include "be/x64typeconv.h"

#include "be/x64parser.h"

namespace be {

int meType2beType(me::Op::Type type)
{
    switch (type)
    {
        case me::Op::R_BOOL:   return X64_BOOL;

        case me::Op::R_INT8:   return X64_INT8;
        case me::Op::R_INT16:  return X64_INT16;
        case me::Op::R_INT32:  return X64_INT32;
        case me::Op::R_INT64:  return X64_INT64;
        case me::Op::R_SAT8:   return X64_SAT8;
        case me::Op::R_SAT16:  return X64_SAT16;

        case me::Op::R_UINT8:  return X64_UINT8;
        case me::Op::R_UINT16: return X64_UINT16;
        case me::Op::R_UINT32: return X64_UINT32;
        case me::Op::R_UINT64: return X64_UINT64;
        case me::Op::R_USAT8:  return X64_USAT8;
        case me::Op::R_USAT16: return X64_USAT16;

        case me::Op::R_REAL32: return X64_REAL32;
        case me::Op::R_REAL64: return X64_REAL64;

        case me::Op::R_PTR:    return X64_UINT64;
        case me::Op::R_MEM:    return X64_STACK;

        case me::Op::S_INT8:   return X64_S_INT8;
        case me::Op::S_INT16:  return X64_S_INT16;
        case me::Op::S_INT32:  return X64_S_INT32;
        case me::Op::S_INT64:  return X64_S_INT64;
        case me::Op::S_SAT8:   return X64_S_SAT8;
        case me::Op::S_SAT16:  return X64_S_SAT16;

        case me::Op::S_UINT8:  return X64_S_UINT8;
        case me::Op::S_UINT16: return X64_S_UINT16;
        case me::Op::S_UINT32: return X64_S_UINT32;
        case me::Op::S_UINT64: return X64_S_UINT64;
        case me::Op::S_USAT8:  return X64_S_USAT8;
        case me::Op::S_USAT16: return X64_S_USAT16;

        case me::Op::S_REAL32: return X64_S_REAL32;
        case me::Op::S_REAL64: return X64_S_REAL64;
    }

    swiftAssert(false, "unreachable code");
    return -1;
}

} // namespace be
