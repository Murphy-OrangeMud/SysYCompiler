#include <iostream>
#include <algorithm>
#include "mid/typeck.hpp"
#include "front/parser.hpp"
#include "mid/genIR.hpp"

// TODO:释放掉智能指针
// TODO: Add command parser
int main(int argc, char *argv[]) {
#ifndef OJ
    if (argc < 3) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    freopen(argv[1], "r", stdin);
    if (!freopen(argv[2], "w", stdout))
        std::cerr << "Open output file failed\n";
#else
    // Parse command
#endif
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
    std::cout << out << std::endl;
    return 0;
}