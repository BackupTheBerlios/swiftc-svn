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

class int
    #create (int i)
    #end

    operator + (int i1, int i2) -> int res
    end
    operator - (int i1, int i2) -> int res
    end
    operator * (int i1, int i2) -> int res
    end
    operator / (int i1, int i2) -> int res
    end

#  operator += -= *= /= (int i1)
#  end

#    operator inc dec
#    end

    operator == (int i1, int i2) -> bool res
    end
    operator <> (int i1, int i2) -> bool res
    end
    operator <  (int i1, int i2) -> bool res
    end
    operator >  (int i1, int i2) -> bool res
    end
    operator <= (int i1, int i2) -> bool res
    end
    operator >= (int i1, int i2) -> bool res
    end

    # unary minus
    operator - (int i) -> int res
    end

#    iterator int each(int begin, int ending, int step = 1)
#    end

#    iterator int up(int begin, int ending, int step = 1)
#    end

#    iterator int down(int begin, int ending, int step = -1)
#    end
end
