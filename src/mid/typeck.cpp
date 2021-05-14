#include <memory>
#include <iostream>
#include <cstddef>
#include <algorithm>

#include "../define/ast.hpp"
#include "typeck.hpp"

namespace SysYToEeyore {
    bool TypeCheck::FillInValue(int *memory, InitValAST *init, std::vector<int> &dim, size_t i) {
        logger.SetFunc("FillInValue");
        int idx = 0;
        int i_idx = 0;
        int elem = 1;
        for (int j = i + 1; j < dim.size(); j++) {
            elem *= dim[j];
        }
        if (init) {
            for (const auto &initval: init->getValues()) {
                if (dynamic_cast<NumberAST *>(initval.get())) {
                    idx++;
                    if (idx == elem) {
                        idx = 0;
                        i_idx++;
                    }
                    *memory = dynamic_cast<NumberAST *>(initval.get())->getVal();
                    memory++;
                } else if (dynamic_cast<InitValAST *>(initval.get())) {
                    if (dynamic_cast<InitValAST *>(initval.get())->getType() == VarType::VAR) {
                        if (!dynamic_cast<NumberAST *>(dynamic_cast<InitValAST *>(initval.get())->getValues()[0].get())) {
                            logger.Error("Initialize const variable with inconstant value");
                            return false;
                        }
                        idx++;
                        if (idx == elem) {
                            idx = 0;
                            i_idx++;
                        }
                        *memory = dynamic_cast<NumberAST *>(dynamic_cast<InitValAST *>(initval.get())->getValues()[0].get())->getVal();
                        memory++;
                    } else {
                        if (idx != 0) {
                            logger.Error("InitVal illegal");
                            return false;
                        }
                        i_idx++;
                        if (i == dim.size() - 1) {
                            logger.Error("InitVal dim larger than array dim");
                            return false;
                        }
                        if (!FillInValue(memory, dynamic_cast<InitValAST *>(initval.get()), dim, i + 1))
                            return false;
                        logger.UnSetFunc("FillInValue");
                    }
                } else {
                    logger.Error("Initialize const variable with inconstant value");
                    return false;
                }
            }
            if (i_idx > dim[i] || (i_idx == dim[i] && idx > 0)) {
                logger.Error("Too many InitVals");
                return false;
            }
            for (; i_idx < dim[i]; i_idx++) {
                if (i == dim.size() - 1) {
                    *memory = 0;
                    memory++;
                } else {
                    if (idx > 0) {
                        logger.Error("InitVal illegal");
                        return false;
                    }
                    if (!FillInValue(memory, nullptr, dim, i + 1))
                        return false;
                    logger.UnSetFunc("FillInValue");
                }
            }
        } else {
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
        logger.Info("Var Name: " + id.getName() + ", Var type: " + std::to_string(id.getType()) + ", var dim size: " +
                    std::to_string(id.getDim().size()));
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
            } else if (dynamic_cast<LValAST *>(exp.get())) {
                auto result = dynamic_cast<LValAST *>(exp.get())->Eval(*this);
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

    std::unique_ptr<VarDefAST> TypeCheck::EvalVarDef(VarDefAST &varDef) {
        logger.SetFunc("EvalVarDef");
        if (varDef.isConst()) {
            if (!varDef.getInitVal()) {
                logger.Error("Uninitialized const variable");
                return nullptr;
            }
            auto id = varDef.getVar()->Eval(*this); // ProcessedIdAST
            logger.UnSetFunc("EvalVarDef");
            std::string name = dynamic_cast<ProcessedIdAST *>(id.get())->getName();
            VarType type = dynamic_cast<ProcessedIdAST *>(id.get())->getType();
            std::vector<int> ndim = dynamic_cast<ProcessedIdAST *>(id.get())->getDim();
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
                if (BlockVars[currentBlock].find(name) != BlockVars[currentBlock].end()) {
                    logger.Error("Repeated definition: " + name);
                    return nullptr;
                }
                // 常量数组不需要折叠
                BlockVars[currentBlock][name] = Var(name, VarType::ARRAY, varDef.isConst(), ndim);
            } else {
                if (BlockVars[currentBlock].find(name) != BlockVars[currentBlock].end()) {
                    logger.Error("Repeated definition: " + name);
                    return nullptr;
                }
                if (!dynamic_cast<NumberAST *>(dynamic_cast<InitValAST *>(initVal.get())->getValues()[0].get())) {
                    logger.Error("Initialize const variable with inconstant value");
                    return nullptr;
                }
                BlockVars[currentBlock][name] = Var(name, VarType::VAR, varDef.isConst(), std::vector<int>{},
                                                    dynamic_cast<NumberAST *>(dynamic_cast<InitValAST *>(initVal.get())->getValues()[0].get())->getVal());
            }

            dynamic_cast<InitValAST *>(initVal.get())->setDim(ndim);
            return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id), std::move(initVal));
        } else {
            auto id = varDef.getVar()->Eval(*this);
            logger.UnSetFunc("EvalVarDef");
            std::string name = dynamic_cast<ProcessedIdAST *>(id.get())->getName();
            VarType type = dynamic_cast<ProcessedIdAST *>(id.get())->getType();
            std::vector<int> ndim = dynamic_cast<ProcessedIdAST *>(id.get())->getDim();
            logger.Info("Var name: " + name + ", var type: " + std::to_string(type) + ", var dim size: " +
                        std::to_string(ndim.size()));
            if (type == VarType::ARRAY) {
                int size = 1;
                for (auto x: ndim) {
                    size *= x;
                }
                if (BlockVars[currentBlock].find(name) != BlockVars[currentBlock].end()) {
                    logger.Error("Repeated definition: " + name);
                    return nullptr;
                }
                BlockVars[currentBlock][name] = Var(name, VarType::ARRAY, varDef.isConst(), ndim);
                if (varDef.getInitVal()) {
                    auto initVal = varDef.getInitVal()->Eval(*this);
                    logger.UnSetFunc("EvalVarDef");
                    if (!initVal) {
                        logger.Error("Invalid initialization of global array");
                        return nullptr;
                    }
                    if (currentBlock == 0) {
                        int *arrayVal = (int *) malloc(size * sizeof(int));
                        int *tmp = arrayVal;
                        if (!FillInValue(tmp, dynamic_cast<InitValAST *>(initVal.get()), ndim, 0)) {
                            logger.Error("Initialize const variable with inconstant value");
                            return nullptr;
                        }
                        logger.UnSetFunc("EvalVarDef");
                    }
                    dynamic_cast<InitValAST *>(initVal.get())->setDim(ndim);
                    return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id), std::move(initVal));
                } else {
                    return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id));
                }
            } else {
                if (BlockVars[currentBlock].find(name) != BlockVars[currentBlock].end()) {
                    logger.Error("Repeated definition: " + name);
                    return nullptr;
                }
                if (varDef.getInitVal()) {
                    auto initVal = varDef.getInitVal()->Eval(*this);
                    logger.UnSetFunc("EvalVarDef");
                    if (!initVal) {
                        logger.Error("Invalid initialization");
                        return nullptr;
                    }
                    if (currentBlock == 0) {
                        if (!dynamic_cast<NumberAST *>(dynamic_cast<InitValAST *>(initVal.get())->getValues()[0].get())) {
                            logger.Error("Mismatched type in variable initialization");
                            return nullptr;
                        }
                        BlockVars[currentBlock][name] = Var(name, VarType::VAR, varDef.isConst(), std::vector<int>{},
                                                            dynamic_cast<NumberAST *>(dynamic_cast<InitValAST *>(initVal.get())->getValues()[0].get())->getVal());
                    } else {
                        BlockVars[currentBlock][name] = Var(name, VarType::VAR, varDef.isConst(), std::vector<int>{});
                    }
                    return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id), std::move(initVal));
                } else {
                    if (currentBlock == 0) {
                        BlockVars[currentBlock][name] = Var(name, VarType::VAR, varDef.isConst(), std::vector<int>{},
                                                            0);
                        ASTPtrList retlist;
                        retlist.push_back(std::make_unique<NumberAST>(0));
                        return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id),
                                                           std::make_unique<InitValAST>(VarType::VAR,
                                                                                        std::move(retlist)));
                    } else {
                        BlockVars[currentBlock][name] = Var(name, VarType::VAR, varDef.isConst(), std::vector<int>{});
                        return std::make_unique<VarDefAST>(varDef.isConst(), std::move(id));
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
            if (func.getArgs().size() != FuncTable[func.getName()].argTable.size()) {
                logger.Error("Mismatched argument number");
                return nullptr;
            }
            ASTPtrList newArgs;
            for (size_t i = 0; i < func.getArgs().size(); i++) {
                // 检查参数合法性
                auto arg = func.getArgs()[i]->Eval(*this);
                logger.UnSetFunc("EvalFuncCall");
                if (!arg) {
                    logger.Error("Eval arg failed");
                    return nullptr;
                }
                if (FuncTable[func.getName()].argTable[i].type == VarType::ARRAY) {
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
                        int tmpCurrentBlock = currentBlock;
                        std::map<std::string, Var>::iterator iter;
                        while (tmpCurrentBlock != -1) {
                            iter = BlockVars[tmpCurrentBlock].find(dynamic_cast<LValAST *>(arg.get())->getName());
                            if (iter != BlockVars[tmpCurrentBlock].end()) {
                                break;
                            }
                            tmpCurrentBlock = parentBlock[tmpCurrentBlock];
                        }
                        if (tmpCurrentBlock == -1) {
                            logger.Error("Undefined identifier " + dynamic_cast<LValAST *>(arg.get())->getName());
                            return nullptr;
                        }
                        if (iter->second.type == VarType::VAR) {
                            logger.Error("Unmatched parameter type: int[] and variable 2");
                            return nullptr;
                        }
                        if (iter->second.dims.size() == dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                            logger.Error("Unmatched parameter type: int[] and variable 3");
                            return nullptr;
                        }
                        for (size_t j = dynamic_cast<LValAST *>(arg.get())->getPosition().size() + 1;
                             j < iter->second.dims.size(); j++) {
                            // std::cout << j << " " << iter->second.dims[j] << " " << FuncTable[func.getName()].argTable[i].dims[j - dynamic_cast<LValAST *>(arg.get())->getPosition().size()] << "\n";
                            if (iter->second.dims[j] != FuncTable[func.getName()].argTable[i].dims[j -
                                                                                                   dynamic_cast<LValAST *>(arg.get())->getPosition().size()]) {
                                logger.Error("Unmatched parameter dim");
                                return nullptr;
                            }
                        }
                    }
                } else {
                    if (dynamic_cast<LValAST *>(arg.get())) {
                        int tmpCurrentBlock = currentBlock;
                        std::map<std::string, Var>::iterator iter;
                        while (tmpCurrentBlock != -1) {
                            iter = BlockVars[tmpCurrentBlock].find(dynamic_cast<LValAST *>(arg.get())->getName());
                            if (iter != BlockVars[tmpCurrentBlock].end()) {
                                break;
                            }
                            tmpCurrentBlock = parentBlock[tmpCurrentBlock];
                        }
                        if (tmpCurrentBlock == -1) {
                            logger.Error("Undefined identifier " + dynamic_cast<LValAST *>(arg.get())->getName());
                            return nullptr;
                        }
                        if (iter->second.dims.size() != dynamic_cast<LValAST *>(arg.get())->getPosition().size()) {
                            logger.Error("Unmatched parameter type: int and int[]");
                            return nullptr;
                        }
                        // 不检查数组越界
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
        parentBlock.push_back(currentBlock);
        currentBlock = parentBlock.size() - 1;
        for (const auto &stmt: block.getStmts()) {
            auto nStmt = stmt->Eval(*this);
            if (!nStmt) {
                logger.Error("Eval stmt failed for block");
                return nullptr;
            }
            logger.UnSetFunc("EvalBlock");
            stmts.push_back(std::move(nStmt));

        }
        currentBlock = parentBlock[currentBlock];
        return std::make_unique<BlockAST>(std::move(stmts));
    }

    std::unique_ptr<IfElseAST> TypeCheck::EvalIfElse(IfElseAST &stmt) {
        logger.SetFunc("EvalIfElse");
        // std::cout << "COND: " << dynamic_cast<BinaryExpAST*>(stmt.getCond().get())->getOp() << "\n";
        auto cond = stmt.getCond()->Eval(*this);
        logger.UnSetFunc("EvalIfElse");
        if (!cond) {
            logger.Error("Eval cond stmt failed for if else");
            return nullptr;
        }
        // std::cout << "COND: " << dynamic_cast<BinaryExpAST*>(cond.get())->getOp() << "\n";
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
            if (FuncTable[currentFunc].funcType == Type::VOID) {
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
        int tmpCurrentBlock = currentBlock;
        std::map<std::string, Var>::iterator iter;
        while (tmpCurrentBlock != -1) {
            iter = BlockVars[tmpCurrentBlock].find(dynamic_cast<LValAST *>(lhs.get())->getName());
            if (iter != BlockVars[tmpCurrentBlock].end()) {
                break;
            }
            tmpCurrentBlock = parentBlock[tmpCurrentBlock];
        }
        if (tmpCurrentBlock == -1) {
            logger.Error("Undefined identifier " + dynamic_cast<LValAST *>(lhs.get())->getName());
            return nullptr;
        }
        if (iter->second.type == VarType::ARRAY &&
            iter->second.dims.size() != dynamic_cast<LValAST *>(lhs.get())->getPosition().size()) {
            logger.Error("Mismatched type, array to assign value");
            return nullptr;
        }
        if (iter->second.isConst) {
            logger.Error("Cannot change the value of a constant array");
            return nullptr;
        }
        auto rhs = assign.getRight()->Eval(*this);
        logger.UnSetFunc("EvalAssign");
        if (!rhs) {
            logger.Error("Eval rhs failed for assignment");
            return nullptr;
        }
        if (dynamic_cast<LValAST *>(rhs.get())) {
            int tmpCurrentBlock = currentBlock;
            std::map<std::string, Var>::iterator iter;
            while (tmpCurrentBlock != -1) {
                iter = BlockVars[tmpCurrentBlock].find(dynamic_cast<LValAST *>(rhs.get())->getName());
                if (iter != BlockVars[tmpCurrentBlock].end()) {
                    break;
                }
                tmpCurrentBlock = parentBlock[tmpCurrentBlock];
            }
            if (tmpCurrentBlock == -1) {
                logger.Error("Undefined identifier " + dynamic_cast<LValAST *>(rhs.get())->getName());
                return nullptr;
            }
            if (iter->second.type == VarType::ARRAY &&
                iter->second.dims.size() != dynamic_cast<LValAST *>(rhs.get())->getPosition().size()) {
                logger.Error("Mismatched type, array as right value");
                return nullptr;
            }
        }
        return std::make_unique<AssignAST>(std::move(lhs), std::move(rhs));
    }

// return number ast or lval ast
    ASTPtr TypeCheck::EvalLVal(LValAST &lval) {
        logger.SetFunc("EvalLVal");
        if (lval.getType() == VarType::ARRAY) {
            ASTPtrList pos;
            for (const auto &exp: lval.getPosition()) {
                auto val = exp->Eval(*this);
                logger.UnSetFunc("EvalLVal");
                if (!val) {
                    logger.Error("Eval dim failed");
                    return nullptr;
                }
                pos.push_back(std::move(val));
            }
            const std::string &name = lval.getName();
            int tmpCurrentBlock = currentBlock;
            // std::cout << tmpCurrentBlock << std::endl;
            std::map<std::string, Var>::iterator iter;
            while (tmpCurrentBlock != -1) {
                iter = BlockVars[tmpCurrentBlock].find(name);
                if (iter != BlockVars[tmpCurrentBlock].end()) {
                    break;
                }
                tmpCurrentBlock = parentBlock[tmpCurrentBlock];
            }
            if (tmpCurrentBlock == -1) {
                logger.Error("Undefined identifier " + name);
                return nullptr;
            }
            // std::cout << iter->second.name << " " << iter->second.type << std::endl;
            return std::make_unique<LValAST>(lval.getName(), lval.getType(), std::move(pos));

        } else {
            // var
            const std::string &name = lval.getName();
            int tmpCurrentBlock = currentBlock;
            std::map<std::string, Var>::iterator iter;
            while (tmpCurrentBlock != -1) {
                iter = BlockVars[tmpCurrentBlock].find(name);
                if (iter != BlockVars[tmpCurrentBlock].end()) {
                    break;
                }
                tmpCurrentBlock = parentBlock[tmpCurrentBlock];
            }
            if (tmpCurrentBlock == -1) {
                logger.Error("Undefined identifier " + name);
                return nullptr;
            }
            if (iter->second.isConst) {
                return std::make_unique<NumberAST>(iter->second.val);
            } else {
                return std::make_unique<LValAST>(lval.getName(), lval.getType());
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
            logger.Error("Eval lhs for eval_add_exp failed");
            return nullptr;
        }
        auto rval = exp.getRHS()->Eval(*this);
        logger.UnSetFunc("EvalAddExp");
        if (!rval) {
            logger.Error("Eval rhs for eval_add_exp failed");
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

    ASTPtr TypeCheck::EvalRelExp(BinaryExpAST &exp) {
        logger.SetFunc("EvalRelExp");
        auto lhs = exp.getLHS()->Eval(*this);
        logger.UnSetFunc("EvalRelExp");
        if (!lhs) {
            logger.Error("Eval lhs failed for rel exp");
            return nullptr;
        }
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
        /*
        if (exp.getRHS()) {

        } else {
            return lhs;
        }
         */
    }

    ASTPtr TypeCheck::EvalLAndExp(BinaryExpAST &exp) {
        logger.SetFunc("EvalLAndExp");
        auto lhs = exp.getLHS()->Eval(*this);
        logger.UnSetFunc("EvalLAndExp");
        if (dynamic_cast<NumberAST *>(lhs.get()) && dynamic_cast<NumberAST *>(lhs.get())->getVal() == 0) {
            return std::make_unique<NumberAST>(0);
        }
        auto rhs = exp.getRHS()->Eval(*this);
        logger.UnSetFunc("EvalLAndExp");
        if (!rhs) {
            logger.Error("Eval rhs failed for land exp");
            return nullptr;
        }
        if (dynamic_cast<NumberAST *>(rhs.get()) && dynamic_cast<NumberAST *>(lhs.get())->getVal()) {
            return std::make_unique<NumberAST>(1);
        } else if (dynamic_cast<NumberAST *>(rhs.get())) {
            return std::make_unique<NumberAST>(0);
        } else {
            return std::make_unique<BinaryExpAST>(Operator::AND, std::move(lhs), std::move(rhs));
        }
    }

    ASTPtr TypeCheck::EvalLOrExp(BinaryExpAST &exp) {
        logger.SetFunc("EvalLOrExp");
        auto lhs = exp.getLHS()->Eval(*this);
        logger.UnSetFunc("EvalLOrExp");
        if (dynamic_cast<NumberAST *>(lhs.get()) && dynamic_cast<NumberAST *>(lhs.get())->getVal()) {
            return std::make_unique<NumberAST>(1);
        }
        auto rhs = exp.getRHS()->Eval(*this);
        logger.UnSetFunc("EvalLOrExp");
        if (!rhs) {
            logger.Error("Eval rhs failed for land exp");
            return nullptr;
        }
        if (dynamic_cast<NumberAST *>(rhs.get()) && dynamic_cast<NumberAST *>(rhs.get())->getVal()) {
            return std::make_unique<NumberAST>(1);
        } else if (dynamic_cast<NumberAST *>(rhs.get())) {
            return std::make_unique<NumberAST>(0);
        } else {
            return std::make_unique<BinaryExpAST>(Operator::OR, std::move(lhs), std::move(rhs));
        }
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
                        dynamic_cast<NumberAST *>(lhs.get())->getVal() ==
                        dynamic_cast<NumberAST *>(rhs.get())->getVal());
            case Operator::NEQ:
                return std::make_unique<NumberAST>(
                        dynamic_cast<NumberAST *>(lhs.get())->getVal() !=
                        dynamic_cast<NumberAST *>(rhs.get())->getVal());
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
            logger.UnSetFunc("EvalUnaryExp");
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
        std::vector<Var> args;
        for (const auto &arg: funcDef.getArgs()) {
            //语义要求函数定义中变量维度数一定是常量
            if (dynamic_cast<IdAST *>(arg.get())->getType() == VarType::ARRAY) {
                std::vector<int> dims;
                for (const auto &exp: dynamic_cast<IdAST *>(arg.get())->getDim()) {
                    auto res = exp->Eval(*this);
                    // std::cout << dynamic_cast<NumberAST *>(res.get())->getVal() << std::endl;
                    logger.UnSetFunc("EvalFuncDef");
                    if (!res || !dynamic_cast<NumberAST *>(res.get())) {
                        logger.Error("Inconstant value for typed array arg dim");
                        return nullptr;
                    }
                    dims.push_back(dynamic_cast<NumberAST *>(res.get())->getVal());
                }
                newArgs.push_back(
                        std::make_unique<ProcessedIdAST>(dynamic_cast<IdAST *>(arg.get())->getName(),
                                                         VarType::ARRAY, false, dims));
                args.emplace_back(dynamic_cast<IdAST *>(arg.get())->getName(), VarType::ARRAY, false, dims);
            } else {
                newArgs.push_back(
                        std::make_unique<ProcessedIdAST>(dynamic_cast<IdAST *>(arg.get())->getName(), VarType::VAR,
                                                         false));
                args.emplace_back(dynamic_cast<IdAST *>(arg.get())->getName(), VarType::VAR, false);
            }
            BlockVars[parentBlock.size()][dynamic_cast<IdAST *>(arg.get())->getName()] = args[args.size() - 1];
        }
        if (FuncTable.find(funcDef.getName()) != FuncTable.end()) {
            logger.Error("Repeated definition of function");
            return nullptr;
        }
        FuncTable[funcDef.getName()] = Function(funcDef.getName(), funcDef.getType(), args);
        ASTPtr block = funcDef.getBody()->Eval(*this);
        logger.UnSetFunc("EvalFuncDef");
        if (!block) {
            logger.Error("Eval function body failed");
            return nullptr;
        }
        currentFunc = "";
        return std::make_unique<FuncDefAST>(funcDef.getType(), funcDef.getName(), std::move(newArgs), std::move(block));
    }

    std::unique_ptr<CompUnitAST> TypeCheck::EvalCompUnit(CompUnitAST &unit) {
        logger.SetFunc("EvalCompUnit");
        ASTPtrList newNodes;
        parentBlock.push_back(-1);
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

    std::unique_ptr<NumberAST> TypeCheck::EvalNumber(NumberAST &num) {
        logger.SetFunc("EvalNumber");
        return std::make_unique<NumberAST>(num.getVal());
    }
}
