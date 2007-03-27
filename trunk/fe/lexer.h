#ifndef SWIFT_LEXER_H
#define SWIFT_LEXER_H

FILE* lexerInit(const char* filename);

extern int currentLine;
extern int getKeyLine();

extern "C" int yylex(void);
void yyerror(char *);

#endif // SWIFT_LEXER_H
