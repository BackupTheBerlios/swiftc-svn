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

#ifndef SWIFT_SET_H
#define SWIFT_SET_H

#include <algorithm>
#include <set>
#include <vector>

template<class T>
class Set : public std::set<T>
{
public:

    inline bool contains(const T& t) const
    {
        return find(t) != this->end();
    }

    typedef std::vector<T> ResultVec;
     
    inline ResultVec difference(const Set<T>& s) const
    {
        std::vector<T> result( this->size() );

        result.erase( 
                std::set_difference( 
                    this->begin(), this->end(), 
                    s.begin(), s.end(), 
                    result.begin() ), 
                result.end() );

        return result;
    }

    inline ResultVec unite(const Set<T>& s) const
    {
        std::vector<T> result( this->size() + s.size() );

        result.erase( 
                std::set_union(
                    this->begin(), this->end(), 
                    s.begin(), s.end(), 
                    result.begin() ), 
                result.end() );

        return result;
    }
};

#endif // SWIFT_SET_H
