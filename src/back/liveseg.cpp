#include "liveseg.hpp"
#include <utils/logger.hpp>
#include <algorithm>


void LiveSeg::SegFuncDef(FuncDefIR &funcDef, std::set<std::string> &nextVars) {
    logger.SetFunc("SegFuncDef");
    funcDef.getBody()->Seg(*this, nextVars);
    logger.UnSetFunc("SegFuncDef");
}

void LiveSeg::SegStatements(StatementsIR &stmts, std::set<std::string> &nextVars) {
    logger.SetFunc("SegStatements");
    for (int i = stmts.getStmts().size() - 1; i >= 0; i--) {
        if (i == stmts.getStmts().size() - 1) {
            stmts.getStmts()[i]->Seg(*this, std::set<std::string>{});
            logger.UnSetFunc("SegStatements");
        } else if (i == 0 || dynamic_cast<DeclIR*>(stmts.getStmts()[i-1].get())) {
            for (const auto & beginVar : stmts.getStmts()[i]->liveVars) {
                auto iter = segments[beginVar].begin();
                segments[beginVar].insert(Seg(stmts.getStmts()[i]->lineno, iter->end));
                segments[beginVar].erase(iter);
            }
        } else {
            stmts.getStmts()[i]->Seg(*this, stmts.getStmts()[i+1]->liveVars);
            logger.UnSetFunc("SegStatements");
        }
    }
}

void LiveSeg::SegStmtExp(BaseIR &stmt, std::set<std::string> &nextVars) {
    logger.SetFunc("SegStmtExp");
    // 注意这里的begin是从下一行开始
    std::set<std::string> beginVars, endVars;
    std::set_difference(stmt.liveVars.begin(), stmt.liveVars.end(), nextVars.begin(), nextVars.end(), endVars);
    std::set_difference(nextVars.begin(), nextVars.end(), stmt.liveVars.begin(), stmt.liveVars.end(), beginVars);

    // 严格从后往前扫描，因此不会出现嵌套的情况
    for (const auto & endVar : endVars) {
        segments[endVar].insert(Seg(-1, stmt.lineno));
    }
    for (const auto & beginVar : beginVars) {
        // iter->begin == -1
        auto iter = segments[beginVar].begin();
        segments[beginVar].insert(Seg(stmt.lineno + 1, iter->end));
        segments[beginVar].erase(iter);
    }
}

void LiveSeg::SegProgram(ProgramIR &program, std::set<std::string> &nextVars) {
    logger.SetFunc("SegProgram");
    for (int i = program.getNodes().size() - 1; i >= 0; i--) {
        if (i == program.getNodes().size() - 1) {
            program.getNodes()[i]->Seg(*this, std::set<std::string>{});
            logger.UnSetFunc("SegProgram");
        } else {
            program.getNodes()[i]->Seg(*this, program.getNodes()[i+1]->liveVars);
            logger.UnSetFunc("SegProgram");
        }
    }
}
