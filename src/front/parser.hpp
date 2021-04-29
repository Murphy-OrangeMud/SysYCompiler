#ifndef PARSER_INCLUDED
#define PARSER_INCLUDED

#include <iostream>
#include <functional>
#include <initializer_list>
#include <map>
#include "../define/ast.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <ctime>
#include "../oj.hpp"

class Parser {
private:
    Lexer lexer;
    Logger logger;
    Token current;
public:
    Parser(const std::string& i) {
        lexer = Lexer();
        std::string path = R"(./logs/log_parser_)" + i;
#ifndef OJ
        logger = Logger(path);
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

#endif