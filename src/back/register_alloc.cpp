#include "register_alloc.hpp"
#include <algorithm>

namespace EeyoreToTigger {
    void RegisterAlloc::AllocFuncDef(FuncDefIR &ir, std::set<std::string> prevVars) {
        logger.SetFunc("AllocFuncDef");
        ir.getBody()->AllocRegister(*this, prevVars);
        logger.UnSetFunc("AllocFuncDef");
    }

    void RegisterAlloc::AllocProgram(ProgramIR &program, std::set<std::string> prevVars) {
        logger.SetFunc("AllocProgram");
        for (int i = 0; i < program.getNodes().size(); i++) {
            if (i == 0) {
                program.getNodes()[i]->AllocRegister(*this, prevVars);
                logger.UnSetFunc("AllocProgram");

            } else {
                program.getNodes()[i]->AllocRegister(*this, program.getNodes()[i-1]->liveVars);
                logger.UnSetFunc("AllocProgram");
            }
        }
    }

    void RegisterAlloc::AllocStatements(StatementsIR &stmts, std::set<std::string> prevVars) {
        logger.SetFunc("AllocStatements");
        for (int i = 0; i < stmts.getStmts().size(); i++) {
            if (i == 0) {
                stmts.getStmts()[i]->AllocRegister(*this, prevVars);
                logger.UnSetFunc("AllocStatements");

            } else {
                stmts.getStmts()[i]->AllocRegister(*this, stmts.getStmts()[i-1]->liveVars);
                logger.UnSetFunc("AllocStatements");
            }
        }
    }

    void RegisterAlloc::AllocStmtExp(BaseIR &stmt, std::set<std::string> prevVars) {
        logger.SetFunc("AllocStmtExp");
        std::set<std::string> beginVars, endVars;
        std::set_difference(stmt.liveVars.begin(), stmt.liveVars.end(), prevVars.begin(), prevVars.end(), beginVars);
        std::set_difference(prevVars.begin(), prevVars.end(), stmt.liveVars.begin(), stmt.liveVars.end(), endVars);

        if (stmt.lineno > 0) {
            regVars[stmt.lineno].insert(regVars[stmt.lineno - 1].begin(), regVars[stmt.lineno - 1].end());
        }
        regVars[stmt.lineno].insert(beginVars.begin(), beginVars.end());
        for (const auto & endVar : endVars) {
            regVars[stmt.lineno].erase(endVar);
        }
        while (regVars[stmt.lineno].size() > 24) {
            std::string to_erase;
            int most_end = 0;
            for (const auto & var : regVars[stmt.lineno]) {
                for (const auto & seg : segments[var]) {
                    if (seg.begin <= stmt.lineno && seg.end >= stmt.lineno) {
                        if (seg.end > most_end) {
                            most_end = seg.end;
                            to_erase = var;
                        }
                        break;
                    }
                }
            }
            spiltVars[stmt.lineno].insert(to_erase);
            regVars[stmt.lineno].erase(to_erase);
        }
    }

}
