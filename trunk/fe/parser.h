#ifndef SWIFT_PARSER_H
#define SWIFT_PARSER_H

#include <cstdio>

struct SyntaxTree;
struct SymbolTable;

void parserInit(SyntaxTree* syntaxTree);
int yyparse(void);

#endif // SWIFT_PARSER_H
