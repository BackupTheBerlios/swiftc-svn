#ifndef SWIFT_PARSER_H
#define SWIFT_PARSER_H

extern "C" int swiftparse();

namespace swift {

// it is important to declare all union members of YYSTYPE here
struct Type;
struct BaseType;

struct Module;
struct Definition;

struct Class;
struct ClassMember;
struct Param;
struct MemberVar;
struct Method;

struct Statement;
struct ExprList;
struct Expr;

//------------------------------------------------------------------------------

extern bool parseerror;

std::string* operatorToString(int _operator);

} // namespace swift

// include auto generated parser header before tokens
#include "parser.tab.hpp"

#endif // SWIFT_PARSER_H
