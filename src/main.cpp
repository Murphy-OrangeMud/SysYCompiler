#include <iostream>
#include <algorithm>
#include <back/gen_riscv.hpp>
#include "mid/typeck.hpp"
#include "front/parser.hpp"
#include "mid/genIR.hpp"
#include "back/eeyore_parser.hpp"
#include "back/gen_tigger.hpp"

int main(int argc, char *argv[]) {
#ifndef OJ
    if (argc < 0) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    freopen("/mnt/c/Users/cheng/SysYCompiler/open-test-cases/sysy/section1/functional_test/00_arr_defn2.sy", "r", stdin);
    if (!freopen("00.eeyore", "w", stdout))
        std::cerr << "Open output file failed\n";
#else
    if (argc < 6) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    if (!freopen(argv[3], "r", stdin)) { std::cerr << "open input file failed" << std::endl; exit(7); }
    if (!freopen(argv[5], "w", stdout)) { std::cerr << "open output file failed" << std::endl; exit(8); }
#endif

#ifdef SYSY2EEYORE
    using namespace SysYToEeyore;
    Parser parser = Parser();
    ASTPtr root = parser.ParseCompUnit();
    if (!root) {
        std::cerr << "Parse error, syntax error\n";
        exit(1);
    }
    TypeCheck checker = TypeCheck();
    ASTPtr nRoot = root->Eval(checker);
    if (!nRoot) {
        std::cerr << "Type check error\n";
        exit(2);
    }
    std::map<std::string, Function> FuncTable = checker.FuncTable;
    std::map<int, std::map<std::string, Var>> BlockVars = checker.BlockVars;
    IRGenerator generator = IRGenerator(std::move(FuncTable), std::move(BlockVars));
    std::string out;
    nRoot->GenerateIR(generator, out);
    std::cout << out << std::endl;
#else
#ifdef EEYORE2TIGGER
    using namespace EeyoreToTigger;
    Parser parser = Parser();
    IRPtr root = parser.ParseProgram();
    TiggerGenerator generator = TiggerGenerator();
    std::string out;
    root->Generate(generator, out);
    std::cout << out << std::endl;
#else
#ifdef TIGGER2RISCV
    using namespace TiggerToRiscV;
    RiscVGenerator generator = RiscVGenerator();
    generator.Generate();
#else
#endif
#endif
#endif
    return 0;
}