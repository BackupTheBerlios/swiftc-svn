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

#include "assert.h"

#ifdef SWIFT_DEBUG

#include <cstdlib>

#include <sstream>
#include <iostream>


bool customAssert(int line, const char* filename, const char* functionName, const char* description, bool& always)
{
    std::cout << "Assertion: " << filename << " " << line << ": " << std::endl << functionName << std::endl;
    std::cout << "   " << description << std::endl;

    do
    {
        std::cout << "   (b)reakpoint, (i)gnore, ignore (a)lways, (q)uit" << std::endl;
        std::string inStr;
        std::cin >> inStr;

        switch (inStr[0])
        {
            case 'b':
                return true;
            case 'i':
                return false;
            case 'a':
                always = true;
                return false;
            case 'q':
                exit(EXIT_FAILURE);
            default:
                std::cout << "An assertion in an assertion... :(" << std::endl;
        }
    } while (true);
}

#endif // swift_DEBUG
