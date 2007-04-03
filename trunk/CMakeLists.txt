PROJECT (SWIFT)

OPTION (BUILD_RELEASE "Should I build the release (on) or debug version (off)?" off)

IF (BUILD_RELEASE)
    ADD_DEFINITIONS (-DNDEBUG -O3 -fomit-frame-pointer)
ELSE (BUILD_RELEASE)
    ADD_DEFINITIONS (-ggdb -Wall -DSWIFT_DEBUG)
ENDIF (BUILD_RELEASE)

# add . as include path
ADD_DEFINITIONS (-I.)


# call bison
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

# call flex
ADD_CUSTOM_COMMAND (
    OUTPUT  fe/lex.yy.cpp
    COMMAND flex -P swift -o fe/lex.yy.cpp fe/lexer.l
    DEPENDS fe/lexer.l
)


ADD_EXECUTABLE (swift
    utils/assert.cpp
    utils/memmgr.cpp

    fe/lex.yy.cpp
    fe/parser.tab.cpp
    fe/parser.tab.hpp

    fe/class.cpp
    fe/error.cpp
    fe/expr.cpp
    fe/statement.cpp
    fe/symtab.cpp
    fe/syntaxtree.cpp
    fe/type.cpp

    fe/cmdlineparser.cpp
    fe/main.cpp

    me/pseudoreg.cpp
    me/scopetab.cpp
    me/ssa.cpp

)
