#include <algorithm>
#include "ast.hpp"
#include "../mid/typeck.hpp"
#include "../mid/genIR.hpp"

ASTPtr FuncDefAST::Eval(TypeCheck &checker) {
    return checker.EvalFuncDef(*this);
}

ASTPtr BlockAST::Eval(TypeCheck &checker) {
    return checker.EvalBlock(*this);
}

ASTPtr BinaryExpAST::Eval(TypeCheck &checker) {
    std::initializer_list<Operator> opAdd = {Operator::ADD, Operator::SUB};
    std::initializer_list<Operator> opMul = {Operator::MUL, Operator::DIV, Operator::MOD};
    std::initializer_list<Operator> opRel = {Operator::LE, Operator::GE, Operator::LT, Operator::GT};
    std::initializer_list<Operator> opEq = {Operator::EQ, Operator::NEQ};
    std::initializer_list<Operator> opLAnd = {Operator::AND};
    std::initializer_list<Operator> opLOr = {Operator::OR};

    if (std::find(opLOr.begin(), opLOr.end(), op) != opLOr.end()) {
        return checker.EvalLOrExp(*this);
    } else if (std::find(opLAnd.begin(), opLAnd.end(), op) != opLAnd.end()) {
        return checker.EvalLAndExp(*this);
    } else if (std::find(opEq.begin(), opEq.end(), op) != opEq.end()) {
        return checker.EvalEqExp(*this);
    } else if (std::find(opRel.begin(), opRel.end(), op) != opRel.end()) {
        return checker.EvalRelExp(*this);
    } else if (std::find(opAdd.begin(), opAdd.end(), op) != opAdd.end()) {
        return checker.EvalAddExp(*this);
    } else if (std::find(opMul.begin(), opMul.end(), op) != opMul.end()) {
        return checker.EvalMulExp(*this);
    } else {
        return nullptr;
    }
}

ASTPtr IfElseAST::Eval(TypeCheck &checker) {
    return checker.EvalIfElse(*this);
}

ASTPtr WhileAST::Eval(TypeCheck &checker) {
    return checker.EvalWhile(*this);
}

ASTPtr NumberAST::Eval(TypeCheck &checker) {
    return checker.EvalNumber(*this);
}

ASTPtr ProcessedIdAST::Eval(TypeCheck &checker) {
    return std::unique_ptr<ProcessedIdAST>(this);
}

ASTPtr IdAST::Eval(TypeCheck &checker) {
    return checker.EvalId(*this);
}

ASTPtr UnaryExpAST::Eval(TypeCheck &checker) {
    return checker.EvalUnaryExp(*this);
}

ASTPtr ControlAST::Eval(TypeCheck &checker) {
    return checker.EvalControl(*this);
}

ASTPtr AssignAST::Eval(TypeCheck &checker) {
    return checker.EvalAssign(*this);
}

ASTPtr StmtAST::Eval(TypeCheck &checker) {
    return checker.EvalStmt(*this);
}

ASTPtr LValAST::Eval(TypeCheck &checker) {
    return checker.EvalLVal(*this);
}

ASTPtr FuncCallAST::Eval(TypeCheck &checker) {
    return checker.EvalFuncCall(*this);
}

ASTPtr VarDeclAST::Eval(TypeCheck &checker) {
    return checker.EvalVarDecl(*this);
}

ASTPtr VarDefAST::Eval(TypeCheck &checker) {
    return checker.EvalVarDef(*this);
}

ASTPtr InitValAST::Eval(TypeCheck &checker) {
    return checker.EvalInitVal(*this);
}

ASTPtr CompUnitAST::Eval(TypeCheck &checker) {
    return checker.EvalCompUnit(*this);
}
