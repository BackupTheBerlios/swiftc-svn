#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "utils/memmgr.h"
#include "utils/stringhelper.h"

#include "fe/cmdlineparser.h"
#include "fe/error.h"
#include "fe/lexer.h"
#include "fe/parser.h"
#include "fe/symtab.h"
#include "fe/syntaxtree.h"

#include "me/functab.h"
#include "me/optimizer.h"

#include "be/codegenerator.h"
#include "be/amd64/spiller.h"

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
    functab = new FuncTab(cmdLineParser.filename_); // the symbol table of the middle-end

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
return 0;
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
        optimize if applicable
    */
    if (cmdLineParser.optimize_)
    {
        Optimizer::commonSubexprElimination_    = true;
        Optimizer::deadCodeElimination_         = true;
        Optimizer::constantPropagation_         = true;

        for (FunctionTable::FunctionMap::iterator iter = functab->functions_.begin(); iter != functab->functions_.end(); ++iter)
        {
            Optimizer optimizer(iter->second);
            optimizer.optimize();
        }
    }


    /*
        debug output
    */
    functab->dumpSSA();
    functab->dumpDot();

    /*
        build up back-end and generate assembly code
    */
    CodeGenerator::spiller_ = new Amd64Spiller();

    std::ostringstream oss;
    oss << cmdLineParser.filename_ << ".asm";

    std::ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...

    for (FunctionTable::FunctionMap::iterator iter = functab->functions_.begin(); iter != functab->functions_.end(); ++iter)
    {
        CodeGenerator cg(ofs, iter->second);
        cg.genCode();
    }

    // finish
    ofs.close();

    /*
        clean up middle-end
    */
    delete functab;

    /*
        clean up back-end
    */
    delete CodeGenerator::spiller_;

    return 0;
}

void readBuiltinTypes()
{
    gencode = false;

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

    gencode = true;
}
