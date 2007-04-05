#include "error.h"

#include <cstdio>

#include "utils/assert.h"


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
