#include <iostream>
#include "mid/typeck.hpp"
#include "front/lexer.hpp"
#include "front/parser.hpp"
#include "define/ast.hpp"
#include "mid/genIR.hpp"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    freopen(argv[1], "r", stdin);
    Parser parser = Parser();
    ASTPtr root = parser.ParseCompUnit();
    TypeCheck checker = TypeCheck();
    ASTPtr nRoot = checker.EvalCompUnit();
    std::map<std::string, std::vector<int>> arrayTable;
    IRGenerator generator = IRGenerator();
}