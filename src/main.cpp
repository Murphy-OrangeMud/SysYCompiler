#include <iostream>
#include <algorithm>
#include <back/gen_riscv.hpp>
#include "mid/typeck.hpp"
#include "front/parser.hpp"
#include "mid/genIR.hpp"
#include "back/eeyore_parser.hpp"
#include "back/gen_tigger.hpp"

int main(int argc, char *argv[]) {
#ifdef SYSY2EEYORE
    if (argc < 6) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    if (!freopen(argv[3], "r", stdin)) { std::cerr << "open input file failed" << std::endl; exit(7); }
    if (!freopen(argv[5], "w", stdout)) { std::cerr << "open output file failed" << std::endl; exit(8); }
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
    if (argc < 6) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    if (!freopen(argv[3], "r", stdin)) { std::cerr << "open input file failed" << std::endl; exit(7); }
    if (!freopen(argv[5], "w", stdout)) { std::cerr << "open output file failed" << std::endl; exit(8); }
    using namespace EeyoreToTigger;
    Parser parser = Parser();
    IRPtr root = parser.ParseProgram();
    TiggerGenerator generator = TiggerGenerator();
    std::string out;
    root->Generate(generator, out);
    std::cout << out << std::endl;
#else
#ifdef TIGGER2RISCV
    if (argc < 5) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    if (!freopen(argv[2], "r", stdin)) { std::cerr << "open input file failed" << std::endl; exit(7); }
    if (!freopen(argv[4], "w", stdout)) { std::cerr << "open output file failed" << std::endl; exit(8); }
    using namespace TiggerToRiscV;
    RiscVGenerator generator = RiscVGenerator();
    std::string out;
    generator.Generate(out);
    std::cout << out << std::endl;
#else
#ifdef SYSY2RISCV
    if (argc < 5) {
        std::cerr << "Please enter filename\n";
        exit(1);
    }
    if (!freopen(argv[2], "r", stdin)) { std::cerr << "open input file failed" << std::endl; exit(7); }
    if (!freopen(argv[4], "w", stdout)) { std::cerr << "open output file failed" << std::endl; exit(8); }

#endif
#endif
#endif
#endif
    return 0;
}