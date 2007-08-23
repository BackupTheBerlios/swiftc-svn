#include "cmdlineparser.h"

#include <iostream>

CmdLineParser::CmdLineParser(int argc, char** argv)
    : argc_(argc)
    , argv_(argv)
    , error_(false)
    , optimize_(false)
{
    switch (argc)
    {
        case 0:
        case 1:
            std::cerr << "error: no input file specified" << std::endl;
            error_ = true;
            return;

        case 2:
            filename_ = argv[1];
            break;

        default:
            std::cerr << "error: too many arguments" << std::endl;
            error_ = true;
            return;
    }

}
