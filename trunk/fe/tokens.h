#ifndef SWIFT_TOKENS
#define SWIFT_TOKENS

#include <list>

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

struct Expr;
struct Arg;

struct Statement;

// include auto generated parser header for tokens
#include "parser.tab.hpp"

#endif // SWIFT_TOKENS
