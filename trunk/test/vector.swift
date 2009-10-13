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

        real v1x = v1.x
        real v1y = v1.y
        real v1z = v1.z
        real v2x = v2.x
        real v2y = v2.y
        real v2z = v2.z

        real resultx = v1x + v2x
        real resulty = v1y + v2y
        real resultz = v1z + v2z

        if resultx <= 2.0
            while resultx < 10.0
                if resultx < 5.0

                    while resultz <> 5.0
                        resultx = 6.0
                    end

                    if resulty > 6.0
                        resultx = 8.0
                        if resultz >= 13.5
                            resulty = 8.0
                        end
                    else
                        resultx = 4.0
                    end
                else
                    resultx = 7.0
                end

                resulty = v1z + 8.0
            end
        end

        if resultx == 4.0
            resultx = 5.0
        else
            resulty = 6.0
        end

        repeat
            if resultx == 10.0
                resultx = 10.0
            end

            resultx = resultx - 2.5
            resultx = resultx + 1.0
        until resultx > 5.0

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

        # simd[0x, 40000000x]: vecs1s = vecs2s + vecs3s

        simd[0x, 40000000x]: vecs1s = Vec3::cross(vecs2s, vecs3s)

        result = 0
    end
end
