#ifndef SWIFT_COMPILER_H
#define SWIFT_COMPILER_H

#include "fe/parser.h"
#include "fe/syntaxtree.h"
#include "fe/symboltable.h"

// forward declarations
struct Class;
struct Method;
struct MemberVar;

struct Compiler
{
    FILE*       file_;
    SyntaxTree  syntaxTree_;

    bool parse();
    bool analyze();
    bool prepareCodeGen();
    bool genCode();
    bool optimize();
    bool buildAssemblyCode();

    std::string toString();
};

#endif // SWIFT_COMPILER_H
