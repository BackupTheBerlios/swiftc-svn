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

#include "vector.h"

Vec3 Vec3::operator + (const Vec3& v2) 
{ 
    Vec3 result;
    result.x = x + v2.x;
    result.y = y + v2.y;
    result.z = z + v2.z;

    return result;
}

Vec3 Vec3::operator + (float r) 
{ 
    Vec3 result;
    result.x = x + r;
    result.y = y + r;
    result.z = z + r;

    if (result.x < 5.0)
        result.x = 4.0;
    else
        result.x = 7.0;

    return result;
}

Vec3 Vec3::cross(const Vec3& v1, const Vec3& v2) 
{ 
    Vec3 result;
    result.x = v1.y * v2.z - v1.z * v2.y;
    result.y = v1.z * v2.x - v1.x * v2.z;
    result.z = v1.x * v2.y - v1.y * v2.x;
    return result;
}
