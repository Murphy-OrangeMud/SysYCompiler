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

class Parser {
private:
    Lexer lexer;
    Logger logger;
    Token current;
public:
    Parser() {
        srand(std::time(0));
        lexer = Lexer();
        std::string path = "..\\..\\logs\\log_" + std::to_string(rand() % 10000);
        std::cout << path << std::endl;
        logger = Logger(path);
        logger.open();
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