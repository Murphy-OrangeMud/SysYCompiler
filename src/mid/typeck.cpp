#include <memory>
#include <iostream>
#include <cstddef>
#include <algorithm>

#include "../define/ast.hpp"
#include "typeck.hpp"

bool TypeCheck::FillInValue(int *memory, InitValAST *init, std::vector<int> &dim, size_t i) {
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
                if (!FillInValue(memory, nullptr, dim, i + 1))
                    return false;
                logger.UnSetFunc("FillInValue");
            }
        }
    } else {
        for (const auto &initval: init->getValues()) {
            idx++;
            if (dynamic_cast<InitValAST *>(initval.get())) {
                if (!FillInValue(memory, dynamic_cast<InitValAST *>(initval.get()), dim, i + 1))
                    return false;
                logger.UnSetFunc("FillInValue");
            } else {
                if (!dynamic_cast<NumberAST *>(initval.get())) {
                    logger.Error("Initialize const variable with inconstant value");
                    return false;
                }
                *memory = dynamic_cast<NumberAST *>(initval.get())->getVal();
                memory++;
            }
        }
        for (; idx < dim[i]; idx++) {
            if (!FillInValue(memory, nullptr, dim, i + 1))
                return false;
            logger.UnSetFunc("FillInValue");
        }
    }
    return true;
}

std::unique_ptr<VarDeclAST> TypeCheck::EvalVarDecl(VarDeclAST &varDecl) {
    logger.SetFunc("EvalVarDecl");
    if (!varDecl.isConst()) {
        ASTPtrList list;
        for (const auto &def: varDecl.getVarDefs()) {
            auto varDef = def->Eval(*this);
            logger.UnSetFunc("EvalVarDecl");
            if (!varDef) {
                logger.Error("Eval var definition failed");
                return nullptr;
            }
            list.push_back(std::move(varDef));
        }
        return std::make_unique<VarDeclAST>(varDecl.isConst(), std::move(list));
    } else {
        ASTPtrList list;
        for (const auto &def: varDecl.getVarDefs()) {
            auto nDef = def->Eval(*this);  // 变量常量折叠，不放入语法树
            logger.UnSetFunc("EvalVarDecl");
            if (dynamic_cast<ProcessedIdAST *>(dynamic_cast<VarDefAST *>(nDef.get())->getVar().get())->getType() ==
                VarType::ARRAY) {
                list.push_back(std::move(nDef));
            }
        }
        return std::make_unique<VarDeclAST>(varDecl.isConst(), std::move(list));
    }
}

std::unique_ptr<ProcessedIdAST> TypeCheck::EvalId(IdAST &id) {
    logger.SetFunc("EvalId");
    std::vector<int> ndim;
    for (const auto &exp: id.getDim()) {
        if (dynamic_cast<NumberAST *>(exp.get())) {
            ndim.push_back(dynamic_cast<NumberAST *>(exp.get())->getVal());
        } else if (dynamic_cast<BinaryExpAST *>(exp.get())) {
            auto result = dynamic_cast<BinaryExpAST *>(exp.get())->Eval(*this);
            logger.UnSetFunc("EvalId");
            if (!dynamic_cast<NumberAST *>(result.get())) {
                logger.Error("Declare array with variable size");
                return nullptr;
            }
            ndim.push_back(dynamic_cast<NumberAST *>(result.get())->getVal());
        } else if (dynamic_cast<UnaryExpAST *>(exp.get())) {
            auto result = dynamic_cast<UnaryExpAST *>(exp.get())->Eval(*this);
            logger.UnSetFunc("EvalId");
            if (!dynamic_cast<NumberAST *>(result.get())) {
                logger.Error("Declare array with variable size");
                return nullptr;
            }
            ndim.push_back(dynamic_cast<NumberAST *>(result.get())->getVal());
        }
    }
    return std::make_unique<ProcessedIdAST>(id.getName(), id.getType(), id.isConst(), std::move(ndim));
}

// FillInValue没用，只是用来检查初始值是否都是常量
std::unique_ptr<VarDefAST> TypeCheck::EvalVarDef(VarDefAST &varDef) {
    logger.SetFunc("EvalVarDef");
    if (varDef.isConst()) {
        if (!varDef.getInitVal()) {
            logger.Error("Uninitialized const variable");
            return nullptr;
        }
        auto id = varDef.getVar()->Eval(*this); // ProcessedIdAST
        std::string name = dynamic_cast<ProcessedIdAST *>(id.get())->getName();
        VarType type = dynamic_cast<ProcessedIdAST *>(id.get())->getType();
        std::vector<int> ndim = dynamic_cast<ProcessedIdAST *>(id.get())->getDim();
        logger.UnSetFunc("EvalVarDef");
        auto initVal = varDef.getInitVal()->Eval(*this);
        logger.UnSetFunc("EvalVarDef");
        if (!initVal) {
            logger.Error("Eval initVal failed");
            return nullptr;
        }
        logger.Info("Var name: " + name + ", var type: " + std::to_string(type));
        if (type == VarType::ARRAY) {
            int size = 1;
            for (auto x: ndim) {
                size *= x;
            }
            int *arrayVal = (int *) malloc(size * sizeof(int));
            int *tmp = arrayVal;
            if (!FillInValue(tmp, dynamic_cast<InitValAST *>(initVal.get()), ndim, 0)) {
                logger.Error("Initialize const variable with inconstant value");
                return nullptr;
            }
            logger.UnSetFunc("EvalVarDef");
            if (!currentFunc.empty()) {
                if (FuncConstArrayTable[currentFunc].find(name) != FuncConstArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncArrayTable[currentFunc].find(name) != FuncArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncConstVarTable[currentFunc].find(name) != FuncConstVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncVarTable[currentFunc].find(name) != FuncVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                for (auto &i : FuncTable[currentFunc].second) {
                    if (i.first == name) {
                        std::string message = "Repeated definition: " + name;
                        logger.Error(message);
                        return nullptr;
                    }
                }
                FuncConstArrayTable[currentFunc][name] = ndim;
            } else {
                if (ConstArrayTable.find(name) != ConstArrayTable.end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (ArrayTable.find(name) != ArrayTable.end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (ConstVarTable.find(name) != ConstVarTable.end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (VarTable.find(name) != VarTable.end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                ConstArrayTable[name] = ndim;
            }
        } else {
            if (!currentFunc.empty()) {
                if (FuncConstVarTable[currentFunc].find(name) != FuncConstVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncVarTable[currentFunc].find(name) != FuncVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncConstArrayTable[currentFunc].find(name) != FuncConstArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncArrayTable[currentFunc].find(name) != FuncArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                for (auto &i : FuncTable[currentFunc].second) {
                    if (i.first == name) {
                        std::string message = "Repeated definition: " + name;
                        logger.Error(message);
                        return nullptr;
                    }
                }
                if (!dynamic_cast<NumberAST *>(initVal.get())) {
                    logger.Error("Initialize constant variable with inconstant value");
                    return nullptr;
                }
                FuncConstVarTable[currentFunc][name] = dynamic_cast<NumberAST *>(initVal.get())->getVal();
            } else {
                if (ConstVarTable.find(name) != ConstVarTable.end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (VarTable.find(name) != VarTable.end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (ConstArrayTable.find(name) != ConstArrayTable.end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (ArrayTable.find(name) != ArrayTable.end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (!dynamic_cast<NumberAST *>(initVal.get())) {
                    logger.Error("Initialize constant variable with inconstant value");
                    return nullptr;
                }
                ConstVarTable[name] = dynamic_cast<NumberAST *>(initVal.get())->getVal();
            }
        }

        dynamic_cast<InitValAST*>(initVal.get())->setDim(ndim);
        return std::make_unique<VarDefAST>(true, std::move(id), std::move(initVal));
    } else {
        auto id = varDef.getVar()->Eval(*this);
        std::string name = dynamic_cast<ProcessedIdAST *>(id.get())->getName();
        VarType type = dynamic_cast<ProcessedIdAST *>(id.get())->getType();
        std::vector<int> ndim = dynamic_cast<ProcessedIdAST *>(id.get())->getDim();
        logger.UnSetFunc("EvalVarDef");
        logger.Info("Var name: " + name + ", var type: " + std::to_string(type));
        if (type == VarType::ARRAY) {
            int size = 1;
            for (auto x: ndim) {
                size *= x;
            }
            // 在类型检查步骤不需要为变量定义求值
            if (!currentFunc.empty()) {
                FuncArrayTable[currentFunc][name] = ndim;
                auto initVal = varDef.getInitVal()->Eval(*this);
                logger.UnSetFunc("EvalVarDef");

                dynamic_cast<InitValAST*>(initVal.get())->setDim(ndim);
                return std::make_unique<VarDefAST>(true, std::move(id), std::move(initVal));
            } else {
                // global must initialize with constant value
                if (varDef.getInitVal()) {
                    auto initVal = varDef.getInitVal()->Eval(*this);
                    logger.UnSetFunc("EvalVarDef");
                    if (!initVal) {
                        logger.Error("Invalid initialization of global array");
                        return nullptr;
                    } else {
                        int *arrayVal = (int *) malloc(size * sizeof(int));
                        int *tmp = arrayVal;
                        if (!FillInValue(tmp, dynamic_cast<InitValAST *>(initVal.get()), ndim, 0)) {
                            logger.Error("Initialize const variable with inconstant value");
                            return nullptr;
                        }
                        logger.UnSetFunc("EvalVarDef");
                        if (ArrayTable.find(name) != ArrayTable.end()) {
                            std::string message = "Repeated definition: " + name;
                            logger.Error(message);
                            return nullptr;
                        }
                        if (ConstArrayTable.find(name) != ConstArrayTable.end()) {
                            std::string message = "Repeated definition: " + name;
                            logger.Error(message);
                            return nullptr;
                        }
                        ArrayTable[name] = ndim;
                    }

                    dynamic_cast<InitValAST*>(initVal.get())->setDim(ndim);
                    return std::make_unique<VarDefAST>(true, std::move(id), std::move(initVal));
                } else {
                    if (ArrayTable.find(name) != ArrayTable.end()) {
                        std::string message = "Repeated definition: " + name;
                        logger.Error(message);
                        return nullptr;
                    }
                    if (ConstArrayTable.find(name) != ConstArrayTable.end()) {
                        std::string message = "Repeated definition: " + name;
                        logger.Error(message);
                        return nullptr;
                    }
                    ArrayTable[name] = ndim;
                }
                return std::make_unique<VarDefAST>(false, std::move(id));
            }
        } else {
            if (!currentFunc.empty()) {
                if (FuncVarTable[currentFunc].find(name) != FuncVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncConstVarTable[currentFunc].find(name) != FuncConstVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncConstArrayTable[currentFunc].find(name) != FuncConstArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncArrayTable[currentFunc].find(name) != FuncArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + name;
                    logger.Error(message);
                    return nullptr;
                }
                for (auto &i : FuncTable[currentFunc].second) {
                    if (i.first == name) {
                        std::string message = "Repeated definition: " + name;
                        logger.Error(message);
                        return nullptr;
                    }
                }
                FuncVarTable[currentFunc].insert(name);
                if (varDef.getInitVal()) {
                    auto initVal = varDef.getInitVal()->Eval(*this);
                    if (!initVal) {
                        logger.Error("eval init val failed");
                        return nullptr;
                    }
                    return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id), std::move(initVal));
                }
                return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id));
            } else {
                if (varDef.getInitVal()) {
                    auto initVal = varDef.getInitVal()->Eval(*this);
                    if (!initVal) {
                        logger.Error("Invalid initialization of global variable");
                        return nullptr;
                    } else {
                        if (!dynamic_cast<NumberAST *>(initVal.get())) {
                            logger.Error("Mismatched type in variable initialization");
                            return nullptr;
                        }
                        if (ConstVarTable.find(name) != ConstVarTable.end()) {
                            std::string message = "Repeated definition: " + name;
                            logger.Error(message);
                            return nullptr;
                        }
                        if (VarTable.find(name) != VarTable.end()) {
                            std::string message = "Repeated definition: " + name;
                            logger.Error(message);
                            return nullptr;
                        }
                        VarTable[name] = dynamic_cast<NumberAST *>(initVal.get())->getVal();
                        return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id), std::move(initVal));
                    }
                } else {
                    if (ConstVarTable.find(name) != ConstVarTable.end()) {
                        std::string message = "Repeated definition: " + name;
                        logger.Error(message);
                        return nullptr;
                    }
                    if (VarTable.find(name) != VarTable.end()) {
                        std::string message = "Repeated definition: " + name;
                        logger.Error(message);
                        return nullptr;
                    }
                    VarTable[name] = 0;
                    ASTPtrList retlist;
                    retlist.push_back(std::make_unique<NumberAST>(0));
                    return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id),
                                                       std::make_unique<InitValAST>(VarType::VAR,
                                                                                             std::move(retlist)));
                }
            }
        }
    }
}

std::unique_ptr<FuncCallAST> TypeCheck::EvalFuncCall(FuncCallAST &func) {
    logger.SetFunc("EvalFuncCall");
    if (FuncTable.find(func.getName()) == FuncTable.end()) {
        logger.Error("Undefined function");
        return nullptr;
    } else {
        if (func.getArgs().size() != FuncTable[func.getName()].second.size()) {
            logger.Error("Mismatched argument number");
            return nullptr;
        }
        ASTPtrList newArgs;
        for (size_t i = 0; i < func.getArgs().size(); i++) {
            auto arg = func.getArgs()[i]->Eval(*this);
            if (FuncTable[func.getName()].second[i].second.first == VarType::ARRAY) {
                if (dynamic_cast<NumberAST *>(arg.get())) {
                    logger.Error("Unmatched parameter type: int[] and int");
                    return nullptr;
                }
                if (dynamic_cast<BinaryExpAST *>(arg.get())) {
                    logger.Error("Unmatched parameter type: int[] and exp");
                    return nullptr;
                }
                if (dynamic_cast<FuncCallAST *>(arg.get())) {
                    logger.Error("Unmatched parameter type: int[] and returned int");
                    return nullptr;
                }
                if (dynamic_cast<UnaryExpAST *>(arg.get())) {
                    logger.Error("Unmatched parameter type: int[] and unary exp");
                    return nullptr;
                }
                if (dynamic_cast<LValAST *>(arg.get())) {
                    if (dynamic_cast<LValAST *>(arg.get())->getType() == VarType::VAR) {
                        logger.Error("Unmatched parameter type: int[] and variable");
                        return nullptr;
                    } else {
                        if (!currentFunc.empty()) {
                            auto i2 = FuncArrayTable[currentFunc].find(dynamic_cast<LValAST *>(arg.get())->getName());
                            auto i4 = FuncConstArrayTable[currentFunc].find(
                                    dynamic_cast<LValAST *>(arg.get())->getName());
                            if (i2 == FuncArrayTable[currentFunc].end()) {
                                if (i4 == FuncConstArrayTable[currentFunc].end()) {
                                    goto out1;
                                }
                                if (i4->second.size() ==
                                    dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                    logger.Error("Unmatched parameter type: int[] and variable");
                                    return nullptr;
                                }
                                for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                     j < i4->second.size(); j++) {
                                    if (i4->second[j] != FuncTable[func.getName()].second[i].second.second[j -
                                                                                                           dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                        logger.Error("Unmatched parameter dim");
                                        return nullptr;
                                    }
                                }
                            } else {
                                if (i2->second.size() ==
                                    dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                    logger.Error("Unmatched parameter type: int[] and variable");
                                    return nullptr;
                                }
                                for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                     j < i2->second.size(); j++) {
                                    if (i2->second[j] != FuncTable[func.getName()].second[i].second.second[j -
                                                                                                           dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                        logger.Error("Unmatched parameter dim");
                                        return nullptr;
                                    }
                                }
                            }
                        }
                        out1:
                        auto i1 = ArrayTable.find(dynamic_cast<LValAST *>(arg.get())->getName());
                        auto i3 = ConstArrayTable.find(dynamic_cast<LValAST *>(arg.get())->getName());
                        if (i1 == FuncArrayTable[currentFunc].end()) {
                            if (i3 == FuncConstArrayTable[currentFunc].end()) {
                                std::string message =
                                        "Undefined identifier " + dynamic_cast<LValAST *>(arg.get())->getName();
                                logger.Error(message);
                                return nullptr;
                            }
                            if (i3->second.size() ==
                                dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                logger.Error("Unmatched parameter type: int[] and variable");
                                return nullptr;
                            }
                            for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                 j < i3->second.size(); j++) {
                                if (i3->second[j] != FuncTable[func.getName()].second[i].second.second[j -
                                                                                                       dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                    logger.Error("Unmatched parameter dim");
                                    return nullptr;
                                }
                            }
                        } else {
                            if (i1->second.size() ==
                                dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                logger.Error("Unmatched parameter type: int[] and variable");
                                return nullptr;
                            }
                            for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                 j < i1->second.size(); j++) {
                                if (i1->second[j] != FuncTable[func.getName()].second[i].second.second[j -
                                                                                                       dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                    logger.Error("Unmatched parameter dim");
                                    return nullptr;
                                }
                            }
                        }
                    }
                }
            } else {
                if (dynamic_cast<LValAST *>(arg.get())) {
                    if (dynamic_cast<LValAST *>(arg.get())->getType() == VarType::ARRAY) {
                        if (!currentFunc.empty()) {
                            auto i2 = FuncArrayTable[currentFunc].find(
                                    dynamic_cast<LValAST *>(arg.get())->getName());
                            auto i4 = FuncConstArrayTable[currentFunc].find(
                                    dynamic_cast<LValAST *>(arg.get())->getName());
                            if (i2 == FuncArrayTable[currentFunc].end()) {
                                if (i4 == FuncConstArrayTable[currentFunc].end()) {
                                    goto out2;
                                }
                                if (i4->second.size() ==
                                    dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                    logger.Error("Unmatched parameter type: int[] and variable");
                                    return nullptr;
                                }
                                for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                     j < i4->second.size(); j++) {
                                    if (i4->second[j] != FuncTable[func.getName()].second[i].second.second[j -
                                                                                                           dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                        logger.Error("Unmatched parameter dim");
                                        return nullptr;
                                    }
                                }
                            } else {
                                if (i2->second.size() ==
                                    dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                    logger.Error("Unmatched parameter type: int[] and variable");
                                    return nullptr;
                                }
                                for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                     j < i2->second.size(); j++) {
                                    if (i2->second[j] != FuncTable[func.getName()].second[i].second.second[j -
                                                                                                           dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                        logger.Error("Unmatched parameter dim");
                                        return nullptr;
                                    }
                                }
                            }
                        }
                        out2:
                        auto i1 = ArrayTable.find(dynamic_cast<LValAST *>(arg.get())->getName());
                        auto i3 = ConstArrayTable.find(dynamic_cast<LValAST *>(arg.get())->getName());
                        if (i1 == FuncArrayTable[currentFunc].end()) {
                            if (i3 == FuncConstArrayTable[currentFunc].end()) {
                                std::string message =
                                        "Undefined identifier " + dynamic_cast<LValAST *>(arg.get())->getName();
                                logger.Error(message);
                                return nullptr;
                            }
                            if (i3->second.size() ==
                                dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                logger.Error("Unmatched parameter type: int[] and variable");
                                return nullptr;
                            }
                            for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                 j < i3->second.size(); j++) {
                                if (i3->second[j] != FuncTable[func.getName()].second[i].second.second[j -
                                                                                                       dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                    logger.Error("Unmatched parameter dim");
                                    return nullptr;
                                }
                            }
                        } else {
                            if (i1->second.size() ==
                                dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                logger.Error("Unmatched parameter type: int[] and variable");
                                return nullptr;
                            }
                            for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                 j < i1->second.size(); j++) {
                                if (i1->second[j] != FuncTable[func.getName()].second[i].second.second[j -
                                                                                                       dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                    logger.Error("Unmatched parameter dim");
                                    return nullptr;
                                }
                            }
                        }
                    }
                }
            }
            newArgs.push_back(std::move(arg));
        }
        return std::make_unique<FuncCallAST>(func.getName(), std::move(newArgs));
    }
}

std::unique_ptr<BlockAST> TypeCheck::EvalBlock(BlockAST &block) {
    logger.SetFunc("EvalBlock");
    ASTPtrList stmts;
    for (const auto &stmt: block.getStmts()) {
        auto nStmt = stmt->Eval(*this);
        if (!nStmt) {
            logger.Error("Eval stmt failed for block");
            return nullptr;
        }
        logger.UnSetFunc("EvalBlock");
        stmts.push_back(std::move(nStmt));

    }
    return std::make_unique<BlockAST>(std::move(stmts));
}

std::unique_ptr<IfElseAST> TypeCheck::EvalIfElse(IfElseAST &stmt) {
    logger.SetFunc("EvalIfElse");
    auto cond = stmt.getCond()->Eval(*this);
    logger.UnSetFunc("EvalIfElse");
    if (!cond) {
        logger.Error("Eval cond stmt failed for if else");
        return nullptr;
    }
    auto then = stmt.getThenStmt()->Eval(*this);
    logger.UnSetFunc("EvalIfElse");
    if (!then) {
        logger.Error("Eval then stmt failed for if else");
        return nullptr;
    }
    if (stmt.getElseStmt()) {
        auto els = stmt.getElseStmt()->Eval(*this);
        logger.UnSetFunc("EvalIfElse");
        if (!els) {
            logger.Error("Eval else stmt failed for if else");
            return nullptr;
        }
        return std::make_unique<IfElseAST>(std::move(cond), std::move(then), std::move(els));
    }
    return std::make_unique<IfElseAST>(std::move(cond), std::move(then));
}

std::unique_ptr<WhileAST> TypeCheck::EvalWhile(WhileAST &stmt) {
    logger.SetFunc("EvalWhile");
    auto cond = stmt.getCond()->Eval(*this);
    logger.UnSetFunc("EvalWhile");
    if (!cond) {
        logger.Error("Eval cond stmt failed for while");
        return nullptr;
    }
    auto body = stmt.getStmt()->Eval(*this);
    logger.UnSetFunc("EvalWhile");
    if (!body) {
        logger.Error("Eval body stmt failed for while");
        return nullptr;
    }
    return std::make_unique<WhileAST>(std::move(cond), std::move(body));
}

std::unique_ptr<ControlAST> TypeCheck::EvalControl(ControlAST &stmt) {
    logger.SetFunc("EvalControl");
    if (stmt.getControl() == Token::RETURN) {
        if (FuncTable[currentFunc].first == Type::VOID) {
            if (stmt.getReturnExp() != nullptr) {
                logger.Error("Return type do not match in void function: " + currentFunc);
                return nullptr;
            }
        } else {
            if (stmt.getReturnExp() == nullptr) {
                logger.Warn("Return undefined value in int typed function");
            } else {
                auto exp = stmt.getReturnExp()->Eval(*this);
                logger.UnSetFunc("EvalControl");
                if (!exp) {
                    logger.Error("Eval return exp failed");
                    return nullptr;
                }
                return std::make_unique<ControlAST>(stmt.getControl(), std::move(exp));
            }
        }
    }
    return std::make_unique<ControlAST>(stmt.getControl());
}

std::unique_ptr<AssignAST> TypeCheck::EvalAssign(AssignAST &assign) {
    logger.SetFunc("EvalAssign");
    auto lhs = assign.getLeft()->Eval(*this);
    logger.UnSetFunc("EvalAssign");
    if (!lhs) {
        logger.Error("Eval lhs failed for assignment");
        return nullptr;
    }
    if (!dynamic_cast<LValAST *>(lhs.get())) {
        logger.Error("Cannot assign value to a right value");
        return nullptr;
    }
    if (!currentFunc.empty()) {
        if (FuncConstArrayTable[currentFunc].find(dynamic_cast<LValAST *>(lhs.get())->getName()) !=
            FuncConstArrayTable[currentFunc].end()) {
            logger.Error("Cannot change the value of a constant array");
            return nullptr;
        }
        if (FuncConstVarTable[currentFunc].find(dynamic_cast<LValAST *>(lhs.get())->getName()) !=
            FuncConstVarTable[currentFunc].end()) {
            logger.Error("Cannot change the value of a constant variable");
            return nullptr;
        }
    }
    if (ConstArrayTable.find(dynamic_cast<LValAST *>(lhs.get())->getName()) != ConstArrayTable.end()) {
        logger.Error("Cannot change the value of a constant array");
        return nullptr;
    }
    if (ConstVarTable.find(dynamic_cast<LValAST *>(lhs.get())->getName()) != ConstVarTable.end()) {
        logger.Error("Cannot change the value of a constant variable");
        return nullptr;
    }
    auto rhs = assign.getRight()->Eval(*this);
    logger.UnSetFunc("EvalAssign");
    if (!rhs) {
        logger.Error("Eval rhs failed for assignment");
        return nullptr;
    }
    return std::make_unique<AssignAST>(std::move(lhs), std::move(rhs));
}

// return number ast or lval ast
ASTPtr TypeCheck::EvalLVal(LValAST &lval) {
    if (lval.getType() == VarType::ARRAY) {
        ASTPtrList pos;
        for (const auto &exp: lval.getPosition()) {
            auto val = exp->Eval(*this);
            logger.UnSetFunc("EvalUnaryExp");
            if (!val) {
                logger.Error("Eval dim failed");
                return nullptr;
            }
            pos.push_back(std::move(val));
        }
        const std::string &name = lval.getName();
        std::map<std::string, std::vector<int>/*dim*/>::iterator iter;

        // 需要常量值
        if (currentFunc.empty()) {
            iter = ConstArrayTable.find(name);
            if (iter == ConstArrayTable.end()) {
                iter = ArrayTable.find(name);
                if (iter == ArrayTable.end()) {
                    logger.Error("Undefined identifier");
                    return nullptr;
                } else {
                    return std::make_unique<LValAST>(lval.getName(), lval.getType(), std::move(pos));
                }
            }
            return std::make_unique<LValAST>(lval.getName(), lval.getType(), std::move(pos));
        } else {
            iter = FuncConstArrayTable[currentFunc].find(name);
            if (iter == FuncConstArrayTable[currentFunc].end()) {
                iter = FuncArrayTable[currentFunc].find(name);
                if (iter == FuncArrayTable[currentFunc].end()) {
                    logger.Error("Undefined identifier: " + name);
                    return nullptr;
                } else {
                    return std::make_unique<LValAST>(lval.getName(), lval.getType(), std::move(pos));
                }
            }
            return std::make_unique<LValAST>(lval.getName(), lval.getType(), std::move(pos));
        }
    } else {
        // var
        const std::string &name = lval.getName();
        if (currentFunc.empty()) {
            auto iter = ConstVarTable.find(name);
            if (iter == ConstVarTable.end()) {
                iter = VarTable.find(name);
                if (iter == VarTable.end()) {
                    logger.Error("Undefined identifier :" + name);
                    return nullptr;
                } else {
                    return std::make_unique<LValAST>(lval.getName(), lval.getType());
                }
            }
            return std::make_unique<NumberAST>(iter->second);
        } else {
            auto iter = FuncConstVarTable[currentFunc].find(name);
            if (iter == FuncConstVarTable[currentFunc].end()) {
                auto iter2 = FuncVarTable[currentFunc].find(name);
                if (iter2 == FuncVarTable[currentFunc].end()) {
                    logger.Error("Undefined identifier " + name);
                    return nullptr;
                }
                return std::make_unique<LValAST>(lval.getName(), lval.getType());
            }
            return std::make_unique<NumberAST>(iter->second);
        }
    }
}

std::unique_ptr<InitValAST> TypeCheck::EvalInitVal(InitValAST &init) {
    logger.SetFunc("EvalInitVal");
    ASTPtrList newInitVals;
    for (const auto &val: init.getValues()) {
        auto newVal = val->Eval(*this);
        logger.UnSetFunc("EvalInitVal");
        if (!newVal) {
            logger.Error("eval init val failed");
            return nullptr;
        }
        newInitVals.push_back(std::move(newVal));
    }
    return std::make_unique<InitValAST>(init.getType(), std::move(newInitVals));
}

ASTPtr TypeCheck::EvalAddExp(BinaryExpAST &exp) {
    logger.SetFunc("EvalAddExp");
    auto lval = exp.getLHS()->Eval(*this);
    logger.UnSetFunc("EvalAddExp");
    if (!lval) {
        logger.Error("Parse rhs for eval_mul_exp failed");
        return nullptr;
    }
    auto rval = exp.getRHS()->Eval(*this);
    logger.UnSetFunc("EvalAddExp");
    if (!rval) {
        logger.Error("Parse rhs for eval_mul_exp failed");
        return nullptr;
    }
    if (!dynamic_cast<NumberAST *>(lval.get()) || !dynamic_cast<NumberAST *>(rval.get())) {
        return std::make_unique<BinaryExpAST>(exp.getOp(), std::move(lval), std::move(rval));
    }
    switch (exp.getOp()) {
        case Operator::ADD: {
            return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST *>(lval.get())->getVal()
                    + dynamic_cast<NumberAST *>(rval.get())->getVal());
        }
        case Operator::SUB: {
            return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST *>(lval.get())->getVal()
                    - dynamic_cast<NumberAST *>(rval.get())->getVal());
        }
        default:
            logger.Error("Invalid operator");
            return nullptr;
    }
}

ASTPtr TypeCheck::EvalMulExp(BinaryExpAST &exp) {
    logger.SetFunc("EvalMulExp");
    auto lval = exp.getLHS()->Eval(*this);
    logger.UnSetFunc("EvalMulExp");
    if (!lval) {
        logger.Error("Parse lhs for eval_mul_exp failed");
        return nullptr;
    }
    auto rval = exp.getRHS()->Eval(*this);
    logger.UnSetFunc("EvalMulExp");
    if (!rval) {
        logger.Error("Parse rhs for eval_mul_exp failed");
        return nullptr;
    }
    if (!dynamic_cast<NumberAST *>(lval.get()) || !dynamic_cast<NumberAST *>(rval.get())) {
        return std::make_unique<BinaryExpAST>(exp.getOp(), std::move(lval), std::move(rval));
    }
    switch (exp.getOp()) {
        case Operator::MUL: {
            return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST *>(lval.get())->getVal()
                    * dynamic_cast<NumberAST *>(rval.get())->getVal());
        }
        case Operator::DIV: {
            if (dynamic_cast<NumberAST *>(rval.get())->getVal() == 0) {
                logger.Error("0 for divisor");
                return nullptr;
            }
            return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST *>(lval.get())->getVal()
                    / dynamic_cast<NumberAST *>(rval.get())->getVal());
        }
        case Operator::MOD: {
            if (dynamic_cast<NumberAST *>(rval.get())->getVal() == 0) {
                logger.Error("0 for modulo");
                return nullptr;
            }
            return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST *>(lval.get())->getVal()
                    % dynamic_cast<NumberAST *>(rval.get())->getVal());
        }
        default:
            logger.Error("Invalid operator");
            return nullptr;
    }
}

ASTPtr TypeCheck::EvalUnaryExp(UnaryExpAST &exp) {
    logger.SetFunc("EvalUnaryExp");
    if (dynamic_cast<NumberAST *>(exp.getNode().get())) {
        if (exp.getOp() == Operator::NONE) {
            return std::make_unique<NumberAST>(dynamic_cast<NumberAST *>(exp.getNode().get())->getVal());
        } else {
            switch (exp.getOp()) {
                case Operator::NOT:
                    return std::make_unique<NumberAST>(!dynamic_cast<NumberAST *>(exp.getNode().get())->getVal());
                case Operator::SUB:
                    return std::make_unique<NumberAST>(-dynamic_cast<NumberAST *>(exp.getNode().get())->getVal());
                case Operator::ADD:
                    return std::make_unique<NumberAST>(dynamic_cast<NumberAST *>(exp.getNode().get())->getVal());
                default:
                    logger.Error("Invalid unary exp operator");
                    return nullptr;
            }
        }
    } else {
        auto lval = exp.getNode()->Eval(*this);
        if (!lval) {
            logger.Error("Eval lval failed");
            return nullptr;
        }
        if (dynamic_cast<NumberAST *>(lval.get())) {
            int value = dynamic_cast<NumberAST *>(lval.get())->getVal();
            switch (exp.getOp()) {
                case Operator::NOT:
                    return std::make_unique<NumberAST>(!value);
                case Operator::SUB:
                    return std::make_unique<NumberAST>(-value);
                case Operator::ADD:
                    return std::make_unique<NumberAST>(+value);
                case Operator::NONE:
                    return std::make_unique<NumberAST>(value);
                default:
                    logger.Error("Invalid unary exp operator");
                    return nullptr;
            }
        } else {
            return std::make_unique<UnaryExpAST>(std::move(lval), exp.getOp());
        }
    }
}

std::unique_ptr<FuncDefAST> TypeCheck::EvalFuncDef(FuncDefAST &funcDef) {
    logger.SetFunc("EvalFuncDef");
    currentFunc = funcDef.getName();
    ASTPtrList newArgs;
    std::vector<std::pair<std::string, std::pair<VarType, std::vector<int>/*dims*/>>> args;
    for (const auto &arg: funcDef.getArgs()) {
        //语义要求函数定义中变量维度数一定是常量
        if (dynamic_cast<IdAST *>(arg.get())->getType() == VarType::ARRAY) {
            std::vector<int> dims;
            dims.push_back(0);
            for (const auto &exp: dynamic_cast<IdAST *>(arg.get())->getDim()) {
                auto res = exp->Eval(*this);
                logger.UnSetFunc("EvalFuncDef");
                if (!res || !dynamic_cast<NumberAST *>(res.get())) {
                    logger.Error("Inconstant value for typed array arg dim");
                    return nullptr;
                }
                dims.push_back(dynamic_cast<NumberAST *>(res.get())->getVal());
            }
            newArgs.push_back(
                    std::make_unique<ProcessedIdAST>(dynamic_cast<ProcessedIdAST *>(arg.get())->getName(),
                                                     VarType::ARRAY, false, dims));
            args.emplace_back(dynamic_cast<IdAST *>(arg.get())->getName(), std::make_pair(VarType::ARRAY, dims));
        } else {
            newArgs.push_back(
                    std::make_unique<ProcessedIdAST>(dynamic_cast<ProcessedIdAST *>(arg.get())->getName(), VarType::VAR,
                                                     false));
            args.emplace_back(dynamic_cast<ProcessedIdAST *>(arg.get())->getName(),
                              std::make_pair(VarType::VAR, std::vector<int>{}));
        }
    }
    if (FuncTable.find(currentFunc) != FuncTable.end()) {
        logger.Error("Repeated definition of function");
        return nullptr;
    }
    FuncTable[currentFunc] = std::make_pair(funcDef.getType(), args);
    ASTPtr block = funcDef.getBody()->Eval(*this);
    logger.UnSetFunc("EvalFuncDef");
    if (!block) {
        logger.Error("Eval function body failed");
        return nullptr;
    }
    currentFunc = "";
    return std::make_unique<FuncDefAST>(funcDef.getType(), funcDef.getName(), std::move(newArgs), std::move(block));
}

ASTPtr TypeCheck::EvalRelExp(BinaryExpAST &exp) {
    logger.SetFunc("EvalRelExp");
    auto lhs = exp.getLHS()->Eval(*this);
    logger.UnSetFunc("EvalRelExp");
    if (!lhs) {
        logger.Error("Eval lhs failed for rel exp");
        return nullptr;
    }
    if (exp.getRHS()) {
        auto rhs = exp.getRHS()->Eval(*this);
        logger.UnSetFunc("EvalRelExp");
        if (!rhs) {
            logger.Error("Eval rhs failed for rel exp");
            return nullptr;
        }
        if (!dynamic_cast<NumberAST *>(lhs.get()) || !dynamic_cast<NumberAST *>(rhs.get())) {
            return std::make_unique<BinaryExpAST>(exp.getOp(), std::move(lhs), std::move(rhs));
        }
        switch (exp.getOp()) {
            case Operator::LT:
                return std::make_unique<NumberAST>(dynamic_cast<NumberAST *>(lhs.get())->getVal() <
                                                   dynamic_cast<NumberAST *>(rhs.get())->getVal());
            case Operator::GT:
                return std::make_unique<NumberAST>(dynamic_cast<NumberAST *>(lhs.get())->getVal() >
                                                   dynamic_cast<NumberAST *>(rhs.get())->getVal());
            case Operator::LE:
                return std::make_unique<NumberAST>(dynamic_cast<NumberAST *>(lhs.get())->getVal() <=
                                                   dynamic_cast<NumberAST *>(rhs.get())->getVal());
            case Operator::GE:
                return std::make_unique<NumberAST>(dynamic_cast<NumberAST *>(lhs.get())->getVal() >=
                                                   dynamic_cast<NumberAST *>(rhs.get())->getVal());
            default:
                logger.Error("Invalid operator");
                return nullptr;
        }
    } else {
        return lhs;
    }
}

ASTPtr TypeCheck::EvalLAndExp(BinaryExpAST &exp) {
    logger.SetFunc("EvalLAndExp");
    auto lhs = exp.getLHS()->Eval(*this);
    logger.UnSetFunc("EvalLAndExp");
    if (dynamic_cast<NumberAST *>(lhs.get())->getVal() == 0) {
        return std::make_unique<NumberAST>(0);
    }
    logger.UnSetFunc("EvalLAndExp");
    auto rhs = exp.getRHS()->Eval(*this);
    logger.UnSetFunc("EvalLAndExp");
    if (!rhs) {
        logger.Error("Eval rhs failed for land exp");
        return nullptr;
    }
    if (dynamic_cast<NumberAST *>(lhs.get())->getVal()) {
        return std::make_unique<NumberAST>(1);
    } else {
        return std::make_unique<NumberAST>(0);
    }
}

ASTPtr TypeCheck::EvalLOrExp(BinaryExpAST &exp) {
    logger.SetFunc("EvalLOrExp");
    auto lhs = exp.getLHS()->Eval(*this);
    if (dynamic_cast<NumberAST *>(lhs.get())->getVal()) {
        return std::make_unique<NumberAST>(1);
    }
    logger.UnSetFunc("EvalLOrExp");
    auto rhs = exp.getRHS()->Eval(*this);
    logger.UnSetFunc("EvalLOrExp");
    if (!rhs) {
        logger.Error("Eval rhs failed for land exp");
        return nullptr;
    }
    if (dynamic_cast<NumberAST *>(lhs.get())->getVal()) {
        return std::make_unique<NumberAST>(1);
    } else {
        return std::make_unique<NumberAST>(0);
    }
}

std::unique_ptr<CompUnitAST> TypeCheck::EvalCompUnit(CompUnitAST &unit) {
    logger.SetFunc("EvalCompUnit");
    ASTPtrList newNodes;
    for (const auto &node: unit.getNodes()) {
        auto newNode = node->Eval(*this);
        logger.UnSetFunc("EvalCompUnit");
        if (!newNode) {
            logger.Error("Eval node failed");
            return nullptr;
        }
        newNodes.push_back(std::move(newNode));
    }
    if (FuncTable.find("main") == FuncTable.end()) {
        logger.Error("Main function not found");
        return nullptr;
    }
    return std::make_unique<CompUnitAST>(std::move(newNodes));
}

ASTPtr TypeCheck::EvalEqExp(BinaryExpAST &exp) {
    logger.SetFunc("EvalEqExp");
    auto lhs = exp.getLHS()->Eval(*this);
    logger.UnSetFunc("EvalEqExp");
    if (!lhs) {
        logger.Error("Eval lhs failed for eq exp");
        return nullptr;
    }
    auto rhs = exp.getRHS()->Eval(*this);
    logger.UnSetFunc("EvalEqExp");
    if (!rhs) {
        logger.Error("Eval rhs failed for eq exp");
        return nullptr;
    }
    if (!dynamic_cast<NumberAST *>(lhs.get()) || !dynamic_cast<NumberAST *>(rhs.get())) {
        return std::make_unique<BinaryExpAST>(exp.getOp(), std::move(lhs), std::move(rhs));
    }
    switch (exp.getOp()) {
        case Operator::EQ:
            return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST *>(lhs.get())->getVal() == dynamic_cast<NumberAST *>(rhs.get())->getVal());
        case Operator::NEQ:
            return std::make_unique<NumberAST>(
                    dynamic_cast<NumberAST *>(lhs.get())->getVal() != dynamic_cast<NumberAST *>(rhs.get())->getVal());
        default:
            logger.Error("Invalid operator");
            return nullptr;
    }
}

std::unique_ptr<StmtAST> TypeCheck::EvalStmt(StmtAST &stmt) {
    logger.SetFunc("EvalStmt");
    if (!stmt.getStmt())
        return std::make_unique<StmtAST>(nullptr);
    else {
        auto nStmt = stmt.getStmt()->Eval(*this);
        logger.UnSetFunc("EvalStmt");
        if (!nStmt) {
            logger.Error("Eval stmt failed");
            return nullptr;
        }
        return std::make_unique<StmtAST>(std::move(nStmt));
    }
}