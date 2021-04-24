#include <memory>
#include <iostream>
#include <cstddef>
#include <memory.h>

#include "../define/ast.hpp"
#include "../define/token.hpp"
#include "typeck.hpp"

bool TypeCheck::FillInValue(int *memory, InitValAST &init, std::vector<int> &dim, int i) {
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
        for (auto initval: init.getValues()) {
            idx++;
            if (dynamic_cast<InitValAST *>(initval.get())) {
                if (!FillInValue(memory, *dynamic_cast<InitValAST *>(initval.get()), dim, i + 1))
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

std::unique_ptr <VarDeclAST> TypeCheck::EvalVarDecl(VarDeclAST &varDecl) {
    logger.SetFunc("EvalVarDecl");
    if (!varDecl.isConst()) {
        ASTPtrList list;
        for (auto def: varDecl.getVarDefs()) {
            auto varDef = def->Eval(*this);
            logger.UnSetFunc("EvalVarDecl");
            if (!varDef) {
                logger.Error("Eval var definition failed");
                return nullptr;
            }
            list.push_back(varDef);
        }
        return std::make_unique<VarDeclAST>(std::move(list));
    } else {
        ASTPtrList list;
        for (auto def: varDecl.getVarDefs()) {
            auto nDef = def->Eval(*this);  // 变量常量折叠，不放入语法树
            logger.UnSetFunc("EvalVarDecl");
            if (dynamic_cast<ProcessedIdAST *>(dynamic_cast<VarDefAST *>(nDef.get())->getVar())->getType() ==
                VarType::ARRAY) {
                list.push_back(nDef);
            }
        }
        return std::make_unique<VarDeclAST>(std::move(list));
    }
}

std::unique_ptr <ProcessedIdAST> TypeCheck::EvalId(IdAST &id) {
    std::vector<int> ndim;
    for (auto exp: id.getDim()) {
        if (dynamic_cast<NumberAST *>(exp.get())) {
            ndim.push_back(dynamic_cast<NumberAST *>(exp.get())->getVal())
        } else if (dynamic_cast<BinaryExpAST>(exp)) {
            auto result = dynamic_cast<BinaryExpAST *>(exp.get())->Eval(*this);
            logger.UnSetFunc("EvalId");
            if (!dynamic_cast<NumberAST *>(result.get())) {
                logger.Error("Declare array with variable size");
                return nullptr;
            }
            ndim.push_back(dynamic_cast<NumberAST *>(result.get())->getVal());
        } else if (dynamic_cast<UnaryExpAST *>(exp.get())) {
            auto result = dynamic_cast<UnaryExpAST *>(exp.get())->Eval(this);
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
std::unique_ptr <VarDefAST> TypeCheck::EvalVarDef(VarDefAST &varDef) {
    logger.SetFunc("EvalVarDef");
    if (varDef.isConst()) {
        if (!varDef.getInitVal()) {
            logger.Error("Uninitialized const variable");
            return nullptr;
        }
        auto id = varDef.getVar()->Eval(*this); // ProcessedIdAST
        std::vector<int> ndim = dynamic_cast<ProcessedIdAST *>(id.get())->getDim();
        logger.UnSetFunc("EvalVarDef");
        auto initVal = varDef.getInitVal()->Eval(*this);
        logger.UnSetFunc("EvalVarDef");
        if (!initVal) {
            logger.Error("Eval initVal failed");
            return nullptr;
        }
        if (id.getType() == VarType::ARRAY) {
            int size = 1;
            for (auto x: id.getDim()) {
                size *= x;
            }
            int *arrayVal = (int *) malloc(size * sizeof(int));
            int *tmp = arrayVal;
            if (!FillInValue(tmp, initVal, ndim, 0)) {
                logger.Error("Initialize const variable with inconstant value");
                return nullptr;
            }
            logger.UnSetFunc("EvalVarDef");
            if (currentFunc != "") {
                if (FuncConstArrayTable[currentFunc].find(id.getName()) != FuncConstArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncArrayTable[currentFunc].find(id.getName()) != FuncArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncConstVarTable[currentFunc].find(id.getName()) != FuncConstVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncVarTable[currentFunc].find(id.getName()) != FuncVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                for (int i = 0; i < FuncTable[currentFunc].second.size(); i++) {
                    if (FuncTable[currentFunc].second[i].first == id.getName()) {
                        std::string message = "Repeated definition: " + id.getName();
                        logger.Error(message);
                        return nullptr;
                    }
                }
                FuncConstArrayTable[currentFunc][id.getName()] = ndim;
            } else {
                if (ConstArrayTable.find(id.getName()) != ConstArrayTable.end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (ArrayTable.find(id.getName()) != ArrayTable.end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (ConstVarTable.find(id.getName()) != ConstVarTable.end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (VarTable.find(id.getName()) != VarTable.end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                ConstArrayTable[id.getName()] = ndim;
            }
        } else {
            if (currentFunc != "") {
                if (FuncConstVarTable[currentFunc].find(id.getName()) != FuncConstVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncVarTable[currentFunc].find(id.getName()) != FuncVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncConstArrayTable[currentFunc].find(id.getName()) != FuncConstArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncArrayTable[currentFunc].find(id.getName()) != FuncArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                for (int i = 0; i < FuncTable[currentFunc].second.size(); i++) {
                    if (FuncTable[currentFunc].second[i].first == id.getName()) {
                        std::string message = "Repeated definition: " + id.getName();
                        logger.Error(message);
                        return nullptr;
                    }
                }
                if (!dynamic_cast<NumberAST *>(initVal)) {
                    logger.Error("Initialize constant variable with inconstant value");
                    return nullptr;
                }
                FuncConstVarTable[currentFunc][id.getName()] = dynamic_cast<NumberAST *>(initVal)->getVal();
            } else {
                if (ConstVarTable.find(id.getName()) != ConstVarTable.end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (VarTable.find(id.getName()) != VarTable.end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (ConstArrayTable.find(id.getName()) != ConstArrayTable.end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (ArrayTable.find(id.getName()) != ArrayTable.end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (!dynamic_cast<NumberAST *>(initVal)) {
                    logger.Error("Initialize constant variable with inconstant value");
                    return nullptr;
                }
                ConstVarTable[id.getName()] = dynamic_cast<NumberAST *>(initVal)->getVal();
            }
        }

        return std::make_unique<ValDefAST>(true, id, std::make_unique<ProcessedInitValAST>(
                dynamic_cast<InitValAST *>(initVal.get())->getType(),
                dynamic_cast<InitValAST *>(initVal.get())->getValues(), ndim));
    } else {
        auto id = varDef.getVar()->Eval(*this);
        std::vector<int> ndim = dynamic_cast<ProcessedIdAST *>(id.get())->getDim();
        logger.UnSetFunc("EvalVarDef");
        if (dynamic_cast<ProcessedIdAST *>(id.get())->getType() == VarType::ARRAY) {
            int size = 1;
            for (auto x: id.getDim()) {
                size *= x;
            }
            // 在类型检查步骤不需要为变量定义求值
            if (currentFunc != "") {
                FuncArrayTable[currentFunc][id.getName()] = ndim;
                auto initVal = varDef.getInitVal()->Eval(*this);
                logger.UnSetFunc("EvalVarDef");
                return std::make_unique<ValDefAST>(false, id, std::make_unique<ProcessedInitValAST>(
                        dynamic_cast<InitValAST *>(initVal.get())->getType(),
                        dynamic_cast<InitValAST *>(initVal.get())->getValues(), ndim));
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
                        if (ArrayTable.find(id.getName()) != ArrayTable.end()) {
                            std::string message = "Repeated definition: " + id.getName();
                            logger.Error(message);
                            return nullptr;
                        }
                        if (ConstArrayTable.find(id.getName()) != ConstArrayTable.end()) {
                            std::string message = "Repeated definition: " + id.getName();
                            logger.Error(message);
                            return nullptr;
                        }
                        ArrayTable[id.getName()] = ndim;
                    }
                    return std::make_unique<ValDefAST>(false, id, std::make_unique<ProcessedInitValAST>(
                            dynamic_cast<InitValAST *>(initVal.get())->getType(),
                            dynamic_cast<InitValAST *>(initVal.get())->getValues(), ndim));
                }
                return std::make_unique<VarDefAST>(false, id);
            }
        } else {
            if (currentFunc != "") {
                if (FuncVarTable[currentFunc].find(id.getName()) != FuncVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncConstVarTable[currentFunc].find(id.getName()) != FuncConstVarTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncConstArrayTable[currentFunc].find(id.getName()) != FuncConstArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                if (FuncArrayTable[currentFunc].find(id.getName()) != FuncArrayTable[currentFunc].end()) {
                    std::string message = "Repeated definition: " + id.getName();
                    logger.Error(message);
                    return nullptr;
                }
                for (int i = 0; i < FuncTable[currentFunc].second.size(); i++) {
                    if (FuncTable[currentFunc].second[i].first == id.getName()) {
                        std::string message = "Repeated definition: " + id.getName();
                        logger.Error(message);
                        return nullptr;
                    }
                }
                FuncVarTable[currentFunc].insert(id.getName());
                return std::make_unique<VarDefAST>(id, varDef.getInitVal());
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
                        if (ConstVarTable.find(id.getName()) != ConstVarTable.end()) {
                            std::string message = "Repeated definition: " + id.getName();
                            logger.Error(message);
                            return nullptr;
                        }
                        if (VarTable.find(id.getName()) != VarTable.end()) {
                            std::string message = "Repeated definition: " + id.getName();
                            logger.Error(message);
                            return nullptr;
                        }
                        VarTable[id.getName()] = dynamic_cast<NumberAST *>(initVal.get())->getVal();
                        return std::make_unique<VarDefAST>(id, initVal);
                    }
                } else {
                    if (ConstVarTable.find(id.getName()) != ConstVarTable.end()) {
                        std::string message = "Repeated definition: " + id.getName();
                        logger.Error(message);
                        return nullptr;
                    }
                    if (VarTable.find(id.getName()) != VarTable.end()) {
                        std::string message = "Repeated definition: " + id.getName();
                        logger.Error(message);
                        return nullptr;
                    }
                    VarTable[id.getName()] = 0;
                    return std::make_unique<VarDefAST>(id,
                                                       std::make_unique<ProcessedInitValAST>(VarType::VAR, ASTPtrList{
                                                                                                     std::make_unique<NumberAST>(0)},
                                                                                             std::vector < int > {}));
                }
            }
        }
    }
}

std::unique_ptr <FuncCallAST> TypeCheck::EvalFuncCall(FuncCallAST &func) {
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
        for (int i = 0; i < func.getArgs().size(); i++) {
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
                        if (currentFunc != "") {
                            std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
                            i2 = FuncArrayTable[currentFunc].find(dynamic_cast<LValAST *>(arg.get())->getName());
                            std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
                            i4 = FuncConstArrayTable[currentFunc].find(dynamic_cast<LValAST *>(arg.get())->getName());
                            if (i2 == FuncArrayTable[currentFunc].end()) {
                                if (i4 == FuncConstArrayTable[currentFunc].end()) {
                                    goto out;
                                }
                                if (i4->second.first.size() ==
                                    dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                    logger.Error("Unmatched parameter type: int[] and variable");
                                    return nullptr;
                                };
                                for (int j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                     j < i4->second.first.size(); j++) {
                                    if (i4->second.first[j] != FuncTable[func.getName()].second[i].second[j -
                                                                                                          dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                        logger.Error("Unmatched parameter dim");
                                        return nullptr;
                                    }
                                }
                            } else {
                                if (i2->second.first.size() ==
                                    dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                    logger.Error("Unmatched parameter type: int[] and variable");
                                    return nullptr;
                                };
                                for (int j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                     j < i2->second.first.size(); j++) {
                                    if (i2->second.first[j] != FuncTable[func.getName()].second[i].second[j -
                                                                                                          dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                        logger.Error("Unmatched parameter dim");
                                        return nullptr;
                                    }
                                }
                            }
                        }
                        out:
                        std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
                        i1 = ArrayTable.find(dynamic_cast<LValAST *>(arg.get())->getName());
                        std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
                        i3 = ConstArrayTable.find(dynamic_cast<LValAST *>(arg.get())->getName());
                        if (i1 == FuncArrayTable[currentFunc].end()) {
                            if (i3 == ConstFuncArrayTable[currentFunc].end()) {
                                std::string message =
                                        "Undefined identifier " + dynamic_cast<LValAST *>(arg.get())->getName();
                                logger.Error(message);
                                return nullptr;
                            }
                            if (i3->second.first.size() ==
                                dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                logger.Error("Unmatched parameter type: int[] and variable");
                                return nullptr;
                            };
                            for (int j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                 j < i3->second.first.size(); j++) {
                                if (i3->second.first[j] != FuncTable[func.getName()].second[i].second[j -
                                                                                                      dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                    logger.Error("Unmatched parameter dim");
                                    return nullptr;
                                }
                            }
                        } else {
                            if (i1->second.first.size() ==
                                dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                logger.Error("Unmatched parameter type: int[] and variable");
                                return nullptr;
                            };
                            for (int j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                 j < i1->second.first.size(); j++) {
                                if (i1->second.first[j] != FuncTable[func.getName()].second[i].second[j -
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
                        if (currentFunc != "") {
                            std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
                            i2 = FuncArrayTable[currentFunc].find(dynamic_cast<LValAST *>(arg.get())->getName());
                            std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
                            i4 = FuncConstArrayTable[currentFunc].find(dynamic_cast<LValAST *>(arg.get())->getName());
                            if (i2 == FuncArrayTable[currentFunc].end()) {
                                if (i4 == FuncConstArrayTable[currentFunc].end()) {
                                    goto out;
                                }
                                if (i4->second.first.size() ==
                                    dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                    logger.Error("Unmatched parameter type: int[] and variable");
                                    return nullptr;
                                };
                                for (int j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                     j < i4->second.first.size(); j++) {
                                    if (i4->second.first[j] != FuncTable[func.getName()].second[i].second[j -
                                                                                                          dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                        logger.Error("Unmatched parameter dim");
                                        return nullptr;
                                    }
                                }
                            } else {
                                if (i2->second.first.size() ==
                                    dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                    logger.Error("Unmatched parameter type: int[] and variable");
                                    return nullptr;
                                };
                                for (int j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                     j < i2->second.first.size(); j++) {
                                    if (i2->second.first[j] != FuncTable[func.getName()].second[i].second[j -
                                                                                                          dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                        logger.Error("Unmatched parameter dim");
                                        return nullptr;
                                    }
                                }
                            }
                        }
                        out:
                        std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
                        i1 = ArrayTable.find(dynamic_cast<LValAST *>(arg.get())->getName());
                        std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
                        i3 = ConstArrayTable.find(dynamic_cast<LValAST *>(arg.get())->getName());
                        if (i1 == FuncArrayTable[currentFunc].end()) {
                            if (i3 == ConstFuncArrayTable[currentFunc].end()) {
                                std::string message =
                                        "Undefined identifier " + dynamic_cast<LValAST *>(arg.get())->getName();
                                logger.Error(message);
                                return nullptr;
                            }
                            if (i3->second.first.size() ==
                                dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                logger.Error("Unmatched parameter type: int[] and variable");
                                return nullptr;
                            };
                            for (int j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                 j < i3->second.first.size(); j++) {
                                if (i3->second.first[j] != FuncTable[func.getName()].second[i].second[j -
                                                                                                      dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                    logger.Error("Unmatched parameter dim");
                                    return nullptr;
                                }
                            }
                        } else {
                            if (i1->second.first.size() ==
                                dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                                logger.Error("Unmatched parameter type: int[] and variable");
                                return nullptr;
                            };
                            for (int j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                                 j < i1->second.first.size(); j++) {
                                if (i1->second.first[j] != FuncTable[func.getName()].second[i].second[j -
                                                                                                      dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                    logger.Error("Unmatched parameter dim");
                                    return nullptr;
                                }
                            }
                        }
                    }
                }
            }
            newArgs.push_back(arg);
        }
    }
    return std::make_unique<FuncCallAST>(func.getName(), std::move(newArgs));
}

std::unique_ptr <BlockAST> TypeCheck::EvalBlock(BlockAST &block) {
    logger.SetFunc("EvalBlock");
    ASTPtrList stmts;
    for (auto stmt: block.getStmts()) {
        auto nStmt = stmt->Eval(*this);
        if (!nStmt) {
            logger.Error("Eval stmt failed for block");
            return nullptr;
        }
        logger.UnSetFunc("EvalBlock");
        stmts.push_back(nStmt);

    }
    return std::make_unique<BlockAST>(stmts);
}

std::unique_ptr <IfElseAST> TypeCheck::EvalIfElse(IfElseAST &stmt) {
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
        return std::make_unique<IfElseAST>(cond, then, els);
    }
    return std::make_unique<IfElseAST>(cond, then);
}

std::unique_ptr <WhileAST> TypeCheck::EvalWhile(WhileAST &stmt) {
    logger.SetFunc("EvalWhile");
    auto cond = stmt.getCond()->Eval(this);
    logger.UnSetFunc("EvalWhile");
    if (!cond) {
        logger.Error("Eval cond stmt failed for while");
        return nullptr;
    }
    auto body = stmt.getStmt()->Eval(this);
    logger.UnSetFunc("EvalWhile");
    if (!body) {
        logger.Error("Eval body stmt failed for while");
        return nullptr;
    }
    return std::make_unique<WhileAST>(cond, body);
}

std::unique_ptr <ControlAST> TypeCheck::EvalControl(ControlAST &stmt) {
    logger.SetFunc("EvalControl");
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
                auto exp = stmt.getReturnExp()->Eval(*this);
                logger.UnSetFunc("EvalControl");
                return std::make_unique<ControlAST>(stmt.getControl(), stmt.getReturnExp());
            }
        }
    }
    return stmt;
}

std::unique_ptr <AssignAST> TypeCheck::EvalAssign(AssignAST &assign) {
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
    if (currentFunc != "") {
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
    return std::make_unique<AssignAST>(lhs, rhs);
}

// return number ast or lval ast
ASTPtr TypeCheck::EvalLVal(LValAST &lval) {
    if (lval.getType() == VarType::ARRAY) {
        ASTPtrList pos;
        for (auto exp: lval.getPosition()) {
            auto val = exp->Eval(*this);
            logger.UnSetFunc("EvalUnaryExp");
            if (!val) {
                logger.Error("Eval dim failed");
                return nullptr;
            }
            pos.push_back(val);
        }
        std::string name = lval.getName();
        std::map < std::string, std::pair < std::vector < int >/*dim*/, int * >> ::iterator
        iter;

        // 需要常量值
        if (currentFunc == "") {
            iter = ConstArrayTable.find(name);
            if (iter == ConstArrayTable.end()) {
                iter = ArrayTable.find(name);
                if (iter == ArrayTable.end()) {
                    logger.Error("Undefined identifier");
                    return nullptr;
                } else {
                    return std::make_unique<LValAST>(lval.getName(), lval.getType(), pos);
                }
            } //else below
        } else {
            iter = FuncConstArrayTable[currentFunc].find(name);
            if (iter == FuncConstArrayTable[currentFunc].end()) {
                iter = FuncArrayTable[currentFunc].find(name);
                if (iter == FuncArrayTable[currentFunc].end()) {
                    logger.Error("Undefined identifier: " + name);
                    return nullptr;
                } else {
                    return std::make_unique<LValAST>(lval.getName(), lval.getType(), pos);
                }
            }
        }

        int position = 1;
        if (pos.size() != iter->second.first.size()) {
            logger.Error("Mismatched array position: %s", name);
            return nullptr;
        }
        int volume = 1;
        for (int i = pos.size() - 1; i >= 0; i--) {
            if (!dynamic_cast<NumberAST *>(pos[i].get())->getVal()) {
                return std::make_unique<LValAST>(lval.getName(), lval.getType(), pos);
            }
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
            std::map<std::string, int>::iterator iter = FuncConstVarTable[currentFunc].find(name);
            if (iter == FuncConstVarTable[currentFunc].end()) {
                iter = FuncVarTable[currentFunc].find(name);
                if (iter == FuncVarTable[currentFunc].end()) {
                    logger.Error("Undefined identifier " + name);
                    return nullptr;
                }
                return std::make_unique<LValAST>(lval.getName(), lval.getType();
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

std::unique_ptr <InitValAST> TypeCheck::EvalInitVal(InitValAST &init) {
    logger.SetFunc("EvalInitVal");
    ASTPtrList newInitVals;
    for (auto val: init.getValues()) {
        auto newVal = val->Eval(*this);
        logger.UnSetFunc("EvalInitVal");
        if (!newVal) {
            logger.Error("eval init val failed");
            return nullptr;
        }
        newInitVals.push_back(newVal);
    }
    return std::make_unique<InitValAST>(init.getType(), newInitVals);
}

ASTPtr TypeCheck::EvalAddExp(BinaryExpAST exp) {
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
    if (!dynamic_cast<NumberAST *>(lval) || !dynamic_cast<NumberAST *>(rval)) {
        return std::make_unique<BinaryExpAST>(exp.getOp(), lval, rval)
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
    auto lval = exp.getLHS()->Eval(*this);
    logger.UnSetFunc("EvalMulExp");
    if (!lval) {
        logger.Error("Parse lhs for eval_mul_exp failed");
        return nullptr;
    }
    auto rval = exp.getRHS()->Eval(this);
    logger.UnSetFunc("EvalMulExp");
    if (!rval) {
        logger.Error("Parse rhs for eval_mul_exp failed");
        return nullptr;
    }
    if (!dynamic_cast<NumberAST *>(lval) || !dynamic_cast<NumberAST *>(rval)) {
        return std::make_unique<BinaryExpAST>(exp.getOp(), lval, rval)
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
        auto lval = exp.getNode()->Eval(*this);
        if (!lval) {
            logger.Error("Eval lval failed");
            return nullptr;
        }
        if (dynamic_cast<NumberAST *>(lval.get())) {
            int value = dynamic_cast<NumberAST *>(lval.get())->getVal();
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
            return std::make_unique<UnaryExpAST>(lval, exp.getOp());
        }
    } else if (dynamic_cast<BinaryExpAST>(exp.getNode())) {
        auto val = EvalAddExp(exp.getNode());
        logger.UnSetFunc("EvalUnaryExp");
        if (!val) {
            logger.Error("Eval exp failed");
            return nullptr;
        }
        if (!dynamic_cast<NumberAST *>(val.get())) {
            return std::make_unique<UnaryExpAST>(lval, exp.getOp());
        }
        int value = dynamic_cast<NumberAST *>(val.get())->getVal();
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
        if (!val) {
            logger.Error("Eval unary exp failed");
            return nullptr;
        }
        if (!dynamic_cast<NumberAST *>(val.get())) {
            return std::make_unique<UnaryExpAST>(lval, exp.getOp());
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

std::unique_ptr <FuncDefAST> TypeCheck::EvalFuncDef(FuncDefAST funcDef) {
    logger.SetFunc("EvalFuncDef");
    currentFunc = funcDef.getName();
    ASTPtrList newArgs;
    std::vector < std::pair < std::string, std::pair < VarType, std::vector < int >/*dims*/>>> args;
    for (auto arg: funcDef.getArgs()) {
        //语义要求函数定义中变量维度数一定是常量
        if (dynamic_cast<IdAST *>(arg.get())->getType() == VarType::ARRAY) {
            std::vector<int> dims;
            dims.push_back(0);
            for (auto exp: dynamic_cast<IdAST *>(arg.get())->getDim()) {
                auto res = exp->Eval(*this);
                logger.UnSetFunc("EvalFuncDef");
                if (!res || !dynamic_cast<NumberAST *>(res.get())) {
                    logger.Error("Inconstant value for typed array arg dim");
                    return nullptr;
                }
                dims.push_back(dynamic_cast<NumberAST>(res)->getVal());
            }
            newArgs.push_back(
                    std::make_unique<ProcessedIdAST>(dynamic_cast<ProcessedIdAST *>(arg.get())->getName(),
                                                     VarType::ARRAY, false, dims));
            args.push_back(
                    std::make_pair(dynamic_cast<IdAST *>(arg.get())->getName(), std::make_pair(VarType::ARRAY, dims)));
        } else {
            newArgs.push_back(
                    std::make_unique<ProcessedIdAST>(dynamic_cast<ProcessedIdAST *>(arg)->getName(), VarType::VAR,
                                                     false));
            args.push_back(std::make_pair(dynamic_cast<ProcessedIdAST *>(arg.get())->getName(),
                                          std::make_pair(VarType::VAR, std::vector < int > {})));
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
    return std::make_unique<FuncDefAST>(funcDef.getType(), funcDef.getName(), newArgs, block);
}

ASTPtr TypeCheck::EvalRelExp(BinaryExpAST exp) {
    logger.SetFunc("EvalRelExp");
    auto lval = exp.getLHS()->Eval(*this);
    logger.UnSetFunc("EvalRelExp");
    if (!lval) {
        logger.Error("Eval lhs failed for rel exp");
        return nullptr;
    }
    if (exp.getRHS()) {
        auto rhs = exp.getRHS()->Eval(*this);
        logger.UnSetFunc("EvalRelExp");
        if (!rval) {
            logger.Error("Eval rhs failed for rel exp");
            return nullptr;
        }
        if (!dynamic_cast<NumberAST *>(lval) || !dynamic_cast<NumberAST *>(rval)) {
            return std::make_unique<BinaryExpAST>(exp.getOp(), lval, rval)
        }
        switch (exp.getOp()) {
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
        return lval;
    }
}

ASTPtr TypeCheck::EvalLAndExp(BinaryExpAST exp) {
    logger.SetFunc("EvalLAndExp");
    auto lval = exp.getLHS()->Eval(this);
    logger.UnSetFunc("EvalLAndExp");
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

std::unique_ptr <CompUnitAST> TypeCheck::EvalCompUnit(CompUnitAST unit) {
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
    if (FuncTable.find("main") == FuncTable.end()) {
        logger.Error("Main function not found");
        return nullptr;
    }
    return std::make_unique<CompUnitAST>(newNodes);
}

ASTPtr TypeCheck::EvalEqExp(BinaryExpAST exp) {
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
    if (!dynamic_cast<NumberAST *>(lhs) || !dynamic_cast<NumberAST *>(rhs)) {
        return std::make_unique<BinaryExpAST>(exp.getOp(), lhs, rhs)
    }
    switch (exp.getOp()) {
        case Operator::EQ:
            return dynamic_cast<NumberAST *>(lhs.get())->getVal() == dynamic_cast<NumberAST *>(rhs.get())->getVal();
        case Operator::NEQ:
            return dynamic_cast<NumberAST *>(lhs.get())->getVal() != dynamic_cast<NumberAST *>(rhs.get())->getVal();
        default:
            logger.Error("Invalid operator");
            return nullptr;
    }
}

std::unique_ptr <StmtAST> TypeCheck::EvalStmt(StmtAST &stmt) {
    logger.SetFunc("EvalStmt");
    if (!stmt.getStmt())
        return stmt;
    else {
        auto nStmt = stmt.getStmt()->Eval(*this);
        logger.UnSetFunc("EvalStmt");
        if (!nStmt) {
            logger.Error("Eval stmt failed");
            return nullptr;
        }
        return std::make_unique<StmtAST>(nStmt);
    }
}