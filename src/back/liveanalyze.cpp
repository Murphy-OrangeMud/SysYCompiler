#include "liveanalyze.hpp"
#include <iostream>
#include <algorithm>
#include <define/irtok.hpp>
#include <define/ir.hpp>

void LiveAnalysis::AnalyzeDecl(DeclIR &decl, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeDecl");
}

void LiveAnalysis::AnalyzeFuncDef(FuncDefIR &funcDef, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeFuncDef");
    funcDef.getBody()->Analyze(*this, nextVars);
    logger.UnSetFunc("AnalyzeFuncDef");
}

void LiveAnalysis::AnalyzeStatements(StatementsIR &stmts, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeStatements");
    std::map<int, std::pair<int, int>> label_goto;
    for (int i = stmts.getStmts().size() - 1; i >= 0; i--) {
        if (dynamic_cast<Label*>(stmts.getStmts()[i].get())) {
            label_goto[dynamic_cast<Label*>(stmts.getStmts()[i].get())->getNum()].first = i + 1; //attention here
        }
        if (dynamic_cast<GotoIR*>(stmts.getStmts()[i].get())) {
            label_goto[dynamic_cast<GotoIR*>(stmts.getStmts()[i].get())->getLabel()].second = std::max(i + 1, label_goto[dynamic_cast<GotoIR*>(stmts.getStmts()[i].get())->getLabel()].second);
        }
        if (dynamic_cast<CondGotoIR*>(stmts.getStmts()[i].get())) {
            label_goto[dynamic_cast<CondGotoIR*>(stmts.getStmts()[i].get())->getLabel()].second = std::max(i + 1, label_goto[dynamic_cast<CondGotoIR*>(stmts.getStmts()[i].get())->getLabel()].second);
        }
    }
    for (int i = stmts.getStmts().size() - 1; i >= 0; i--) {
        // TODO: 可能有多个goto指向同一个label，目前选取了最大的一个goto。
        if (dynamic_cast<Label*>(stmts.getStmts()[i].get())) {
            bool flag = false;
            while (!flag) {
                flag = true;
                std::pair<int,int> range = label_goto[dynamic_cast<Label*>(stmts.getStmts()[i].get())->getNum()];
                for (int j = range.second; j >= range.first; j--) {
                    std::set<std::string> tmp = stmts.getStmts()[j]->liveVars;
                    stmts.getStmts()[j]->Analyze(*this, stmts.getStmts()[j+1]->liveVars);
                    logger.UnSetFunc("AnalyzeStatements");
                    std::set<std::string> diff;
                    std::set_difference(tmp.begin(), tmp.end(), stmts.getStmts()[j]->liveVars.begin(), stmts.getStmts()[j]->liveVars.end(), diff.begin());
                    if (diff.size() > 0) {
                        flag = false;
                    }
                }
            }
        }
        if (i < stmts.getStmts().size() - 1) {
            stmts.getStmts()[i]->Analyze(*this, stmts.getStmts()[i+1]->liveVars);
            logger.UnSetFunc("AnalyzeStatements");
        } else {
            stmts.getStmts()[i]->Analyze(*this, std::set<std::string>{});
            logger.UnSetFunc("AnalyzeStatements");
        }
    }
}

void LiveAnalysis::AnalyzeCondGoto(CondGotoIR &condGoto, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeCondGoto");
    condGoto.getCond()->Analyze(*this, nextVars);
    logger.UnSetFunc("AnalyzeCondGoto");
    condGoto.liveVars.insert(condGoto.getCond()->liveVars.begin(), condGoto.getCond()->liveVars.end());
    condGoto.liveVars.insert(nextVars.begin(), nextVars.end());
}

void LiveAnalysis::AnalyzeInit(InitIR &init, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeInit");
}

void LiveAnalysis::AnalyzeBinary(BinaryExpIR &binary, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeBinary");
    binary.getRHS()->Analyze(*this, nextVars);
    logger.UnSetFunc("AnalyzeBinary");
    binary.getLHS()->Analyze(*this, nextVars);
    logger.UnSetFunc("AnalyzeBinary");
    binary.liveVars.insert(binary.getLHS()->liveVars.begin(), binary.getLHS()->liveVars.end());
    binary.liveVars.insert(binary.getRHS()->liveVars.begin(), binary.getRHS()->liveVars.end());
}

void LiveAnalysis::AnalyzeUnary(UnaryExpIR &unary, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeUnary");
    unary.getExp()->Analyze(*this, nextVars);
    logger.UnSetFunc("AnalyzeUnary");
    unary.liveVars.insert(unary.getExp()->liveVars.begin(), unary.getExp()->liveVars.end());
}

void LiveAnalysis::AnalyzeAssign(AssignIR &assign, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeAssign");
    assign.getRHS()->Analyze(*this, nextVars);
    logger.UnSetFunc("AnalyzeAssign");
    assign.getLHS()->Analyze(*this, nextVars);
    logger.UnSetFunc("AnalyzeAssign");
    assign.liveVars.insert(nextVars.begin(), nextVars.end());
    assign.liveVars.insert(assign.getRHS()->liveVars.begin(), assign.getLHS()->liveVars.end());
    std::set<std::string> tmp;
    std::set_difference(assign.liveVars.begin(), assign.liveVars.end(), assign.getLHS()->liveVars.begin(), assign.getLHS()->liveVars.end(), tmp.begin());
    assign.liveVars = tmp;
}

void LiveAnalysis::AnalyzeLVal(LValIR &lval, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeLVal");
    if (lval.getName()[0] != 'p') {
        // 参数不需要计算活跃区间，以a开头的8个寄存器不使用
        lval.liveVars.insert(lval.getName());
    }
    lval.getPos()->Analyze(*this, std::set<std::string>{});
    lval.liveVars.insert(lval.getPos()->liveVars.begin(), lval.getPos()->liveVars.end());

}

void LiveAnalysis::AnalyzeRightVal(RightValIR &rval, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeRightVal");
    if (rval.getType() == EeyoreToTigger::Token::SYMBOL && rval.getName()[0] != 'p') {
        // 参数不需要计算活跃区间，以a开头的8个寄存器不使用
        rval.liveVars.insert(rval.getName());
    }
}

void LiveAnalysis::AnalyzeFuncCall(FuncCallIR &funcCall, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeFuncCall");
    funcCall.liveVars.insert(nextVars.begin(), nextVars.end());
}

void LiveAnalysis::AnalyzeReturn(ReturnIR &ret, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeReturn");
    if (ret.getReturnValue()) {
        ret.getReturnValue()->Analyze(*this, nextVars);
        logger.UnSetFunc("AnalyzeReturn");
        ret.liveVars.insert(ret.getReturnValue()->liveVars.begin(), ret.getReturnValue()->liveVars.end());
    }
}

void LiveAnalysis::AnalyzeParamList(ParamListIR &params, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeParamList");
    params.liveVars.insert(nextVars.begin(), nextVars.end());
    for (const auto &param: params.getParams()) {
        param->Analyze(*this, nextVars);
        logger.UnSetFunc("AnalyzeParamList");
        params.liveVars.insert(param->liveVars.begin(), param->liveVars.end());
    }
}

void LiveAnalysis::AnalyzeProgram(ProgramIR &program, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeProgram");
    for (int i = program.getNodes().size() - 1; i >= 0; i--) {
        program.getNodes()[i]->Analyze(*this, std::set<std::string>{});
        logger.UnSetFunc("AnalyzeProgram");
    }
}

void LiveAnalysis::AnalyzeGoto(GotoIR &gt, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeGoto");
    gt.liveVars.insert(nextVars.begin(), nextVars.end());
}

void LiveAnalysis::AnalyzeLabel(Label &label, std::set<std::string> &nextVars) {
    logger.SetFunc("AnalyzeLabel");
    label.liveVars.insert(nextVars.begin(), nextVars.end());
}
