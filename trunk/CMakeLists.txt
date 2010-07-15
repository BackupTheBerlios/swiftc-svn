# Swift compiler framework
# Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
# 
# This framework is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 3 as published by the Free Software Foundation.
# 
# This framework is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this framework; see the file LICENSE. If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT (SWIFT)

OPTION (BUILD_RELEASE "Should I build the release (on) or debug version (off)?" off)

IF (BUILD_RELEASE)
    ADD_DEFINITIONS (-DNDEBUG -O3 -fomit-frame-pointer -pipe `llvm-config --cxxflags`)
ELSE (BUILD_RELEASE)
    ADD_DEFINITIONS (-ggdb -Wall -DSWIFT_DEBUG -DDEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -Woverloaded-virtual) 
ENDIF (BUILD_RELEASE)

# add . as include path
INCLUDE_DIRECTORIES(. ../packetizer/build/include)

#-------------------------------------------------------------------------------

# call bison for the front-end
IF (BUILD_RELEASE)
    ADD_CUSTOM_COMMAND (
        OUTPUT  fe/parser.cpp fe/parser.inner.h fe/location.hh fe/position.hh fe/stack.hh
        COMMAND bison -Wall --language=C++ --locations -p swift_stream_ --defines=fe/parser.inner.h -o fe/parser.cpp fe/parser.y
        DEPENDS fe/parser.y
    )
ELSE (BUILD_RELEASE)
    ADD_CUSTOM_COMMAND (
        OUTPUT  fe/parser.cpp fe/parser.inner.h fe/parser.output fe/location.hh fe/position.hh fe/stack.hh
        COMMAND bison -Wall --language=C++ --locations -p swift_stream_ --defines=fe/parser.inner.h -t -v -o fe/parser.cpp fe/parser.y && ./repair_position.sh
        DEPENDS fe/parser.y
    )
ENDIF (BUILD_RELEASE)

# call flex for the front-end and patch the output file to get rid of the warning
ADD_CUSTOM_COMMAND (
    OUTPUT  fe/lexer.cpp
    COMMAND flex -P swift_ -o fe/lexer.cpp fe/lexer.l 
    DEPENDS fe/lexer.l
)


#-------------------------------------------------------------------------------

# call bison for the x64 back-end
IF (BUILD_RELEASE)
    ADD_CUSTOM_COMMAND (
        OUTPUT  be/x64parser.tab.cpp be/x64parser.tab.hpp
        COMMAND bison -p x64 -d -o be/x64parser.tab.cpp be/x64parser.y
        DEPENDS be/x64parser.y
    )
ELSE (BUILD_RELEASE)
    ADD_CUSTOM_COMMAND (
        OUTPUT  be/x64parser.tab.cpp be/x64parser.tab.hpp be/x64parser.output
        COMMAND bison -p x64 -t -d -v -o be/x64parser.tab.cpp be/x64parser.y
        DEPENDS be/x64parser.y
    )
ENDIF (BUILD_RELEASE)

#-------------------------------------------------------------------------------

#SET(CMAKE_VERBOSE_MAKEFILE ON)

# sources
ADD_EXECUTABLE (swift
    utils/assert.cpp
    utils/disjointsets.cpp
    utils/memmgr.cpp
    utils/stringhelper.cpp
    utils/llvmhelper.cpp
    utils/llvmplace.cpp

    fe/lexer.cpp
    fe/parser.cpp

    fe/class.cpp
    fe/classanalyzer.cpp
    fe/classcodegen.cpp
    fe/context.cpp
    fe/error.cpp
    fe/fct.cpp
    fe/fctvectorizer.cpp
    fe/tnlist.cpp
    fe/llvmtypebuilder.cpp
    fe/llvmfctdeclarer.cpp
    fe/node.cpp
    fe/scope.cpp
    fe/sig.cpp
    #fe/simdloopanalyzer.cpp
    fe/stmnt.cpp
    fe/stmntanalyzer.cpp
    fe/stmntcodegen.cpp
    fe/type.cpp
    fe/typelist.cpp
    fe/typenode.cpp
    fe/typenodeanalyzer.cpp
    fe/typenodecodegen.cpp
    fe/var.cpp

    fe/cmdlineparser.cpp
    fe/main.cpp

    vec/typevectorizer.cpp
    )

#-------------------------------------------------------------------------------

# autogenerate builtin types
ADD_CUSTOM_COMMAND (
    TARGET swift
    POST_BUILD
    COMMAND ./gentypes.sh
    WORKING_DIRECTORY fe/builtin
    DEPENDS fe/builtin/int_template.swift fe/builtin/real_template.swift fe/builtin/bool.swift
    )

#-------------------------------------------------------------------------------

EXECUTE_PROCESS(
    COMMAND sh -c "llvm-config --libs all"
    RESULT_VARIABLE res_llvm_libs
    OUTPUT_VARIABLE llvm_libs
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

EXECUTE_PROCESS(
    COMMAND sh -c "llvm-config --ldflags"
    RESULT_VARIABLE res_llvm_flags
    OUTPUT_VARIABLE llvm_flags
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

TARGET_LINK_LIBRARIES(swift -L../packetizer/build/lib -lPacketizer ${llvm_libs} ${llvm_flags})
