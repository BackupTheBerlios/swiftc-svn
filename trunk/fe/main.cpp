#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "utils/memmgr.h"
#include "utils/stringhelper.h"

#include "fe/cmdlineparser.h"
#include "fe/error.h"
#include "fe/lexer.h"
#include "fe/module.h"
#include "fe/parser.h"
#include "fe/symtab.h"
#include "fe/syntaxtree.h"

#include "me/coloring.h"
#include "me/functab.h"
#include "me/defusecalc.h"
#include "me/livenessanalysis.h"
#include "me/spiller.h"

#include "be/amd64/spiller.h"

using namespace swift;

//------------------------------------------------------------------------------

// forward declarations

void readBuiltinTypes();
int start(int argc, char** argv);

//------------------------------------------------------------------------------

// inits the memory manager
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

// inits all compiler globas and controls all compiler passes
int start(int argc, char** argv)
{
    // parse the command line
    CmdLineParser cmdLineParser(argc, argv);

    if (cmdLineParser.error_)
        return EXIT_FAILURE;

    /*
        init globals
    */
    syntaxtree = new SyntaxTree();
    syntaxtree->rootModule_ = new Module(new std::string("default"), currentLine);

    symtab = new SymTab();
    symtab->insert(syntaxtree->rootModule_);

    error = new ErrorHandler(cmdLineParser.filename_);
    me::functab = new me::FuncTab(cmdLineParser.filename_); // the symbol table of the middle-end

    // populate symtab with builtin types
    readBuiltinTypes();

    // try to open the input file and init the lexer
    FILE* file = lexerInit(cmdLineParser.filename_);
    if (!file)
    {
        std::cerr << "error: failed to open input file" << std::endl;
        return EXIT_FAILURE;
    }

    /*
        Parse the input file, build a syntax tree
        and start filling the SymbolTable

        Since not all symbols can be found in the first pass there will be gaps
        in the SymbolTable.
    */
    swiftparse(); // call generated parser which on its part calls swiftlex
    if ( parseerror )
        return EXIT_FAILURE; // abort on a parse error

    /*
        Check types of all expressions, fill all gaps in the symtab and
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
    me::functab->buildUpME();
    
    /*
        build up back-end and generate assembly code
    */
    // TODO

    //std::ostringstream oss;
    //oss << cmdLineParser.filename_ << ".asm";

    //std::ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...

    /*
     * build up pipeline
     */
    for (me::FunctionTable::FunctionMap::iterator iter = me::functab->functions_.begin(); iter != me::functab->functions_.end(); ++iter)
    {
        me::Function* function = iter->second;

        me::DefUseCalc(function).process();
        me::LivenessAnalysis(function).process();
        me::Spiller(function).process();
        //me::Coloring(function).process();
    }

    /*
     * debug output
     */
    me::functab->dumpSSA();
    me::functab->dumpDot();

    // finish
    //ofs.close();

    /*
        clean up middle-end
    */
    delete me::functab;

    return 0;
}

void readBuiltinTypes()
{
    /*
        read builtin types
    */
    std::vector<const char*> builtin;

    builtin.push_back("fe/builtin/int.swift");
    builtin.push_back("fe/builtin/int8.swift");
    builtin.push_back("fe/builtin/int16.swift");
    builtin.push_back("fe/builtin/int32.swift");
    builtin.push_back("fe/builtin/int64.swift");

    builtin.push_back("fe/builtin/uint.swift");
    builtin.push_back("fe/builtin/uint8.swift");
    builtin.push_back("fe/builtin/uint16.swift");
    builtin.push_back("fe/builtin/uint32.swift");
    builtin.push_back("fe/builtin/uint64.swift");

    builtin.push_back("fe/builtin/sat8.swift");
    builtin.push_back("fe/builtin/sat16.swift");

    builtin.push_back("fe/builtin/usat8.swift");
    builtin.push_back("fe/builtin/usat16.swift");

    builtin.push_back("fe/builtin/real.swift");
    builtin.push_back("fe/builtin/real32.swift");
    builtin.push_back("fe/builtin/real64.swift");

    builtin.push_back("fe/builtin/bool.swift");

    FILE* file;

    for (size_t i = 0; i < builtin.size(); ++i)
    {
        file = lexerInit(builtin[i]);
        swiftparse();
        fclose(file);
    }
}
