#include <iostream>
#include <cstdio>

#include "utils/memmgr.h"
#include "utils/stringhelper.h"

#include "fe/cmdlineparser.h"
#include "fe/error.h"
#include "fe/lexer.h"
#include "fe/parser.h"
#include "fe/symtab.h"
#include "fe/syntaxtree.h"

#include "me/functab.h"


int start(int argc, char** argv)
{
    // parse the command line
    CmdLineParser cmdLineParser(argc, argv);

    if (cmdLineParser.error_)
        return EXIT_FAILURE;

    // try to open the input file and init the lexer
    FILE* file = lexerInit(argv[1]);
    if (!file)
    {
        std::cerr << "error: failed to open input file" << std::endl;
        return EXIT_FAILURE;
    }

    /*
        init globals
    */
    syntaxtree = new SyntaxTree();
    symtab = new SymTab();
    error = new ErrorHandler(cmdLineParser.filename_);
    functab = new FuncTab(cmdLineParser.filename_); // the symbol table of the middle-end

    /*
        1.  Parse the input file, build a syntax tree
            and  start filling the SymbolTable

        Since not all symbols can be found in the first pass there will be gaps
        in the SymbolTable.
    */
    swiftparse();   // call generated parser which on its part calls swiftlex
    if ( parseerror )
        return EXIT_FAILURE;  // abort on a parse error

    /*
        2.  Check types of all expressions, fill all gaps in the symtab and
            build single static assignment form if everything is ok

        Thus a complete SymbolTable and type consistency is ensured after this pass.
    */
    bool analyzeResult = syntaxtree->analyze();

    /*
        clean up front-end
    */
    delete syntaxtree;
    delete symtab;
    delete error;

    fclose(file);

    if (analyzeResult == false)
    {
        std::cerr << "error" << std::endl;
        return EXIT_FAILURE; // abort on error
    }

    /*
        build up middle-end:

        find basic blocks,
        caculate control flow graph,
        find last assignment of vars in a basic block
        calculate the dominator tree,
        calculate the dominance frontier,
        place phi-functions in SSA form and update vars
    */
    functab->buildUpME();

    /*
        debug output
    */
    functab->dumpSSA();
    functab->dumpDot();

    /*
        clean up middle-end
    */
    delete functab;

    return 0;
}

int main(int argc, char** argv)
{
#ifdef SWIFT_DEBUG
    // find memory leaks
    MemMgr::init();
#endif // SWIFT_DEBUG

//     MemMgr::setBreakpoint(673);
    int result = start(argc, argv);

#ifdef SWIFT_DEBUG
    MemMgr::deinit();
#endif // SWIFT_DEBUG

    return result;
}

