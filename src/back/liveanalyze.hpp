#ifndef ANALYSIS_INCLUDED
#define ANALYSIS_INCLUDED

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <define/ir.hpp>
#include <utils/logger.hpp>
#include <utils/utils.hpp>

namespace EeyoreToTigger {

    // TODO: 在这之前加一层优化，把没用的赋值语句（两端都是局部变量）去掉，减少语句和变量数目。
    class LiveAnalysis {
        Logger logger;

    public:
        LiveAnalysis() {}

        void AnalyzeDecl(DeclIR &decl, std::set<std::string> &nextVars);

        void AnalyzeFuncDef(FuncDefIR &funcDef, std::set<std::string> &nextVars);

        void AnalyzeStatements(StatementsIR &stmts, std::set<std::string> &nextVars);

        void AnalyzeCondGoto(CondGotoIR &condGoto, std::set<std::string> &nextVars);

        void AnalyzeInit(InitIR &init, std::set<std::string> &nextVars);

        void AnalyzeBinary(BinaryExpIR &binary, std::set<std::string> &nextVars);

        void AnalyzeUnary(UnaryExpIR &unary, std::set<std::string> &nextVars);

        void AnalyzeAssign(AssignIR &assign, std::set<std::string> &nextVars);

        void AnalyzeLVal(LValIR &lval, std::set<std::string> &nextVars);

        void AnalyzeRightVal(RightValIR &rval, std::set<std::string> &nextVars);

        void AnalyzeFuncCall(FuncCallIR &funcCall, std::set<std::string> &nextVars);

        void AnalyzeReturn(ReturnIR &ret, std::set<std::string> &nextVars);

        void AnalyzeParamList(ParamListIR &params, std::set<std::string> &nextVars);

        void AnalyzeProgram(ProgramIR &program, std::set<std::string> &nextVars);

        void AnalyzeGoto(GotoIR &gt, std::set<std::string> &nextVars);

        void AnalyzeLabel(Label &label, std::set<std::string> &nextVars);
    };

}
#endif
