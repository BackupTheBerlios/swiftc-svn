#ifndef SWIFT_ERROR_H
#define SWIFT_ERROR_H

#include <cstdarg>
#include <cstring>

namespace swift
{

void errorf(int line, const char* fs, ...);
void warningf(int line, const char* fs, ...);

struct ErrorHandler
{
    char* filename_;

    ErrorHandler()
        : filename_(0)
    {}
    ~ErrorHandler()
    {
        if (filename_)
            delete filename_;
    }

    void setFilename(const char* filename);
};

extern ErrorHandler error;

}

#endif // SWIFT_ERROR_H
