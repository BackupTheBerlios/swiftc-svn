#include "fe/token2str.h"

#include "utils/assert.h"

namespace swift {

std::string* token2str(int token)
{
    switch (token)
    {
        case Token::ADD: return new std::string("+");
        case Token::SUB: return new std::string("-");
        case Token::MUL: return new std::string("*");
        case Token::DIV: return new std::string("/");

        case Token::AND: return new std::string("&");
        case Token:: OR: return new std::string("|");
        case Token::XOR: return new std::string("^");
        case Token::NOT: return new std::string("!");

        case Token::L_AND: return new std::string("and");
        case Token::L_OR:  return new std::string(" or");
        case Token::L_XOR: return new std::string("xor");
        case Token::L_NOT: return new std::string("not");

        case Token:: EQ: return new std::string("==");
        case Token:: NE: return new std::string("<>");
        case Token:: LT: return new std::string("<"); 
        case Token:: LE: return new std::string("<=");
        case Token:: GT: return new std::string(">");
        case Token:: GE: return new std::string(">=");

        case Token::ASGN: return new std::string("=");

        default: swiftAssert(false, "unhandled case");
    }

    return 0;
}

} // namespace swift
