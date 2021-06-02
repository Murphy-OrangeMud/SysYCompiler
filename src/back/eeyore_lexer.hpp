#ifndef EEYORE_LEXER_INCLUDED
#define EEYORE_LEXER_INCLUDED

#include <string>
#include "define/irtok.hpp"

namespace EeyoreToTigger {
    class Lexer {
    public:
        Lexer() = default;
        ~Lexer() = default;

        Token NextToken();
        Operator getOp() { return op; }
        std::string getName() { return name; }
        int getVal() const { return value; }
        int getLineno() const { return lineno; }

    private:
        int value;
        std::string name;
        Operator op;
        int lineno;

        Token ParseNum();
        Token ParseSymbol();
        Token ParseComment();
    };
}

#endif