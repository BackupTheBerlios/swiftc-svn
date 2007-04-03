#include <iostream>
#include <cstdio>

#include "utils/memmgr.h"

#include "fe/cmdlineparser.h"
#include "fe/error.h"
#include "fe/lexer.h"
#include "fe/parser.h"
#include "fe/syntaxtree.h"


int start(int argc, char** argv)
{
    // parse the command line
    CmdLineParser cmdLineParser(argc, argv);

    if (cmdLineParser.error_)
        return -1;

    // try to open the input file and init the lexer
    FILE* file = lexerInit(argv[1]);
    if (!file)
    {
        std::cerr << "error: failed to open input file" << std::endl;
        return -1;
    }

    error.setFilename(argv[1]);

    /*
        1.  Parse the input file, build a syntax tree
            and  start filling the SymbolTable

        Since not all symbols can be found in the first pass there will be gaps
        in the SymbolTable.
    */
    swiftparse(); // call generated parser which on its part calls swiftlex
    // if there is a parse error continue with 2. anyway

    /*
        2.  Check types of all expressions, fill all gaps in the symtab and
            build single static assignment form if everything is ok

        Thus a complete SymbolTable and type consistency is ensured after this pass.
    */
    if ( syntaxtree.analyze() )
        return -1; // abort on error

    if ( parseerror )
    {
        std::cerr << "error while parsing... aborting" << std::endl;
        return -1; // now abort on a parse error
    }

    // the Syntaxtree is not needed anymore.
    syntaxtree.destroy();
    // close the input file
    fclose(file);

    /*
        find basic blocks and calculate next usage of names
    */
    // TODO

    /*
        4.  Optional pass to optimize the 3 address code.
    */
    //optimize3AddrCode();

    /*
        5.  Finally write assembly language code.
    */
    //buildAssemblyCode();

//     std::cout << compiler.toString() << std::endl;

    return 0;
}

int main(int argc, char** argv)
{
#ifdef SWIFT_DEBUG
    // find memory leaks
    MemMgr::init();
#endif // SWIFT_DEBUG

    int result = start(argc, argv);

#ifdef SWIFT_DEBUG
    MemMgr::deinit();
#endif // SWIFT_DEBUG

    return result;
}
