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

#include "error.h"

#include <cstdio>

#include "utils/assert.h"

namespace swift {

ErrorHandler* error = 0;

void ErrorHandler::setFilename(const char* filename)
{
    if (filename_)
        delete filename_;

    filename_ = new char[strlen(filename) + 1]; // alloc one more for the NULL-terminator
    strcpy(filename_, filename);
}

void errorf(int line, const char* fs, ...)
{
    swiftAssert(error, "error is null");

    fprintf(stderr, "%s:%i: error: ", error->filename_, line);

    va_list argptr;
    va_start(argptr, fs);

    vfprintf(stderr, fs, argptr);
    va_end(argptr);
    fprintf(stderr, "\n");
}

void warningf(int line, const char* fs, ...)
{
    swiftAssert(error, "error is null");

    fprintf(stderr, "%s:%i: warning: ", error->filename_, line);

    va_list argptr;
    va_start(argptr, fs);

    vfprintf(stderr, fs, argptr);
    va_end(argptr);
    fprintf(stderr, "\n");
}

} // namespace swift
