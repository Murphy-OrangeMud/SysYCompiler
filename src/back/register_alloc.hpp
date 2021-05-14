#ifndef REGISTER_ALLOC_INCLUDED
#define REGISTER_ALLOC_INCLUDED

#include <map>
#include <set>
#include <string>
#include <utils/logger.hpp>
#include <utils/utils.hpp>
#include <define/ir.hpp>

namespace EeyoreToTigger {
    class RegisterAlloc {
        Logger logger;
        std::map<std::string, std::set<Seg>> segments;
        std::map<int /*lineno*/, std::set<std::string>> regVars;
        std::map<int, std::set<std::string>> spiltVars;

    public:
        void AllocFuncDef(FuncDefIR &ir, std::set<std::string> nextVars);

        void AllocStmtExp(BaseIR &ir, std::set<std::string> nextVars);

        void AllocStatements(StatementsIR &stmts, std::set<std::string> nextVars);

        void AllocProgram(ProgramIR &program, std::set<std::string> nextVars);
    };
}

#endif