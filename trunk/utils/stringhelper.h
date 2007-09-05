#ifndef SWIFT_STRINGHELPER_H
#define SWIFT_STRINGHELPER_H

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

#endif // SWIFT_STRINGHELPER_H
