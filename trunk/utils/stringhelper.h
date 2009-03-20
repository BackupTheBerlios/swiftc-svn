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

#ifndef SWIFT_STRINGHELPER_H
#define SWIFT_STRINGHELPER_H

#include <sstream>
#include <string>

/**
 * Use this function object in order to compare string pointer properly.
 * This is usefull with STL classes.
 */
struct StringPtrCmp
{
    bool operator () (const std::string* str1, const std::string* str2) const
    {
        return *str1 < *str2;
    }
};

/// Builds a string from a number which has at least 4 digits
std::string number2String(int number);

template<class T>
std::string commaList(T begin, T end)
{
    std::ostringstream oss;

    while (begin != end)
    {
        oss << (*begin).toString() << ", ";
        ++begin;
    }

    std::string result = oss.str();

    if ( !result.empty() )
        result = result.substr(0, result.size() - 2);

    return result;
}

template<class T>
std::string commaListPtr(T begin, T end)
{
    std::ostringstream oss;

    while (begin != end)
    {
        oss << (*begin)->toString() << ", ";
        ++begin;
    }

    std::string result = oss.str();

    if ( !result.empty() )
        result = result.substr(0, result.size() - 2);

    return result;
}

#endif // SWIFT_STRINGHELPER_H
