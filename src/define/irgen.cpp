#include "ir.hpp"
#include "../back/gen_tigger.hpp"

namespace EeyoreToTigger {
    std::string DeclIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenDecl(*this, code);
    }

    std::string InitIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenInit(*this, code);
    }

    std::string FuncDefIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenFuncDef(*this, code);
    }

    std::string StatementsIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenStatements(*this, code);
    }

    std::string BinaryExpIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenBinaryExp(*this, code);
    }

    std::string UnaryExpIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenUnaryExp(*this, code);
    }

    std::string AssignIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenAssign(*this, code);
    }

    std::string CondGotoIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenCondGoto(*this, code);
    }

    std::string LValIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenLVal(*this, code);
    }

    std::string GotoIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenGoto(*this, code);
    }

    std::string Label::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenLabel(*this, code);
    }

    std::string ParamListIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenParamList(*this, code);
    }

    std::string FuncCallIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenFuncCall(*this, code);
    }

    std::string ReturnIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenReturn(*this, code);
    }

    std::string RightValIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenRightVal(*this, code);
    }

    std::string ProgramIR::Generate(TiggerGenerator &generator, std::string &code) {
        return generator.GenProgram(*this, code);
    }
}