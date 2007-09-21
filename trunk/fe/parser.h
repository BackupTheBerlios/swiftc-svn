#ifndef SWIFT_PARSER_H
#define SWIFT_PARSER_H

// it is important to declare all union members of YYSTYPE here
struct Type;
struct BaseType;

struct Module;
struct Definition;

struct Class;
struct ClassMember;
struct Parameter;
struct MemberVar;
struct Method;

struct Statement;
struct ExprList;
struct Expr;

//------------------------------------------------------------------------------

extern bool parseerror;
extern bool gencode;

extern "C" int swiftparse();

std::string* operatorToString(int _operator);

// include auto generated parser header for tokens
#include "parser.tab.hpp"

#endif // SWIFT_TOKENS
