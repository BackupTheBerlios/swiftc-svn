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


#endif // SWIFT_ASSERT_H
