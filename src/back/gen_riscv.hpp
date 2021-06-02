#ifndef GEN_RISCV_INCLUDED
#define GEN_RISCV_INCLUDED

#include <map>
#include <string>

namespace Tigger2RiscV {
    class RiscVGenerator {
        std::map<std::string, int> funcStack;
    public:
        void Generate();
    };
}

#endif