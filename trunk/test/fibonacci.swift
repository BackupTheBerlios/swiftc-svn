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

class Fibonacci

    # iterative implementation
    routine fibonacci(int n) -> int result
        if n == 0
            result = 0
            return
        end

        result = 1

        if n == 1
            return
        end

        int pre = 0
        int counter = n

        while counter >= 2
            int f = result
            result = result + pre
            pre = f

            counter = counter - 1
        end

    end

    # recursive implementation
    routine fibonacci_rec(int n) -> int result
        if n == 0
            result = 0
            return
        end

        if n == 1
            result = 1
            return
        end
        
        result = ::fibonacci_rec(n - 1) + ::fibonacci_rec(n - 2)
    end

    # print fibonacci(0) till fibonacci(13) with the iterative and the recursive implementation
    routine main() -> int result
        int n = 0

        # for n = 0 .. 13
        while n <= 13
            c_call print_int( ::fibonacci(n) )
            c_call print_int( ::fibonacci_rec(n) )

            n = n + 1
        end

        result = 0
    end
end
