# Swift compiler framework
# Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
# 
# This framework is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 3 as published by the Free Software Foundation.
# 
# This framework is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this framework; see the file LICENSE. If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

simd class REAL

    # operators for calculating

    simd reader + (REAL r) -> REAL result
    end
    simd reader - (REAL r) -> REAL result
    end
    simd reader * (REAL r) -> REAL result
    end
    simd reader / (REAL r) -> REAL result
    end

    # operators for comparisons

    simd reader == (REAL r) -> bool result
    end
    simd reader != (REAL r) -> bool result
    end
    simd reader <  (REAL r) -> bool result
    end
    simd reader >  (REAL r) -> bool result
    end
    simd reader <= (REAL r) -> bool result
    end
    simd reader >= (REAL r) -> bool result
    end

    # unary minus
    simd reader - () -> REAL res
    end

    # normal casts

    simd reader to_int()   -> int result
    end
    simd reader to_int8()  -> int8 result
    end
    simd reader to_int16() -> int16 result
    end
    simd reader to_int32() -> int32 result
    end
    simd reader to_int64() -> int64 result
    end

    simd reader to_uint()   -> uint result
    end
    simd reader to_uint8()  -> uint8 result
    end
    simd reader to_uint16() -> uint16 result
    end
    simd reader to_uint32() -> uint32 result
    end
    simd reader to_uint64() -> uint64 result
    end

    simd reader to_sat8()  -> sat8 result
    end
    simd reader to_sat16() -> sat16 result
    end
    
    simd reader to_usat8()  -> usat8 result
    end
    simd reader to_usat16() -> usat16 result
    end

    simd reader to_real() -> real result
    end
    simd reader to_real32() -> real32 result
    end
    simd reader to_real64() -> real64 result
    end

    simd reader to_bool() -> bool result
    end
    simd reader to_index() -> index result
    end

#    iterator real each(real begin, real ending, real step = 1)
#    end

#    iterator real up(real begin, real ending, real step = 1)
#    end

#    iterator real down(real begin, real ending, real step = -1)
#    end
end
