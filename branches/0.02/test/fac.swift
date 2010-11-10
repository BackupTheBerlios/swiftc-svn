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

class Fac
    routine fac(real r) -> real result
        real n = 1.0
        result = 1.0
        while n < r
            n = n + 1.0
            result = result * n
        end
    end

    routine fac64(real64 r) -> real64 result
        real64 n = 1.0q
        result = 1.0q
        while n < r
            n = n + 1.0q
            result = result * n
        end
    end

    routine main() -> int result
        int n = 1

        # for n = 1 .. 35
        while n <= 35
            c_call print_float( ::fac(n.to_real()) )
            c_call print_double( ::fac64(n.to_real64()) )

            n = n + 1
        end

        result = 0
    end
end
