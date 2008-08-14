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
