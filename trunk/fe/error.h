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

#ifndef SWIFT_ERROR_H
#define SWIFT_ERROR_H

#include <cstdarg>
#include <cstring>

namespace swift {

void errorf(int line, const char* fs, ...);
void warningf(int line, const char* fs, ...);

struct ErrorHandler
{
    char* filename_;

    ErrorHandler(const char* filename)
        : filename_(0)
    {
        setFilename(filename);
    }
    ~ErrorHandler()
    {
        if (filename_)
            delete[] filename_;
    }

    void setFilename(const char* filename);
};

extern ErrorHandler* error;

} // namespace swift

#endif // SWIFT_ERROR_H
