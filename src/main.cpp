#include <iostream>
#include <algorithm>
#include "mid/typeck.hpp"
#include "front/parser.hpp"
#include "mid/genIR.hpp"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    freopen(argv[1], "r", stdin);
    Parser parser = Parser();
    ASTPtr root = parser.ParseCompUnit();
    TypeCheck checker = TypeCheck();
    ASTPtr nRoot = checker.EvalCompUnit(*dynamic_cast<CompUnitAST*>(root.get()));
    std::map<std::string, std::vector<int>> arrayTable = checker.ArrayTable;
    arrayTable.insert(checker.ConstArrayTable.begin(), checker.ConstArrayTable.end());
    std::map <std::string, std::map<std::string, std::vector < int>>> funcArrayTable = checker.FuncArrayTable;
    funcArrayTable.insert(checker.FuncConstArrayTable.begin(), checker.FuncConstArrayTable.end());
    IRGenerator generator = IRGenerator(arrayTable, funcArrayTable);
    std::string out;
    generator.GenCompUnit(*dynamic_cast<CompUnitAST*>(nRoot.get()), out);
    std::cout << out;
    return 0;
}