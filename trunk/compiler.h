#ifndef SWIFT_COMPILER_H
#define SWIFT_COMPILER_H

#include "parser.h"
#include "syntaxtree.h"
#include "symboltable.h"

namespace swift
{

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

} // namespace swift

#endif // SWIFT_COMPILER_H
