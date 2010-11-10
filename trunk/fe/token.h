#ifndef SWIFT_TOKEN_H
#define SWIFT_TOKEN_H

#include <string>

#include "fe/location.h"

namespace swift {

class Expr;

struct Token
{
    enum Type
    {
        DUMMY = 255,

#define SWIFT_ENUM_VALUE(x) x,
#include "fe/tokenvalues.h"
#undef  SWIFT_ENUM_VALUE

        NUM_TOKENS
    };

    union Value
    {
        std::string* id_;
        Expr* expr_;
    };

    Location loc_;
    int type_;
    Value value_;

    Token() {}

    Token(std::string* filename, int line, char type)
        : loc_(filename, line)
        , type_(type)
    {}

    Token(std::string* filename, int line, Type type)
        : loc_(filename, line)
        , type_(type)
    {}

    Token(std::string* filename, int line, int type, Expr* expr)
        : loc_(filename, line)
        , type_(type)
    {
        value_.expr_ = expr;
    }

    Token(std::string* filename, int line, int type, std::string* id)
        : loc_(filename, line)
        , type_(type) 
    {
        value_.id_ = id;
    }

    template<int type> bool is() { return type_ == type || type_ == Token::END_OF_FILE; }

    static std::string type2str(int type);
    std::string toString() const { return type2str(type_); };
};

typedef Token::Type TokenType;

} // namespace swift

#endif // SWIFT_TOKEN_H
