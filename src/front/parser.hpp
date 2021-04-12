#ifndef PARSER_INCLUDED
#define PARSER_INCLUDED

#include <iostream>
#include <functional>
#include <initializer_list>
#include <map>
#include "../define/ast.hpp"
#include "lexer.hpp"
using namespace std;

class Parser {
private:
    Lexer lexer;
    Token current;
    std::vector<std::pair<std::string, long long>> constVals;
    std::vector<std::string> definedVals;
public:
    void NextToken();
    ASTPtr ParseBinary(std::function<ASTPtr()> parser, std::initializer_list<Operator> ops);
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
}

#endif