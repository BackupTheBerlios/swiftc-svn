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

/*
 * compile with:
 * $ g++ vector.cpp vector_main.cpp
 * or:
 * $ g++ vector.cpp vector_main.cpp -O3
 */

#include <vector>

#include "vector.h"

int main() {
    std::vector<Vec3> vecs1(40000000);
    std::vector<Vec3> vecs2(40000000);
    std::vector<Vec3> vecs3(40000000);

    for (size_t i = 0; i < vecs1.size(); ++i)
        vecs1[i] = vecs2[i] + vecs3[i];
    for (size_t i = 0; i < vecs1.size(); ++i)
        vecs1[i] = Vec3::cross(vecs2[i], vecs3[i]);
    //for (size_t i = 0; i < vecs1.size(); ++i)
        //vecs1[i] = vecs2[i] + static_cast<float>(i);

    return 0;
}
