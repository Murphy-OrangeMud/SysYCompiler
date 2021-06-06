#ifndef GEN_RISCV_INCLUDED
#define GEN_RISCV_INCLUDED

#include <map>
#include <string>
#include <iostream>

namespace TiggerToRiscV {
    class RiscVGenerator {
        std::map<std::string, int> funcStack;
        std::string currentFunc;
        std::istream &cinstream;
    public:
        void Generate(std::string &code);
        RiscVGenerator(std::istream &_cinstream): cinstream(_cinstream) {}
    };
}

#endif