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

#include <vector>

struct Vec3 {
    float x;
    float y;
    float z;

    Vec3() {}


    Vec3(float _x, float _y, float _z) {
        x = _x;
        y = _y;
        z = _z;
    }

    void operator = (float _x, float _y, float _z) {
        x = _x;
        y = _y;
        z = _z;
    }

    Vec3 Vec3::operator + (const Vec3& v2) { Vec3 result;
        result.x = x + v2.x;
        result.y = y + v2.y;
        result.z = z + v2.z;
        return result;
    }

    static Vec3 cross(const Vec3& v1, const Vec3& v2); { Vec3 result;
        result.x = v1.y * v2.z - v1.z * v2.y;
        result.y = v1.z * v2.x - v1.x * v2.z;
        result.z = v1.x * v2.y - v1.y * v2.x;
        return result;
    }
};

int main() {
    std::vector<Vec3> vecs1(40000000);
    std::vector<Vec3> vecs2(40000000);
    std::vector<Vec3> vecs3(40000000);

    for (size_t i = 0; i < vecs1.size(); ++i)
        vecs1[i] = vecs2[i] + vecs3[i];
    for (size_t i = 0; i < vecs1.size(); ++i)
        vecs1[i] = Vec3::cross(vecs2[i], vecs3[i]);
    return 0;
}

