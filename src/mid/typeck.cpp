#include <memory>
#include <iostream>
#include <cstddef>
#include <memory.h>

#include "../define/ast.hpp"
#include "../define/token.hpp"
#include "typeck.hpp"

// to be revised
void TypeCheck::FillInValue(int *memory, InitValAST &init) {
    logger.SetFunc("FillInValue");
    for (auto initval: init.getValues()) {
        if (dynamic_cast<InitValAST>(initval)) {
            FillInValue(memory, initval);
            logger.UnSetFunc("FillInValue");
        } else {
            auto val = EvalAddExp(dynamic_cast<BinaryExpAST>(initval));
            if (!val) {
                logger.Warn("Initialize constant array with inconstant values");
                return;
            }
            *memory = dynamic_cast<NumberAST>(val)->getVal();
            memory++;
        }
    }
}

ASTPtr TypeCheck::EvalVarDecl(VarDeclAST &varDecl) {
    logger.SetFunc("EvalVarDecl");
    if (!varDecl->isConst()) {
        ASTPtrList list;
        for (auto def: varDecl.getVarDefs()) {
            ASTPtr varDef = EvalVarDef(def);
            logger.UnSetFunc("EvalVarDecl");
            if (!varDef) {
                logger.Error("Eval var definition failed");
                return nullptr;
            }
            list.push_back(varDef);
        }
        return std::make_unique<VarDeclAST>(std::move(list));
    }
    else {
        for (auto def: varDecl.getVarDefs()) {
            EvalVarDef(def); // record const variables
            logger.UnSetFunc("EvalVarDecl");
        }
        return std::make_unique<VarDeclAST>(std::move(ASTPtrList{}));
    }
}

// to be revised
ASTPtr TypeCheck::EvalVarDef(VarDefAST &varDef) {
    logger.SetFunc("CheckVarDef");
    if (varDef->isConst()) {
        if (!varDef->getInitVal()) {
            logger.Error("Uninitialized const variable");
            return nullptr;
        }
        auto id = varDef.getVar();
        auto initVal = varDef.getInitVal()->Eval(this);
        ASTPtrList newDim;
        int size = 1;
        vector<int> ndim;
        for (auto exp: dynamic_cast<IdAST>(id)->getDim()) {
            if (dynamic_cast<NumberAST>(exp)) {
                newDim.push_back(exp);
                size *= dynamic_cast<NumberAST>(exp)->getVal();
                ndim.push_back(dynamic_cast<NumberAST>(exp)->getVal())
            } else if (dynamic_cast<BinaryExpAST>(exp)) {
                auto result = EvalBinaryExp(exp);
                logger.UnSetFunc("CheckVarDef");
                if (!result) {
                    logger.Error("Declare array with variable size");
                    return nullptr;
                }
                newDim.push_back(result);
                size *= dynamic_cast<NumberAST>(result)->getVal();
                ndim.push_back(dynamic_cast<NumberAST>(result)->getVal());
            } else if (dynamic_cast<UnaryExpAST>(exp)) {
                auto result = EvalUnaryExp(exp);
                logger.UnSetFunc("CheckVarDef");
                if (!result) {
                    logger.Error("Declare array with variable size");
                    return nullptr;
                }
                newDim.push_back(result);
                size *= dynamic_cast<NumberAST>(result)->getVal();
                ndim.push_back(dynamic_cast<NumberAST>(result)->getVal());
            }
        }
        if (!initVal) {
            logger.Error("Initialize const variable with inconstant value");
            return nullptr;
        }

        if (dynamic_cast<IdAST>(id)->getType() == VarType::ARRAY) {
            int *arrayVal = (int *)malloc(size * sizeof(int));
            int *tmp = arrayVal;
            FillInValue(tmp, initVal); // to be revised
            if (currentFunc != "") {
                FuncConstArrayTable[currentFunc][dynamic_cast<IdAST>(id)->getName()] = std::make_pair(ndim, arrayVal);
            } else {
                ConstArrayTable[dynamic_cast<IdAST>(id)->getName()] = std::make_pair(ndim, arrayVal);
            }
        } else {
            if (currentFunc != "") {
                FuncConstVarTable[currentFunc][dynamic_cast<IdAST>(id).getName()] = dynamic_cast<NumberAST>(id);
            } else {
                ConstVarTable[dynamic_cast<IdAST>(id).getName()] = dynamic_cast<NumberAST>(id);
            }
        }
    } else {
        auto id = varDef.getVar();
        if (currentFunc != "") {
            FuncVarTable[currentFunc].insert(dynamic_cast<IdAST>(id).getName());
        } else {
            if (varDef->getInitVal()) {
                auto initVal = varDef.getInitVal()->Eval(this);
                if (!initVal) {
                    logger.Error("Invalid initialization of variable");
                    return nullptr;
                } else {
                    VarTable[dynamic_cast<IdAST>(id).getName()] = dynamic_cast<NumberAST>(id);
                }
            } else {
                VarTable[dynamic_cast<IdAST>(id).getName()] = 0;
            }
        }
    }
    return varDef;
}

ASTPtr TypeCheck::EvalFuncCall(FuncCallAST func) {

}

ASTPtr TypeCheck::EvalBlock(BlockAST block) {

}

ASTPtr TypeCheck::EvalIfElse(IfElseAST stmt) {

}

ASTPtr TypeCheck::EvalWhile(WhileAST stmt) {

}

ASTPtr TypeCheck::EvalLVal(LValAST lval) {

}

ASTPtr TypeCheck::EvalAddExp(BinaryExpAST exp) {

}

ASTPtr TypeCheck::EvalMulExp(BinaryExpAST exp) {

}

ASTPtr TypeCheck::EvalUnaryExp(UnaryExpAST exp) {
    
}
