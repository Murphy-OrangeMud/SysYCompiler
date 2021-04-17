#include <memory>
#include <iostream>
#include <cstddef>
#include <memory.h>

#include "../define/ast.hpp"
#include "../define/token.hpp"
#include "typeck.hpp"

void TypeCheck::FillInValue(int *memory, InitValAST &init, std::vector<int> &dim, int i) {
    logger.SetFunc("FillInValue");
    int idx = 0;
    if (!init) {
        if (i == dim.size() - 1) {
            for (int j = 0; j < dim[i]; j++) {
                *memory = 0;
                memory++;
            }
        } else {
            for (int j = 0; j < dim[i]; j++) {
                FillInValue(memory, nullptr, dim, i + 1);
                logger.UnSetFunc("FillInValue");
            }
        }
    } else {
        for (auto initval: init.getValues()) {
            idx++;
            if (dynamic_cast<InitValAST>(initval)) {
                FillInValue(memory, initval, dim, i + 1);
                logger.UnSetFunc("FillInValue");
            } else {
                *memory = dynamic_cast<NumberAST>(val)->getVal();
                memory++;
            }
        }
        for (; idx < dim[i]; idx++) {
            FillInValue(memory, nullptr, dim, i + 1);
            logger.UnSetFunc("FillInValue");
        }
    }
}

ASTPtr TypeCheck::EvalVarDecl(VarDeclAST &varDecl) {
    logger.SetFunc("EvalVarDecl");
    if (!varDecl->isConst()) {
        ASTPtrList list;
        for (auto def: varDecl.getVarDefs()) {
            auto varDef = def->Eval(this);
            logger.UnSetFunc("EvalVarDecl");
            if (!varDef) {
                logger.Error("Eval var definition failed");
                return nullptr;
            }
            list.push_back(varDef);
        }
        return std::make_unique<VarDeclAST>(std::move(list));
    } else {
        for (auto def: varDecl.getVarDefs()) {
            def->Eval(this);  // 常量折叠
            logger.UnSetFunc("EvalVarDecl");
        }
        return std::make_unique<VarDeclAST>(std::move(ASTPtrList{}));
    }
}

std::unique_ptr<ProcessedIdAST> TypeCheck::EvalId(IdAST &id) {
    std::vector<int> ndim;
    for (auto exp: id.getDim()) {
        if (dynamic_cast<NumberAST>(exp)) {
            ndim.push_back(dynamic_cast<NumberAST>(exp)->getVal())
        } else if (dynamic_cast<BinaryExpAST>(exp)) {
            auto result = dynamic_cast<BinaryExpAST>(exp)->Eval(this);
            logger.UnSetFunc("EvalVarDef");
            if (!result) {
                logger.Error("Declare array with variable size");
                return nullptr;
            }
            ndim.push_back(dynamic_cast<NumberAST>(result)->getVal());
        } else if (dynamic_cast<UnaryExpAST>(exp)) {
            auto result = dynamic_cast<UnaryExpAST>(exp)->Eval(this);
            logger.UnSetFunc("EvalVarDef");
            if (!result) {
                logger.Error("Declare array with variable size");
                return nullptr;
            }
            ndim.push_back(dynamic_cast<NumberAST>(result)->getVal());
        }
    }
    return std::make_unique<ProcessedIdAST>(id.getName(), id.getType(), id.isConst(), ndim);
}

ASTPtr TypeCheck::EvalVarDef(VarDefAST &varDef) {
    logger.SetFunc("EvalVarDef");
    if (varDef->isConst()) {
        if (!varDef->getInitVal()) {
            logger.Error("Uninitialized const variable");
            return nullptr;
        }
        auto id = varDef.getVar()->Eval(this);
        auto initVal = varDef.getInitVal()->Eval(this);
        logger.UnSetFunc("EvalVarDef");
        if (!initVal) {
            logger.Error("Initialize const variable with inconstant value");
            return nullptr;
        }
        int size = 1;
        for (auto x: id.getDim()) {
            size *= x;
        }
        if (id.getType() == VarType::ARRAY) {
            int *arrayVal = (int *) malloc(size * sizeof(int));
            int *tmp = arrayVal;
            FillInValue(tmp, initVal, ndim, 0);
            logger.UnSetFunc("EvalVarDef");
            if (currentFunc != "") {
                FuncConstArrayTable[currentFunc][id.getName()] = std::make_pair(ndim, arrayVal);
            } else {
                ConstArrayTable[id.getName()] = std::make_pair(ndim, arrayVal);
            }
        } else {
            if (currentFunc != "") {
                FuncConstVarTable[currentFunc][id.getName()] = initVal;
            } else {
                ConstVarTable[id.getName()] = initVal;
            }
        }
    } else {
        auto id = varDef.getVar()->Eval(this);
        if (dynamic_cast<IdAST>(id)->getType() == VarType::ARRAY) {
            int size = 1;
            for (auto x: id.getDim()) {
                size *= x;
            }
            // 在类型检查步骤不需要为变量定义求值
            if (currentFunc != "") {
                FuncArrayTable[currentFunc].insert(ndim);
            } else {
                if (varDef.getInitVal()) {
                    auto initVal = varDef.getInitVal()->Eval(this);
                    if (!initVal) {
                        logger.Error("Invalid initialization of global array");
                        return nullptr;
                    } else {
                        int *arrayVal = (int *) malloc(size * sizeof(int));
                        int *tmp = arrayVal;
                        FillInValue(tmp, initVal, ndim, 0);
                        logger.UnSetFunc("EvalVarDef");
                        ArrayTable[dynamic_cast<IdAST>(id).getName()] = std::make_pair(ndim, arrayVal);
                    }
                }
            }
        } else {
            if (currentFunc != "") {
                FuncVarTable[currentFunc].insert(dynamic_cast<IdAST>(id).getName());
            } else {
                if (varDef->getInitVal()) {
                    auto initVal = varDef.getInitVal()->Eval(this);
                    if (!initVal) {
                        logger.Error("Invalid initialization of global variable");
                        return nullptr;
                    } else {
                        VarTable[dynamic_cast<IdAST>(id).getName()] = initVal;
                    }
                } else {
                    VarTable[dynamic_cast<IdAST>(id).getName()] = 0;
                }
            }
        }
    }
    return varDef;
}

ASTPtr TypeCheck::EvalFuncCall(FuncCallAST func) {
    logger.SetFunc("EvalFuncCall");
    if (FuncTable.find(func.getName()) == FuncTable.end()) {
        logger.Error("Undefined function");
        return nullptr;
    } else {
        if (func.getArgs().size() != FuncTable[func.getName()].second.size()) {
            logger.Error("Mismatched argument number");
            return nullptr;
        }
        for (auto arg: func.getArgs()) {

        }
    }
}

ASTPtr TypeCheck::EvalBlock(BlockAST block) {
    ASTPtrList stmts;
    for (auto stmt: block.getStmts()) {
        stmts.push_back(stmt->Eval(this));
    }
    return std::make_unique<BlockAST>(stmts);
}

ASTPtr TypeCheck::EvalIfElse(IfElseAST stmt) {
    auto cond = stmt.getCond()->Eval(this);
    auto then = stmt.getThenStmt()->Eval(this);
    auto els = stmt.getElseStmt()->Eval(this);
    return std::make_unique<IfElseAST>(cond, then, els);
}

ASTPtr TypeCheck::EvalWhile(WhileAST stmt) {
    auto cond = stmt.getCond()->Eval(this);
    auto body = stmt.getStmt()->Eval(this);
    return std::make_unique<WhileAST>(cond, body);
}

ASTPtr TypeCheck::EvalControl(ControlAST stmt) {
    if (stmt.getControl() == Token::RETURN) {
        if (FuncTable[currentFunc].first == Type::VOID) {
            if (stmt.getReturnExp() != nullptr) {
                logger.Error("Return type do not match in void function: %s", currentFunc);
                return nullptr;
            }
        } else {
            if (stmt.getReturnExp() == nullptr) {
                logger.Warn("Return undefined value in int typed function");
            } else {
                auto exp = stmt.getReturnExp()->Eval(this);
                return std::make_unique<ControlAST>(stmt.getControl(), exp);
            }
        }
    }
    return stmt;
}

ASTPtr TypeCheck::EvalLVal(LValAST lval) {
    if (lval.getType() == VarType::ARRAY) {
        std::vector<int> pos;
        for (auto exp: lval.getPosition()) {
            auto val = exp->Eval(this);
            logger.UnSetFunc("EvalUnaryExp");
            if (!val || !dynamic_cast<NumberAST>(val)) {
                logger.Error("Const Expression needed");
                return nullptr;
            }
            pos.push_back(dynamic_cast<NumberAST>(val)->getVal());
        }
        std::string name = lval.getName();
        std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator iter;

        // 需要常量值
        if (currentFunc == "") {
            iter = ConstArrayTable.find(name);
            if (iter == ConstArrayTable.end()) {
                logger.Error("Undefined identifier or inconstant value: %s", name);
                return nullptr;
            }
        } else {
            iter = FuncConstArrayTable[currentFunc].find(name);
            if (iter == FuncConstArrayTable[currentFunc].end()) {
                logger.Error("Undefined identifier or inconstant value: %s", name);
                return nullptr;
            }
        }

        int position = 1;
        if (pos.size() != iter->second.first.size()) {
            logger.Error("Mismatched array position: %s", name);
            return nullptr;
        }
        int volume = 1;
        for (int i = pos.size() - 1; i >= 0; i--) {
            if (pos[i] >= iter->second.first[i]) {
                logger.Error("Array index out of bound");
                return nullptr;
            }
            position += pos[i] * volume;
            volume *= iter->second.first[i];
        }
        int value = iter->second.second + position;
        switch (exp.getOp()) {
            case Operator::NOT:
                return std::make_unique<NumberAST>(value);
            case Operator::SUB:
                return std::make_unique<NumberAST>(-value);
            case Operator::ADD:
                return std::make_unique<NumberAST>(value);
            case Operator::NONE:
                return std::make_unique<NumberAST>(value);
            default:
                logger.Error("Invalid unary exp operator");
                return nullptr;
        }
    } else {
        // var
        std::string name = dynamic_cast<LValAST>(exp.getNode())->getName();
        if (currentFunc == "") {
            std::map<std::string, int>::iterator iter = ConstVarTable.find(name);
            if (iter == ConstVarTable.end()) {
                logger.Error("Undefined identifier or inconstant value: %s", name);
                return nullptr;
            }
            return std::make_unique<NumberAST>(iter->second);
        } else {
            std::map<std::string, int>::iterator iter = FuncConstVarTable[currentFunc].find(name);
            if (iter == FuncConstVarTable[currentFunc].end()) {
                logger.Error("Undefined identifier or inconstant value: %s", name);
                return nullptr;
            }
            int value = iter->second;
            switch (exp.getOp()) {
                case Operator::NOT:
                    return std::make_unique<NumberAST>(value);
                case Operator::SUB:
                    return std::make_unique<NumberAST>(-value);
                case Operator::ADD:
                    return std::make_unique<NumberAST>(value);
                case Operator::NONE:
                    return std::make_unique<NumberAST>(value);
                default:
                    logger.Error("Invalid unary exp operator");
                    return nullptr;
            }
        }
    }
}

ASTPtr TypeCheck::EvalInitVal(InitValAST init) {
    logger.SetFunc("EvalInitVal");
    ASTPtrList newInitVals;
    for (auto val: init.getValues()) {
        auto newVal = val->Eval(this);
        logger.UnSetFunc("EvalInitVal");
        if (!newVal) {
            logger.Error("Initialize global or constant variable with inconstant values");
            return nullptr;
        }
        newInitVals.push_back(newVal);
    }
    return std::make_unique<InitValAST>(init.getType(), newInitVals);
}

ASTPtr TypeCheck::EvalAddExp(BinaryExpAST exp) {
    logger.SetFunc("EvalAddExp");
    exp.getLHS()->Eval(this);
    logger.UnSetFunc("EvalAddExp");
    if (!lval || !dynamic_cast<NumberAST>(lval)) {
        logger.Error("Parse rhs for eval_mul_exp failed");
        return nullptr;
    }
    exp.getRHS()->Eval(this);
    logger.UnSetFunc("EvalAddExp");
    if (!rval || !dynamic_cast<NumberAST>(rval)) {
        logger.Error("Parse rhs for eval_mul_exp failed");
        return nullptr;
    }
    switch (exp.getOp()) {
        case Operator::ADD{
                    return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST>(lval)->getVal()
                    + dynamic_cast<NumberAST>(rval)->getVal());
            }
        case Operator::SUB{
                    return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST>(lval)->getVal()
                    - dynamic_cast<NumberAST>(rval)->getVal());
            }
        default:
            logger.Error("Invalid operator");
            return nullptr;
    }
}

ASTPtr TypeCheck::EvalMulExp(BinaryExpAST exp) {
    logger.SetFunc("EvalMulExp");
    exp.getLHS()->Eval(this);
    logger.UnSetFunc("EvalMulExp");
    if (!lval || !dynamic_cast<NumberAST>(lval)) {
        logger.Error("Parse lhs for eval_mul_exp failed");
        return nullptr;
    }
    exp.getRHS()->Eval(this);
    logger.UnSetFunc("EvalMulExp");
    if (!rval || !dynamic_cast<NumberAST>(rval)) {
        logger.Error("Parse rhs for eval_mul_exp failed");
        return nullptr;
    }
    switch (exp.getOp()) {
        case Operator::MUL{
                    return std::make_unique<NumberAST>(
                            dynamic_cast<NumberAST>(lval)->getVal()
                            * dynamic_cast<NumberAST>(rval)->getVal());
            }
        case Operator::DIV{
                    if (dynamic_cast<NumberAST>(rval)->getVal() == 0) {
                        logger.Error("0 for divisor");
                        return nullptr;
                    }
                    return std::make_unique<NumberAST>(
                            dynamic_cast<NumberAST>(lval)->getVal()
                            / dynamic_cast<NumberAST>(rval)->getVal());
            }
        case Operator::MOD{
                    if (dynamic_cast<NumberAST>(rval)->getVal() == 0) {
                        logger.Error("0 for modulo");
                        return nullptr;
                    }
                    return std::make_unique<NumberAST>(
                            dynamic_cast<NumberAST>(lval)->getVal()
                            % dynamic_cast<NumberAST>(rval)->getVal());
            }
        default:
            logger.Error("Invalid operator");
            return nullptr;
    }
}

ASTPtr TypeCheck::EvalUnaryExp(UnaryExpAST exp) {
    logger.SetFunc("EvalUnaryExp");
    if (dynamic_cast<NumberAST>(exp.getNode())) {
        if (exp.getOp() == Operator::NONE) {
            return std::make_unique<NumberAST>(dynamic_cast<NumberAST>(exp.getNode())->getVal());
        } else {
            switch (exp.getOp()) {
                case Operator::NOT:
                    return std::make_unique<NumberAST>(!dynamic_cast<NumberAST>(exp.getNode())->getVal());
                case Operator::SUB:
                    return std::make_unique<NumberAST>(-dynamic_cast<NumberAST>(exp.getNode())->getVal());
                case Operator::ADD:
                    return std::make_unique<NumberAST>(dynamic_cast<NumberAST>(exp.getNode())->getVal());
                default:
                    logger.Error("Invalid unary exp operator");
                    return nullptr;
            }
        }
    } else if (dynamic_cast<LValAST>(exp.getNode())) {

    } else if (dynamic_cast<BinaryExpAST>(exp.getNode())) {
        auto val = EvalAddExp(exp.getNode());
        logger.UnSetFunc("EvalUnaryExp");
        if (!val || !dynamic_cast<NumberAST>(val)) {
            logger.Error("Inconstant value");
            return nullptr;
        }
        int value = dynamic_cast<NumberAST>(val)->getVal();
        switch (exp.getOp()) {
            case Operator::NOT:
                return std::make_unique<NumberAST>(value);
            case Operator::SUB:
                return std::make_unique<NumberAST>(-value);
            case Operator::ADD:
                return std::make_unique<NumberAST>(value);
            case Operator::NONE:
                return std::make_unique<NumberAST>(value);
            default:
                logger.Error("Invalid unary exp operator");
                return nullptr;
        }
    } else if (dynamic_cast<UnaryExpAST>(exp.getNode())) {
        auto val = EvalUnaryExp(exp.getNode());
        if (!val || !dynamic_cast<NumberAST>(val)) {
            logger.Error("Inconstant value");
            return nullptr;
        }
        int value = dynamic_cast<NumberAST>(val)->getVal();
        switch (exp.getOp()) {
            case Operator::NOT:
                return std::make_unique<NumberAST>(value);
            case Operator::SUB:
                return std::make_unique<NumberAST>(-value);
            case Operator::ADD:
                return std::make_unique<NumberAST>(value);
            case Operator::NONE:
                return std::make_unique<NumberAST>(value);
            default:
                logger.Error("Invalid unary exp operator");
                return nullptr;
        }
    }
}

ASTPtr TypeCheck::EvalFuncDef(FuncDefAST funcDef) {
    logger.SetFunc("EvalFuncDef");
    currentFunc = funcDef.getName();
    ASTPtrList newArgs;
    std::vector < std::pair < std::string, std::pair < VarType, std::vector < int >/*dims*/>>> args;
    for (auto arg: funcDef.getArgs()) {
        if (dynamic_cast<IdAST>(arg)->getType() == VarType::ARRAY) {
            std::vector<int> dims;
            dims.push_back(0);
            for (auto exp: dynamic_cast<IdAST>(arg)->getDim()) {
                ASTPtr res = EvalAddExp(exp);
                logger.UnSetFunc("EvalFuncDef");
                if (!res || !dynamic_cast<NumberAST>(res)) {
                    logger.Error("Inconstant value for typed array arg dim");
                    return nullptr;
                }
                dims.push_back(dynamic_cast<NumberAST>(res)->getVal());
            }
            newArgs.push_back(
                    std::make_unique<IdAST>(dynamic_cast<ProcessedIdAST>(arg)->getName(), VarType::ARRAY, false, dims));
            args.push_back(std::make_pair(dynamic_cast<IdAST>(arg)->getName(), std::make_pair(VarType::ARRAY, dims)));
        } else {
            newArgs.push_back(
                    std::make_unique<IdAST>(dynamic_cast<ProcessedIdAST>(arg)->getName(), VarType::VAR, false));
            args.push_back(std::make_pair(dynamic_cast<IdAST>(arg)->getName(),
                                          std::make_pair(VarType::VAR, std::vector < int > {})));
        }
    }
    FuncTable[currentFunc] = std::make_pair(funcDef.getType(), args);
    ASTPtr block = EvalBlock(funcDef.getBody());
    currentFunc = "";
    return std::make_unique<FuncDefAST>(funcDef.getType(), funcDef.getName(), newArgs, block);
}

ASTPtr TypeCheck::EvalRelExp(BinaryExpAST exp) {
    logger.SetFunc("EvalRelExp");
    auto lhs = exp.getLHS()->Eval(this);
    logger.UnSetFunc("EvalRelExp");
    if (exp.getRHS()) {
        auto rhs = exp.getRHS()->Eval(this);
        logger.UnSetFunc("EvalRelExp");
        switch(exp.getOp()) {
            case Operator::LT:
                return dynamic_cast<NumberAST>(lhs)->getVal() < dynamic_cast<NumberAST>(rhs)->getVal();
            case Operator::GT:
                return dynamic_cast<NumberAST>(lhs)->getVal() > dynamic_cast<NumberAST>(rhs)->getVal();
            case Operator::LE:
                return dynamic_cast<NumberAST>(lhs)->getVal() <= dynamic_cast<NumberAST>(rhs)->getVal();
            case Operator::GE:
                return dynamic_cast<NumberAST>(lhs)->getVal() >= dynamic_cast<NumberAST>(rhs)->getVal();
            default:
                logger.Error("Invalid operator");
                return nullptr;
        }
    } else {
        return lhs;
    }
}

ASTPtr TypeCheck::EvalLAndExp(BinaryExpAST exp) {
    logger.SetFunc("EvalLAndExp");
    auto lhs = exp.getLHS()->Eval(this);
    if (dynamic_cast<NumberAST>(lhs)->getVal() == 0) {
        return std::make_unique<NumberAST>(0);
    }
    logger.UnSetFunc("EvalLAndExp");
    auto rhs = exp.getRHS()->Eval(this);
    logger.UnSetFunc("EvalLAndExp");
    if (!rhs) {
        logger.Error("Eval rhs failed for land exp");
        return nullptr;
    }
    if (dynamic_cast<NumberAST>(lhs)->getVal()) {
        return std::make_unique<NumberAST>(1);
    } else {
        return std::make_unique<NumberAST>(0);
    }
}

ASTPtr TypeCheck::EvalLOrExp(BinaryExpAST exp) {
    logger.SetFunc("EvalLOrExp");
    auto lhs = exp.getLHS()->Eval(this);
    if (dynamic_cast<NumberAST>(lhs)->getVal()) {
        return std::make_unique<NumberAST>(1);
    }
    logger.UnSetFunc("EvalLOrExp");
    auto rhs = exp.getRHS()->Eval(this);
    logger.UnSetFunc("EvalLOrExp");
    if (!rhs) {
        logger.Error("Eval rhs failed for land exp");
        return nullptr;
    }
    if (dynamic_cast<NumberAST>(lhs)->getVal()) {
        return std::make_unique<NumberAST>(1);
    } else {
        return std::make_unique<NumberAST>(0);
    }
}

ASTPtr TypeCheck::EvalCompUnit(CompUnitAST unit) {
    logger.SetFunc("EvalCompUnit");
    ASTPtrList newNodes;
    for (auto node: unit.getNodes()) {
        auto newNode = node->Eval(this);
        logger.UnSetFunc("EvalCompUnit");
        if (!newNode) {
            logger.Error("Eval node failed");
            return nullptr;
        }
        newNodes.push_back(newNode);
    }
    return std::make_unique<CompUnitAST>(newNodes);
}

ASTPtr TypeCheck::EvalEqExp(BinaryExpAST exp) {
    logger.SetFunc("EvalEqExp");
    auto lhs = exp.getLHS()->Eval(this);
    logger.UnSetFunc("EvalEqExp");
    if (!lhs) {
        logger.Error("Eval lhs failed for eq exp");
        return nullptr;
    }
    auto rhs = exp.getRHS()->Eval(this);
    logger.UnSetFunc("EvalEqExp");
    if (!rhs) {
        logger.Error("Eval rhs failed for eq exp");
        return nullptr;
    }
    switch(exp.getOp()) {
        case Operator::EQ:
            return dynamic_cast<NumberAST>(lhs)->getVal() == dynamic_cast<NumberAST>(rhs)->getVal();
        case Operator::NEQ:
            return dynamic_cast<NumberAST>(lhs)->getVal() != dynamic_cast<NumberAST>(rhs)->getVal();
        default:
            logger.Error("Invalid operator");
            return nullptr;
    }
}

ASTPtr TypeCheck::EvalStmt(StmtAST stmt) {
    logger.SetFunc("EvalStmt");
    if (!stmt.getStmt())
        return stmt;
    else {
        auto nStmt = stmt.getStmt()->Eval(this);
        logger.UnSetFunc("EvalStmt");
        if (!nStmt) {
            logger.Error("Eval stmt failed");
            return nullptr;
        }
        return std::make_unique<StmtAST>(nStmt);
    }
}