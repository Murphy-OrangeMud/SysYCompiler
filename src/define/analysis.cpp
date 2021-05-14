#include "ir.hpp"

namespace EeyoreToTigger {
    void DeclIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeDecl(*this, nextVars);
    }

    void InitIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeInit(*this, nextVars);
    }

    void FuncDefIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeFuncDef(*this, nextVars);
    }

    void StatementsIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeStatements(*this, nextVars);
    }

    void BinaryExpIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeBinary(*this, nextVars);
    }

    void UnaryExpIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeUnary(*this, nextVars);
    }

    void AssignIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeAssign(*this, nextVars);
    }

    void CondGotoIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeCondGoto(*this, nextVars);
    }

    void LValIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeLVal(*this, nextVars);
    }

    void GotoIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeGoto(*this, nextVars);
    }

    void Label::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeLabel(*this, nextVars);
    }

    void ParamListIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeParamList(*this, nextVars);
    }

    void FuncCallIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeFuncCall(*this, nextVars);
    }

    void ReturnIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeReturn(*this, nextVars);
    }

    void RightValIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeRightVal(*this, nextVars);
    }

    void ProgramIR::Analyze(LiveAnalysis &analyzer, std::set<std::string> &nextVars) {
        analyzer.AnalyzeProgram(*this, nextVars);
    }
}
