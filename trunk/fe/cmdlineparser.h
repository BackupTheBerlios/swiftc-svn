#ifndef SWIFT_CMDLINEPARSER_H
#define SWIFT_CMDLINEPARSER_H

/**
 * @brief This class parses the command line
*/
struct CmdLineParser
{
    int argc_;
    char** argv_;
    char* filename_;
    bool error_;

    CmdLineParser(int argc, char** argv);
};

#endif // SWIFT_CMDLINEPARSER_H
