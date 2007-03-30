#ifndef SWIFT_STRINGPTRCMP
#define SWIFT_STRINGPTRCMP

#include <string>

struct StringPtrCmp
{
    bool operator () (const std::string* str1, const std::string* str2)
    {
        return *str1 < *str2;
    }
};

#endif // SWIFT_STRINGPTRCMP
