#ifndef SWIFT_LEXER_H
#define SWIFT_LEXER_H

namespace swift {

FILE* lexerInit(const char* filename);

extern int currentLine;
extern int getKeyLine();

}

int swiftlex();

#endif // SWIFT_LEXER_H
