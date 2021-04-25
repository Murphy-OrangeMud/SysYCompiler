#include <algorithm>
#include "ast.hpp"
#include "../mid/typeck.hpp"
#include "../mid/genIR.hpp"

std::string FuncDefAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenFuncDef(*this, code);
    return {};
}

std::string BlockAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenBlock(*this, code);
    return {};
}

std::string BinaryExpAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return gen.GenBinaryExp(*this, code);
}

std::string IfElseAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenIfElse(*this, code);
    return {};
}

std::string WhileAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenWhile(*this, code);
    return {};
}

std::string NumberAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return gen.GenNumber(*this, code);
}

std::string IdAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return this->getName();
}

std::string ProcessedIdAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return gen.GenId(*this, code);
}

std::string UnaryExpAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return gen.GenUnaryExp(*this, code);
}

std::string ControlAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenControl(*this, code);
    return {};
}

std::string AssignAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return gen.GenAssign(*this, code);
}

std::string StmtAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenStmt(*this, code);
    return {};
}

std::string LValAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return gen.GenLVal(*this, code);
}

std::string FuncCallAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenFuncCall(*this, code);
    return {};
}

std::string VarDeclAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenVarDecl(*this, code);
    return {};
}

std::string VarDefAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return gen.GenVarDef(*this, code);
}

std::string InitValAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return {};
}

std::string ProcessedInitValAST::GenerateIR(IRGenerator &gen, std::string &code) {
    return gen.GenInitVal(*this, code);
}

std::string CompUnitAST::GenerateIR(IRGenerator &gen, std::string &code) {
    gen.GenCompUnit(*this, code);
    return {};
}
