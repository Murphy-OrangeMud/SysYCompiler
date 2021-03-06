#ifndef PARSER_INCLUDED
#define PARSER_INCLUDED

#include <iostream>
#include <functional>
#include <initializer_list>
#include <map>
#include "../define/ast.hpp"
#include "lexer.hpp"
#include "utils/logger.hpp"
#include <cstdlib>
#include <ctime>
#include "../oj.hpp"

namespace SysYToEeyore {
    class Parser {
    private:
        Lexer lexer;
        Logger logger;
        Token current;
    public:
        Parser(const std::string& i="") {
            lexer = Lexer();
#ifndef OJ
            logger = Logger(path);
        std::string path = R"(./logs/log_parser_)" + i;
#else
            logger = Logger();
#endif
        }

        void NextToken();

        ASTPtr ParseBinary(const std::function<ASTPtr()> &parser, std::initializer_list <Operator> ops);

        ASTPtr ParseRelExp();

        ASTPtr ParseEqExp();

        ASTPtr ParseAddExp();

        ASTPtr ParseMulExp();

        ASTPtr ParseLAndExp();

        ASTPtr ParseLOrExp();

        ASTPtr ParseUnaryExp();

        ASTPtr ParseIfElseStmt();

        ASTPtr ParseWhileStmt();

        ASTPtr ParseStmt();

        ASTPtr ParseBlock();

        ASTPtr ParseInitVal();

        ASTPtr ParseFuncDef();

        ASTPtr ParseVarDecl();

        ASTPtr ParseVarDef(bool isConst);

        ASTPtr ParseCompUnit();
    };
}
#endif