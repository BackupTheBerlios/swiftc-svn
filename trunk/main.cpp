#include <iostream>
#include <cstdio>

#include <vector>

#include "utils/memmgr.h"
#include "fe/error.h"
#include "fe/lexer.h"
#include "fe/parser.h"
#include "compiler.h"

FILE* file;

std::string getClass();

int start(int argc, char** argv)
{
    FILE* file;

    switch (argc)
    {
        case 0:
        case 1:
            std::cerr << "error: no input file specified" << std::endl;
            return -1;

        case 2:
            file = lexerInit(argv[1]);
            break;

        default:
            std::cerr << "error: too many arguments" << std::endl;
            return -1;
    }

    if (!file)
    {
        std::cout << "error: failed to open input file" << std::endl;
        return -1;
    }

    Compiler compiler;
    error.setFilename(argv[1]);

    /*
        1.  Parse the input file, build a syntax tree
            and  start filling the SymbolTable

        Since not all symbols can be found in the first pass there will be gaps
        in the SymbolTable.
    */
    if ( !compiler.parse() )
    {
        std::cout << "error while parsing... aborting" << std::endl;
        return -1;
    }

    /*
        2.  Check types of all expressions, fill all gaps in the SymbolTable and evaluate all def-expressions.

        Thus a complete SymbolTable and type consistency is ensured after this pass.
    */
    if ( !compiler.analyze() )
        return -1;

    /*
        find basic blocks and calculate next usage of names
    */
//     compiler.prepareCodeGen();

    /*
        3.  Go through the Syntaxtree and build single static assignment form
    */
    compiler.genCode();

    // The Syntaxtree is not needed anymore.
    //delete SyntaxTree;

    /*
        4.  Optional pass to optimize the 3 address code.
    */
    //optimize3AddrCode();

    /*
        5.  Finally write assembly language code.
    */
    //buildAssemblyCode();

//     do
//     {
//         yylex();
//     } while (feof(file) != 0);

    fclose(file);

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

