#ifndef SYSY_LEXER_INCLUDED
#define SYSY_LEXER_INCLUDED

#include <iostream>
#include "../define/token.hpp"
#include "../oj.hpp"

namespace SysYToEeyore {
    class Lexer {
    public:
        Lexer() = default;
        ~Lexer() = default;

        Token NextToken();
        Operator getOp() { return op; }
        long long getVal() { return value; }
        std::string getName() { return name; }
        Type getType() { return type; }

    private:
        Token parseInt();
        Token parseIDKeyword();
        Token parseComment();

        long long value{};
        std::string name;
        Operator op;
        Type type;
    };
}

#endif