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

#include <llvm/Module.h>

#include "utils/memmgr.h"
#include "utils/stringhelper.h"

#include "fe/auto.h"
#include "fe/cmdlineparser.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/type.h"
#include "fe/typenode.h"

//------------------------------------------------------------------------------

// forward declarations

void readBuiltinTypes(swift::Context* ctxt);
int start(int argc, char** argv);

//------------------------------------------------------------------------------

// inits the memory manager
int main(int argc, char** argv)
{
#ifdef SWIFT_USE_MEM_MGR
    // find memory leaks
    MemMgr::init();
#endif // SWIFT_USE_MEM_MGR

    //MemMgr::setBreakpoint(34079);

    int result = start(argc, argv);

#ifdef SWIFT_USE_MEM_MGR
    MemMgr::deinit();
#endif // SWIFT_USE_MEM_MGR

    return result;
}

// inits all compiler globals and controls all compiler passes
int start(int argc, char** argv)
{
    // parse the command line
    swift::CmdLineParser clp(argc, argv);

    if ( !clp.result() )
        return EXIT_FAILURE;

    /*
     * init globals
     */
    //swift::Literal::initTypeMap();

    swift::Module* module = new swift::Module( swift::location(), new std::string("default") );
    swift::BaseType::initTypeMap(module->llvmCtxt_);

    // populate data structures with builtin types
    readBuiltinTypes(module->ctxt_);

    // try to open the input file and init the lexer
    FILE* file = swift::lexer_init( clp.getFilename() );
    if (!file)
    {
        std::cerr << "error: failed to open input file" << std::endl;
        return EXIT_FAILURE;
    }

    swift::Parser parser(module->ctxt_);
#if 0
    parser.set_debug_level(1);
    parser.set_debug_stream(std::cerr);
#endif
    parser.parse();

    fclose(file);

    module->analyze();

    if (module->ctxt_->result_)
        module->buildLLVMTypes();

    if (module->ctxt_->result_)
        module->codeGen();

    if (module->ctxt_->result_)
    {
        if ( clp.dump() )
            module->llvmDump();
    }

    /*
     * clean up
     */

    bool result = module->ctxt_->result_;


    swift::BaseType::destroyTypeMap();
    delete swift::g_lexer_filename;
    delete module;
    //swift::Literal::destroyTypeMap();

    if (!result)
        return EXIT_FAILURE; // abort on error

    return EXIT_SUCCESS;
}

void readBuiltinTypes(swift::Context* ctxt)
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

    builtin.push_back("fe/builtin/index.swift");

    builtin.push_back("fe/builtin/real.swift");
    builtin.push_back("fe/builtin/real32.swift");
    builtin.push_back("fe/builtin/real64.swift");

    builtin.push_back("fe/builtin/bool.swift");

    FILE* file;

    for (size_t i = 0; i < builtin.size(); ++i)
    {
        file = swift::lexer_init(builtin[i]);

        swift::Parser parser(ctxt);
#if 0
        parser.set_debug_level(1);
        parser.set_debug_stream(std::cerr);
#endif
        parser.parse();

        fclose(file);
    }
}
