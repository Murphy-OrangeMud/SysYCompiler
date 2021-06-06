#ifndef EEYORE_PARSER
#define EEYORE_PARSER

#include <define/ir.hpp>
#include <iostream>
#include "../define/irtok.hpp"
#include "eeyore_lexer.hpp"
#include "../utils/logger.hpp"

namespace EeyoreToTigger {

    class Parser {
    public:
        Parser(std::istream &_cinstream): lexer(_cinstream) {}

        void NextToken();

        IRPtr ParseDecl();

        IRPtr ParseInit();

        IRPtr ParseFuncDef();

        IRPtr ParseStatements();

        IRPtr ParseAssign();

        IRPtr ParseCondGoto();

        IRPtr ParseLVal();

        IRPtr ParseGoto();

        IRPtr ParseLabel();

        IRPtr ParseParams();

        IRPtr ParseFuncCall();

        IRPtr ParseReturn();

        IRPtr ParseProgram();

    private:
        Lexer lexer;
        Logger logger;
        Token current;
    };
}

#endif