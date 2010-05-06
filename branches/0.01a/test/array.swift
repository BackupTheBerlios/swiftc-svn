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
    index x
    real r
end

class Test
    routine main() -> int result
        array{index} a = 20x
        index i = 0x

        # write
        while i < 20x
            a[i] = i
            i = i + 1x
        end

        i = 0x
        # load and print
        while i < 20x
            c_call print_int( a[i] )
            i = i + 1x
        end

        index sum = 0x
        i = 0x

        # sum up
        while i < 20x
            sum = sum + a[i]
            i = i + 1x
        end

        c_call print_int(sum)

        # array{Foo} af = 20x

        # i = 0x
        # # load and print
        # # write
        # while i < 20x
        #     af[i].x = i
        #     af[i].r = 5.0
        #     i = i + 1x
        # end

        # i = 0x
        # # load and print
        # while i < 20x
        #     c_call print_float( af[i].r )
        #     i = i + 1x
        # end

    end
end
