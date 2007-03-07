#ifndef SWIFT_TOKENS
#define SWIFT_TOKENS

#include <list>

namespace swift
{
    // it is important to declare all union members of YYSTYPE here
    class Type;
    class BaseType;

    class Module;
    class Definition;

    class Class;
    class ClassMember;
    class Parameter;
    class MemberVar;
    class Method;

    class Expr;
    class Arg;

    class Statement;
}

// include auto generated parser header for tokens
#include "parser.tab.hpp"

#endif // SWIFT_TOKENS
