PROJECT (SWIFT)

OPTION(BUILD_RELEASE "Should I build the release (on) or debug version (off)?" off)

IF (BUILD_RELEASE)
    ADD_DEFINITIONS(-DNDEBUG -O3 -fomit-frame-pointer)
ELSE (BUILD_RELEASE)
    ADD_DEFINITIONS(-ggdb -Wall -DSWIFT_DEBUG)
ENDIF (BUILD_RELEASE)

# add . as include path
ADD_DEFINITIONS(-I.)


# call bison
IF (BUILD_RELEASE)
    ADD_CUSTOM_COMMAND(
        OUTPUT  fe/parser.tab.cpp fe/parser.tab.hpp
        COMMAND bison -d -o fe/parser.tab.cpp fe/parser.y
        DEPENDS fe/parser.y
    )
ELSE (BUILD_RELEASE)
    ADD_CUSTOM_COMMAND(
        OUTPUT  fe/parser.tab.cpp fe/parser.tab.hpp fe/parser.output
        COMMAND bison -t -d -v -o fe/parser.tab.cpp fe/parser.y
        DEPENDS fe/parser.y
    )
ENDIF (BUILD_RELEASE)

# call flex
ADD_CUSTOM_COMMAND(
    OUTPUT  fe/lex.yy.cpp
    COMMAND flex -o fe/lex.yy.cpp fe/lexer.l
    DEPENDS fe/lexer.l
)


ADD_EXECUTABLE(swift
    utils/assert.cpp

    fe/lex.yy.cpp
    fe/parser.tab.cpp
    fe/parser.tab.hpp

    fe/class.cpp
    fe/error.cpp
    fe/expr.cpp
    fe/statement.cpp
    fe/symboltable.cpp
    fe/syntaxtree.cpp
    fe/type.cpp

    im/scopetable.cpp
    im/ssa.cpp

    compiler.cpp
    main.cpp
)
