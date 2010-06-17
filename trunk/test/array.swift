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

class Foo
    int i
    real r
end

simd class Vec3
    real x
    real y
    real z
end

class Test
    routine foo() -> int a, int b
    end

    routine main() -> int result
        array{index} a = 20x
        index i = 0x

        # write
        while i < 20x
            a[i] = i
            i = i + 1x
        end

        # copy over
        array{index} b = a

        i = 0x
        while i < 20x
            c_call print_int( b[i]:to_int() )
            i = i + 1x
        end

        c_call print_int(1111111111)

        array{Foo} af = 20x

        # and backwards
        i = 19x
        while i <= 19x
            af[19x - i].i = i:to_int() # fill increasingly
            af[i].r = i:to_real()      # fill decreasingly
            i = i - 1x
        end

        i = 0x
        while i < 20x
            c_call print_int( af[i].i )
            c_call print_float( af[i].r )
            i = i + 1x
        end

        result = 0
    end
end
