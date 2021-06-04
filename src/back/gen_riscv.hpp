#ifndef GEN_RISCV_INCLUDED
#define GEN_RISCV_INCLUDED

#include <map>
#include <string>

namespace TiggerToRiscV {
    class RiscVGenerator {
        std::map<std::string, int> funcStack;
    public:
        void Generate();
        RiscVGenerator() = default;
    };
}

#endif