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





simd class Vec3
    real x
    real y
    real z

    create ()
    end

    create (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end

    assign (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end

    simd operator + (Vec3 v1, Vec3 v2) -> Vec3 result
        # result.x = v1.x + v2.x
        # result.y = v1.y + v2.y
        # result.z = v1.z + v2.z

        real resultx = v1.x + v2.x
        real resulty = v1.y + v2.y
        real resultz = v1.z + v2.z

        result.x = resultx
        result.y = resulty
        result.z = resultz
    end

    simd routine cross(Vec3 v1, Vec3 v2) -> Vec3 result
        #result.x = v1.y * v2.z - v1.z * v2.y
        #result.y = v1.z * v2.x - v1.x * v2.z
        #result.z = v1.x * v2.y - v1.y * v2.x
        
        real v1x = v1.x
        real v1y = v1.y
        real v1z = v1.z
        real v2x = v2.x
        real v2y = v2.y
        real v2z = v2.z

        real resultx = v1y * v2z - v1z * v2y
        real resulty = v1z * v2x - v1x * v2z
        real resultz = v1x * v2y - v1y * v2x

        result.x = resultx
        result.y = resulty
        result.z = resultz
    end


    routine main() -> int result
        simd{Vec3} vecs1s = 40000000x
        simd{Vec3} vecs2s = 40000000x
        simd{Vec3} vecs3s = 40000000x

        simd[0x, 40000000x]: vecs1s = vecs2s + vecs3s

        simd[0x, 40000000x]: vecs1s = Vec3::cross(vecs2s, vecs3s)

        result = 0
    end
end
