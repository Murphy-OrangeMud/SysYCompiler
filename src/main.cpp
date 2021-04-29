#include <iostream>
#include <algorithm>
#include "mid/typeck.hpp"
#include "front/parser.hpp"
#include "mid/genIR.hpp"

// TODO:释放掉智能指针
int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    freopen(argv[1], "r", stdin);
    std::string path = argv[1];
    int idx = path.rfind('/');
    path = path.substr(idx+1, path.length() - idx-4);
    Parser parser = Parser(path);
    ASTPtr root = parser.ParseCompUnit();
    if (!root) {
        std::cerr << "Parse error, syntax error\n";
        exit(1);
    }
    TypeCheck checker = TypeCheck(path);
    ASTPtr nRoot = checker.EvalCompUnit(*dynamic_cast<CompUnitAST*>(root.get()));
    if (!nRoot) {
        std::cerr << "Type check error\n";
        exit(1);
    }
    std::map<std::string, Function> FuncTable = checker.FuncTable;
    std::map<int, std::map<std::string, Var>> BlockVars = checker.BlockVars;
    IRGenerator generator = IRGenerator(argv[1], std::move(FuncTable), std::move(BlockVars));
    std::string out;
    generator.GenCompUnit(*dynamic_cast<CompUnitAST*>(nRoot.get()), out);
    std::cout << out;
    return 0;
}