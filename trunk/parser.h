#ifndef SWIFT_PARSER_H
#define SWIFT_PARSER_H

#include <cstdio>

namespace swift
{
    class SyntaxTree;
    class SymbolTable;
}

void parserInit(swift::SyntaxTree* syntaxTree);
int yyparse(void);

#endif // SWIFT_PARSER_H
