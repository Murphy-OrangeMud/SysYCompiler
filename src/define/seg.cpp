#include "ir.hpp"
#include <back/liveseg.hpp>

void DeclIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {}

void InitIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {}

void FuncDefIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {
    return segger.SegFuncDef(*this, nextVars);
}

void StatementsIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {
    return segger.SegStatements(*this, nextVars);
}

void BinaryExpIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {}

void UnaryExpIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {}

void AssignIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {
    return segger.SegStmtExp(*this, nextVars);
}

void CondGotoIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {
    return segger.SegStmtExp(*this, nextVars);
}

void LValIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {}

void GotoIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {}

void Label::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {}

void ParamListIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {
    return segger.SegStmtExp(*this, nextVars);
}

void FuncCallIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {}

void ReturnIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {
    return segger.SegStmtExp(*this, nextVars);
}

void ProgramIR::Seg(LiveSeg &segger, std::set<std::string> &nextVars) {
    return segger.SegProgram(*this, nextVars);
}
