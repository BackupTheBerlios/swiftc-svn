#ifndef SWIFT_CMDLINEPARSER_H
#define SWIFT_CMDLINEPARSER_H

namespace swift {

/**
 * @brief This class parses the command line
 */
struct CmdLineParser
{
    int argc_;
    char** argv_;
    char* filename_;
    bool error_;
    bool optimize_;

    CmdLineParser(int argc, char** argv);
};

} // namespace swift

#endif // SWIFT_CMDLINEPARSER_H
