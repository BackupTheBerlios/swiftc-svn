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
    ADD_DEFINITIONS (-DNDEBUG -O3 -fomit-frame-pointer -pipe)
ELSE (BUILD_RELEASE)
    ADD_DEFINITIONS (-ggdb -Wall -DSWIFT_DEBUG)
ENDIF (BUILD_RELEASE)

# add . as include path
INCLUDE_DIRECTORIES(.)

#-------------------------------------------------------------------------------

# call bison for the front-end
IF (BUILD_RELEASE)
    ADD_CUSTOM_COMMAND (
        OUTPUT  fe/parser.tab.cpp fe/parser.tab.hpp
        COMMAND bison -p swift -d -o fe/parser.tab.cpp fe/parser.y
        DEPENDS fe/parser.y
    )
ELSE (BUILD_RELEASE)
    ADD_CUSTOM_COMMAND (
        OUTPUT  fe/parser.tab.cpp fe/parser.tab.hpp fe/parser.output
        COMMAND bison -p swift -t -d -v -o fe/parser.tab.cpp fe/parser.y
        DEPENDS fe/parser.y
    )
ENDIF (BUILD_RELEASE)

# call flex for the front-end and patch the output file to get rid of the warning
ADD_CUSTOM_COMMAND (
    OUTPUT  fe/lex.yy.cpp
    COMMAND flex -P swift -o fe/lex.yy.cpp fe/lexer.l && sed 's/^[ \\t]*int n\;/\\t\\tsize_t n\;/' <fe/lex.yy.cpp >tmp && mv tmp fe/lex.yy.cpp

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
    utils/memmgr.cpp
    utils/stringhelper.cpp

    fe/lex.yy.cpp
    fe/parser.tab.cpp
    fe/parser.tab.hpp

    fe/class.cpp
    fe/error.cpp
    fe/expr.cpp
    fe/method.cpp
    fe/module.cpp
    fe/scope.cpp
    fe/signature.cpp
    fe/statement.cpp
    fe/symtab.cpp
    fe/syntaxtree.cpp
    fe/type.cpp
    fe/var.cpp

    fe/cmdlineparser.cpp
    fe/main.cpp

    me/arch.cpp
    me/basicblock.cpp
    me/codepass.cpp
    me/cfg.cpp
    me/coloring.cpp
    me/constpool.cpp
    me/copyinsertion.cpp
    me/defusecalc.cpp
    me/functab.cpp
    me/livenessanalysis.cpp
    me/liverangesplitting.cpp
    me/op.cpp
    me/offset.cpp
    me/spiller.cpp
    me/ssa.cpp
    me/stackcoloring.cpp
    me/stacklayout.cpp
    me/struct.cpp

    be/x64.cpp
    be/x64codegen.cpp
    be/x64codegenhelpers.cpp
    be/x64regalloc.cpp
    be/x64parser.tab.cpp
    be/x64parser.tab.hpp
)

#-------------------------------------------------------------------------------

# autogenerate builtin types
ADD_CUSTOM_COMMAND (
    TARGET swift
    POST_BUILD
    COMMAND ./gentypes.sh
    WORKING_DIRECTORY fe/builtin
)
