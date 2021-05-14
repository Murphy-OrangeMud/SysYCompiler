#ifndef LIVESEG_INCLUDED
#define LIVESEG_INCLUDED

#include <map>
#include <utils/utils.hpp>
#include <set>
#include <define/ir.hpp>
#include <utils/logger.hpp>

namespace EeyoreToTigger {
    class LiveSeg {
        Logger logger;

    public:
        std::map<std::string, std::set<Seg>> segments;

        LiveSeg() = default;

        void SegFuncDef(FuncDefIR &funcDef, std::set<std::string> &nextVars);

        void SegStatements(StatementsIR &stmts, std::set<std::string> &nextVars);

        void SegStmtExp(BaseIR &ir, std::set<std::string> &nextVars);

        void SegProgram(ProgramIR &program, std::set<std::string> &nextVars);

    };
}

#endif