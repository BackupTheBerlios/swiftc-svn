#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "utils/memmgr.h"
#include "utils/stringhelper.h"

#include "fe/cmdlineparser.h"
#include "fe/error.h"
#include "fe/expr.h"
#include "fe/lexer.h"
#include "fe/module.h"
#include "fe/parser.h"
#include "fe/symtab.h"
#include "fe/syntaxtree.h"
#include "fe/type.h"

#include "me/constpool.h"
#include "me/functab.h"
#include "me/defusecalc.h"
#include "me/livenessanalysis.h"

#include "be/x64.h"

using namespace swift;

//------------------------------------------------------------------------------

// forward declarations

void readBuiltinTypes();
void initTypeMaps();
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
     * init globals
     */
    me::arch = new be::X64();
    initTypeMaps();

    syntaxtree = new SyntaxTree();
    syntaxtree->rootModule_ = new Module(new std::string("default"), currentLine);

    symtab = new SymTab();
    symtab->insert(syntaxtree->rootModule_);

    error = new ErrorHandler(cmdLineParser.filename_);
    me::functab = new me::FuncTab(cmdLineParser.filename_); // the symbol table of the middle-end

    me::constpool = new me::ConstPool();

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
     * Parse the input file, build a syntax tree
     * and start filling the SymbolTable
     *
     * Since not all symbols can be found in the first pass there will be gaps
     * in the SymbolTable.
     */
    swiftparse(); // call generated parser which on its part calls swiftlex
    if ( parseerror )
        return EXIT_FAILURE; // abort on a parse error

    /*
     * Check types of all expressions, fill all gaps in the symtab and
     * build single static assignment form if everything is ok
     * 
     * Thus a complete SymbolTable and type consistency is ensured after this pass.
     */
    bool analyzeResult = syntaxtree->analyze();

    /*
     * clean up front-end
     */
    delete syntaxtree;
    delete symtab;
    delete error;
    delete Literal::typeMap_;
    delete BaseType::typeMap_;

    fclose(file);

    if (analyzeResult == false)
    {
        std::cerr << "error" << std::endl;
        return EXIT_FAILURE; // abort on error
    }

    /*
     * build up middle-end:
     *
     * find basic blocks,
     * caculate control flow graph,
     * find last assignment of vars in a basic block
     * calculate the dominator tree,
     * calculate the dominance frontier,
     * place phi-functions in SSA form and update vars
     */
    me::functab->buildUpME();
    
    /*
     * build up back-end and generate assembly code
     */

    std::ostringstream oss;
    oss << cmdLineParser.filename_ << ".asm";
    std::ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...

    /*
     * build up pipeline
     */
    for (me::FunctionTable::FunctionMap::iterator iter = me::functab->functions_.begin(); iter != me::functab->functions_.end(); ++iter)
    {
        me::Function* function = iter->second;

        me::DefUseCalc(function).process();
        me::LivenessAnalysis(function).process();
        me::arch->regAlloc(function);
        me::arch->codeGen(function, ofs);
    }

    /*
     * debug output
     */
    me::functab->dumpSSA();
    me::functab->dumpDot();

    // finish
    ofs.close();

    /*
     * clean up middle-end
     */
    delete me::functab;
    delete me::constpool;

    /*
     * clean up back-end
     */
    delete me::arch;

    return 0;
}

void readBuiltinTypes()
{
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

void initTypeMaps()
{
    BaseType::typeMap_ = new BaseType::TypeMap();

    (*BaseType::typeMap_)["bool"]   = me::Op::R_BOOL;

    (*BaseType::typeMap_)["int8"]   = me::Op::R_INT8;
    (*BaseType::typeMap_)["int16"]  = me::Op::R_INT16;
    (*BaseType::typeMap_)["int32"]  = me::Op::R_INT32;
    (*BaseType::typeMap_)["int64"]  = me::Op::R_INT64;

    (*BaseType::typeMap_)["sat8"]   = me::Op::R_SAT8;
    (*BaseType::typeMap_)["sat16"]  = me::Op::R_SAT16;

    (*BaseType::typeMap_)["uint8"]  = me::Op::R_UINT8;
    (*BaseType::typeMap_)["uint16"] = me::Op::R_UINT16;
    (*BaseType::typeMap_)["uint32"] = me::Op::R_UINT32;
    (*BaseType::typeMap_)["uint64"] = me::Op::R_UINT64;

    (*BaseType::typeMap_)["usat8"]   = me::Op::R_USAT8;
    (*BaseType::typeMap_)["usat16"]  = me::Op::R_USAT16;

    (*BaseType::typeMap_)["real32"] = me::Op::R_REAL32;
    (*BaseType::typeMap_)["real64"] = me::Op::R_REAL64;

    (*BaseType::typeMap_)["int"]    = me::arch->getPreferedInt();
    (*BaseType::typeMap_)["uint"]   = me::arch->getPreferedUInt();
    (*BaseType::typeMap_)["index"]  = me::arch->getPreferedIndex();
    (*BaseType::typeMap_)["real"]   = me::arch->getPreferedReal();


    Literal::typeMap_ = new Literal::TypeMap();

    (*Literal::typeMap_)[L_TRUE]   = me::Op::R_BOOL;
    (*Literal::typeMap_)[L_FALSE]  = me::Op::R_BOOL;

    (*Literal::typeMap_)[L_INT8]   = me::Op::R_INT8;
    (*Literal::typeMap_)[L_INT16]  = me::Op::R_INT16;
    (*Literal::typeMap_)[L_INT32]  = me::Op::R_INT32;
    (*Literal::typeMap_)[L_INT64]  = me::Op::R_INT64;

    (*Literal::typeMap_)[L_SAT8]   = me::Op::R_SAT8;
    (*Literal::typeMap_)[L_SAT16]  = me::Op::R_SAT16;

    (*Literal::typeMap_)[L_UINT8]  = me::Op::R_UINT8;
    (*Literal::typeMap_)[L_UINT16] = me::Op::R_UINT16;
    (*Literal::typeMap_)[L_UINT32] = me::Op::R_UINT32;
    (*Literal::typeMap_)[L_UINT64] = me::Op::R_UINT64;

    (*Literal::typeMap_)[L_USAT8]  = me::Op::R_USAT8;
    (*Literal::typeMap_)[L_USAT16] = me::Op::R_USAT16;

    (*Literal::typeMap_)[L_REAL32] = me::Op::R_REAL32;
    (*Literal::typeMap_)[L_REAL64] = me::Op::R_REAL64;

    (*Literal::typeMap_)[L_INT]    = me::arch->getPreferedInt();
    (*Literal::typeMap_)[L_UINT]   = me::arch->getPreferedUInt();
    (*Literal::typeMap_)[L_INDEX]  = me::arch->getPreferedIndex();
    (*Literal::typeMap_)[L_REAL]   = me::arch->getPreferedReal();
}
