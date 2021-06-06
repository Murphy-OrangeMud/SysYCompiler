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
    RiscVGenerator generator = RiscVGenerator(std::cin);
    std::string out;
    generator.Generate(out);
    std::cout << out << std::endl;
#else
#ifdef SYSY2RISCV
    if (argc < 5) {
        std::cerr << "Please enter filename\n";
        exit(-1);
    }
    if (!freopen(argv[2], "r", stdin)) { std::cerr << "open input file failed" << std::endl; exit(7); }
    if (!freopen(argv[4], "w", stdout)) { std::cerr << "open output file failed" << std::endl; exit(8); }

    SysYToEeyore::Parser parser = SysYToEeyore::Parser();
    SysYToEeyore::ASTPtr root = parser.ParseCompUnit();
    if (!root) {
        std::cerr << "Parse error, syntax error\n";
        exit(1);
    }
    SysYToEeyore::TypeCheck checker = SysYToEeyore::TypeCheck();
    SysYToEeyore::ASTPtr nRoot = root->Eval(checker);
    if (!nRoot) {
        std::cerr << "Type check error\n";
        exit(2);
    }
    std::map<std::string, Function> FuncTable = checker.FuncTable;
    std::map<int, std::map<std::string, Var>> BlockVars = checker.BlockVars;
    SysYToEeyore::IRGenerator generator = SysYToEeyore::IRGenerator(std::move(FuncTable), std::move(BlockVars));
    std::string out;
    nRoot->GenerateIR(generator, out);
    std::cerr << out << std::endl;

    std::istringstream stream_stmt(out);
    EeyoreToTigger::Parser parser2 = EeyoreToTigger::Parser(stream_stmt);
    EeyoreToTigger::IRPtr root2 = parser2.ParseProgram();
    EeyoreToTigger::TiggerGenerator generator2 = EeyoreToTigger::TiggerGenerator();
    std::string out2;
    root2->Generate(generator2, out2);
    std::cerr << out2 << std::endl;

    std::istringstream stream_stmt2(out2);
    TiggerToRiscV::RiscVGenerator generator3 = TiggerToRiscV::RiscVGenerator(stream_stmt2);
    std::string out3;
    generator3.Generate(out3);
    std::cout << out3 << std::endl;
    std::cerr << out3 << std::endl;
#endif
#endif
#endif
#endif
    return 0;
}