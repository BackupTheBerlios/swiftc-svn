/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

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
#include "me/stackcoloring.h"

#include "be/x64.h"

using namespace swift;

//------------------------------------------------------------------------------

// forward declarations

void readBuiltinTypes();
int start(int argc, char** argv);
void cleanUpME();

//------------------------------------------------------------------------------

// inits the memory manager
int main(int argc, char** argv)
{
#ifdef SWIFT_DEBUG
    // find memory leaks
    MemMgr::init();
#endif // SWIFT_DEBUG

    //MemMgr::setBreakpoint(32859);

    int result = start(argc, argv);

#ifdef SWIFT_DEBUG
    MemMgr::deinit();
#endif // SWIFT_DEBUG

    return result;
}

// inits all compiler globals and controls all compiler passes
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
    BaseType::initTypeMap();
    Literal::initTypeMap();

    syntaxtree = new SyntaxTree();
    syntaxtree->rootModule_ = new Module(new std::string("default"), currentLine);

    symtab = new SymTab();
    symtab->insert(syntaxtree->rootModule_);

    error = new ErrorHandler(cmdLineParser.filename_);
    me::functab = new me::FuncTab(cmdLineParser.filename_); // the symbol table of the middle-end

    Container::initMeContainer(); // needs inited functab

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
    Literal::destroyTypeMap();
    BaseType::destroyTypeMap();

    fclose(file);

    if (!analyzeResult)
    {
        cleanUpME();
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

        if ( function->ignore() )
            continue;

        me::DefUseCalc(function).process();
        me::LivenessAnalysis(function).process();
        me::arch->regAlloc(function);
        me::StackColoring(function).process();
    }

    // finally generate assembly code
    for (me::FunctionTable::FunctionMap::iterator iter = me::functab->functions_.begin(); iter != me::functab->functions_.end(); ++iter)
    {
        me::Function* function = iter->second;

        if ( function->ignore() )
            continue;

        me::arch->codeGen(function, ofs);
    }

    // write constants to assembly language file
    me::arch->dumpConstants(ofs);
    
    // clean up
    me::arch->cleanUp();

#ifdef SWIFT_DEBUG

    /*
     * debug output
     */
    me::functab->dumpSSA();
    me::functab->dumpDot();

#endif // SWIFT_DEBUG

    // finish
    ofs.close();

    cleanUpME();

    return EXIT_SUCCESS;
}

void cleanUpME()
{
    /*
     * clean up middle-end
     */
    delete me::functab;
    delete me::constpool;

    /*
     * clean up back-end
     */
    delete me::arch;
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
