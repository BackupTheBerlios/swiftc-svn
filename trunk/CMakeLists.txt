PROJECT (SWIFT)

OPTION(BUILD_RELEASE "Should I build the release (on) or debug version (off)?" off)

IF (BUILD_RELEASE)
    ADD_DEFINITIONS(-DNDEBUG -O3 -fomit-frame-pointer)
ELSE (BUILD_RELEASE)
    ADD_DEFINITIONS(-ggdb -Wall -DSWIFT_DEBUG)
ENDIF (BUILD_RELEASE)

# call bison
ADD_CUSTOM_COMMAND(
    OUTPUT  parser.tab.cpp parser.tab.hpp
    COMMAND bison -t -d -v -o parser.tab.cpp parser.y
    DEPENDS parser.y
)

#call flex
ADD_CUSTOM_COMMAND(
    OUTPUT  lex.yy.cpp
    COMMAND flex -o lex.yy.cpp lexer.l
    DEPENDS lexer.l
)


ADD_EXECUTABLE(swift
    parser.tab.cpp
    parser.tab.hpp
    lex.yy.cpp

    assert.cpp
    compiler.cpp
    error.cpp
    symboltable.cpp
    syntaxtree.cpp
    class.cpp
    expr.cpp
    ssa.cpp
    statement.cpp
    type.cpp

    main.cpp
)
