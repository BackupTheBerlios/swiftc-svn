#include "stringhelper.h"

#include <sstream>

std::string number2String(int number)
{
    std::ostringstream oss;

    for (int limit = 100; limit != 1; limit /= 10)
    {
        if (number < limit)
            oss << '0';
        else
            break;
    }

    oss << number;

    return oss.str();
}
