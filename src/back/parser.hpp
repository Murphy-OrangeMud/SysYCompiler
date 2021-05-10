#ifndef EEYORE_PARSER
#define EEYORE_PARSER

#include <define/ir.hpp>
#include "../define/irtok.hpp"
#include "lexer.hpp"
#include "../utils/logger.hpp"

namespace EeyoreToTigger {

    class Parser {
    public:
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