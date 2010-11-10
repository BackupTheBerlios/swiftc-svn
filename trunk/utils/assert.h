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

#ifndef SWIFT_ASSERT_H
#define SWIFT_ASSERT_H

#ifdef SWIFT_DEBUG

/**
 * This is the assert function. It is used by the swiftAssert macro.
 * @param line          line number
 * @param filename      file name
 * @param functionName  function name
 * @param description   description of the assertion
 * @param always        should this assertion be ignored always? This is a kind of a return value
 * @return              true if a breakpoint should be thrown
 */
bool customAssert(int line, const char* filename, const char* functionName, const char* description, bool& always);

/*
    * SWIFT_THROW_BREAKPOINT activates a Breakpoint in debug mode.
    * Does it work with PowerPC? I have no idea. I have googled the asm instruction...
*/

//__GNUC__ is defined by all GCC versions
#ifdef __GNUC__
    #ifdef __APPLE__
        #define SWIFT_THROW_BREAKPOINT asm("trap")
    #else //Then it is a Linux system or Mingw
        #define SWIFT_THROW_BREAKPOINT asm("int $3")
    #endif //__APPLE__
#elif defined(WIN32)
    #define SWIFT_THROW_BREAKPOINT _asm{int 3}
#else //Ok, so it is an unsupported arch -> no breakpoints :(
    #define SWIFT_THROW_BREAKPOINT {}
#endif

/**
 * the assertion macro
 * @param e boolean expression. This one should be true
 * @param description a c-string. The description for the assertion.
 */
#ifdef __GNUC__
    #define swiftAssert(e, description) \
        if ( !(bool(e)) ) { \
            static bool always = false; \
            if (!always) \
                if (customAssert(__LINE__, __FILE__, __PRETTY_FUNCTION__, (description), always)) \
                    SWIFT_THROW_BREAKPOINT; \
        }
#else // __GNUC__
    #define swiftAssert(e, description) \
        if ( !(bool(e)) ) { \
            static bool always = false; \
            if (!always) \
                if (customAssert(__LINE__, __FILE__, __FUNCTION__, (description), always)) \
                    SWIFT_THROW_BREAKPOINT; \
        }
#endif // __GNUC__

#else // SWIFT_DEBUG

    /**
     * No functionality in the release version
    */
    #define swiftAssert(b, description)

#endif // SWIFT_DEBUG

#define SWIFT_PANIC_DEFAULT default: swiftAssert(false, "illegal switch-case value");
#define SWIFT_UNREACHABLE swiftAssert(false, "unreachable");

#endif // SWIFT_ASSERT_H
