#include "../define/ast.hpp"
#include "genIR.hpp"
#include <functional>
#include <vector>

void IRGenerator::GenerateValue(int &idx, InitValAST *init, std::vector<int> dim, int i, std::string &code) {
    logger.SetFunc("GenerateValue");
    if (init == nullptr) {
        if (i == dim.size() - 1) {
            for (int j = 0; j < dim[i]; j++) {
                code += (tab + "T" + std::to_string(T_num - 1) + "[" + std::to_string(idx++) + "] = 0\n");
            }
        } else {
            for (int j = 0; j < dim[i]; j++) {
                GenerateValue(idx, nullptr, dim, i + 1, code);
                logger.UnSetFunc("GenerateValue");
            }
        }
    } else{
        for (const auto &initval: init->getValues()) {
            if (dynamic_cast<InitValAST*>(initval.get())) {
                GenerateValue(idx, dynamic_cast<InitValAST*>(initval.get()), dim, i+1, code);
                logger.UnSetFunc("GenerateValue");
            } else {
                if (dynamic_cast<NumberAST*>(initval.get())) {
                    code += (tab + "T" + std::to_string(T_num - 1) + "[" + std::to_string(idx++) + "] = " + std::to_string(dynamic_cast<NumberAST*>(initval.get())->getVal()) + "\n");
                } else {
                    std::string res = initval->GenerateIR(*this, code);
                    code += (tab + "T" + std::to_string(T_num - 1) + "[" + std::to_string(idx++) + "] = " + res + "\n");
                }
            }
        }
        for (int j = init->getValues().size(); j < dim[i]; j++) {
            GenerateValue(idx, nullptr, dim, i + 1, code);
            logger.UnSetFunc("GenerateValue");
        }
    }
}

std::string IRGenerator::op2char(Operator op) {
    std::string c;
    switch (op) {
        case Operator::ADD: c = "+"; break;
        case Operator::SUB: c = "-"; break;
        case Operator::DIV: c = "/"; break;
        case Operator::MOD: c = "%"; break;
        case Operator::MUL: c = "*"; break;
        case Operator::AND: c = "&&"; break;
        case Operator::EQ: c = "=="; break;
        case Operator::GE: c = ">="; break;
        case Operator::NEQ: c = "!="; break;
        case Operator::LE: c = "<="; break;
        case Operator::LT: c = "<"; break;
        case Operator::GT: c = ">"; break;
        case Operator::OR: c = "||"; break;
        case Operator::NONE:  c = ""; break;
        case Operator::NOT: c = "!"; break;
    }
    return c;
}

std::string IRGenerator::GenBinaryExp(BinaryExpAST &exp, std::string &code) {
    logger.SetFunc("GenBinaryExp");
    std::string t1 = exp.getLHS()->GenerateIR(*this, code);
    logger.UnSetFunc("GenBinaryExp");
    std::string t2 = exp.getRHS()->GenerateIR(*this, code);
    logger.UnSetFunc("GenBinaryExp");
    std::string res = "t" + std::to_string(t_num++);
    code += (tab + res + " = " + t1 + " " + op2char(exp.getOp()) + " " + t2 + "\n");
    return res;
}

std::string IRGenerator::GenNumber(NumberAST &num, std::string &code) {
    logger.SetFunc("GenNumber");
    return std::to_string(num.getVal());
}

std::string IRGenerator::GenVarDef(VarDefAST &varDef, std::string &code) {
    logger.SetFunc("GenVarDef");
    varDef.getVar()->GenerateIR(*this, code);
    logger.UnSetFunc("GenVarDef");
    if (varDef.getInitVal()) {
        varDef.getInitVal()->GenerateIR(*this, code);
        logger.UnSetFunc("GenVarDef");
    } else {
        int idx = 0;
        GenerateValue(idx, nullptr, dynamic_cast<ProcessedIdAST*>(varDef.getVar().get())->getDim(), 0, code);
        logger.UnSetFunc("GenVarDef");
    }
}

// ID：全是变量，没有函数，和左值也是分开的
std::string IRGenerator::GenId(ProcessedIdAST &id, std::string &code) {
    logger.SetFunc("GenId");
    if (currentFunc.empty()) {
        if (SymbolTable.find(id.getName()) == SymbolTable.end()) {
            SymbolTable[id.getName()] = T_num++;
            ReverseSymbolTable.emplace_back(id.getName(), id.getType());
        }
        if (id.getType() == VarType::ARRAY) {
            ArrayTable[id.getName()] = id.getDim();
        }
        return "T" + std::to_string(SymbolTable[id.getName()]);
    } else {
        if (FuncArgTable[currentFunc].find(id.getName()) != FuncArgTable[currentFunc].end()) {
            return "p" + std::to_string(FuncArgTable[currentFunc][id.getName()]);
        }
        if (FuncVarTable[currentFunc].find(id.getName()) != FuncVarTable[currentFunc].end()) {
            return "T" + std::to_string(FuncVarTable[currentFunc][id.getName()]);
        }
        if (SymbolTable.find(id.getName()) != SymbolTable.end()) {
            return "T" + std::to_string(SymbolTable[id.getName()]);
        }
        FuncVarTable[currentFunc][id.getName()] = T_num++;
        if (id.getType() == VarType::ARRAY) {
            FuncArrayTable[currentFunc][id.getName()] = id.getDim();
        }
        return "T" + std::to_string(FuncVarTable[currentFunc][id.getName()]);
    }
}

std::string IRGenerator::GenInitVal(InitValAST &init, std::string &code) {
    logger.SetFunc("GenInitVal");
    if (init.getType() == VarType::VAR) {
        std::string res = init.getValues()[0]->GenerateIR(*this, code);
        logger.SetFunc("GenInitVal");
        code += (tab + "T" + std::to_string(T_num) + " = " + res + "\n");
    } else {
        int idx = 0;
        GenerateValue(idx, &init, init.getDims(), 0, code);
        logger.SetFunc("GenInitVal");
    }
    return "T" + std::to_string(T_num);
}

std::string IRGenerator::GenAssign(AssignAST &assign, std::string &code) {
    logger.SetFunc("GenAssign");
    std::string l = assign.getLeft()->GenerateIR(*this, code);
    logger.UnSetFunc("GenAssign");
    std::string r = assign.getRight()->GenerateIR(*this, code);
    logger.UnSetFunc("GenAssign");
    code += (tab + l + " = " + r + "\n");
    return l;
}

void IRGenerator::GenVarDecl(VarDeclAST &varDecl, std::string &code) {
    logger.SetFunc("GenVarDecl");
    for (const auto &varDef: varDecl.getVarDefs()) {
        varDef->GenerateIR(*this, code);
        logger.UnSetFunc("GenVarDecl");
    }
}

void IRGenerator::GenCompUnit(CompUnitAST &unit, std::string &code) {
    logger.SetFunc("GenCompUnit");
    std::string str;
    int tmp = T_num;
    for (const auto &node: unit.getNodes()) {
        if (dynamic_cast<VarDeclAST*>(node.get())) {
            node->GenerateIR(*this, str);
            logger.UnSetFunc("GenCompUnit");
        }
    }
    for (int i = tmp; i < T_num; i++) {
        if (ArrayTable.find(ReverseSymbolTable[i].first) != ArrayTable.end()) {
            std::vector<int> dim = ArrayTable[ReverseSymbolTable[i].first];
            int size = 4;
            for (int d : dim) {
                size *= d;
            }
            code += (tab + "var " + std::to_string(size) + " T" + std::to_string(i) + "\n");
        } else {
            code += (tab + "var T" + std::to_string(i) + "\n");
        }
    }
    code += str;
    for (const auto &node: unit.getNodes()) {
        if (!dynamic_cast<VarDeclAST*>(node.get())) {
            node->GenerateIR(*this, code);
            logger.UnSetFunc("GenCompUnit");
        }
    }
}

void IRGenerator::GenFuncCall(FuncCallAST &func, std::string &code) {
    logger.SetFunc("GenFuncCall");
    std::vector<std::string> args;
    for (const auto &arg: func.getArgs()) {
        std::string res = arg->GenerateIR(*this, code);
        logger.UnSetFunc("GenFuncCall");
        args.push_back(res);
    }
    for (const auto &res: args) {
        code += ( tab + "param " + res + "\n");
    }
    code += ( tab + "call f_" + func.getName() + "\n");
}

std::string IRGenerator::GenUnaryExp(UnaryExpAST &exp, std::string &code) {
    logger.SetFunc("GenUnaryExp");
    std::string res;
    res += op2char(exp.getOp());
    res += exp.getNode()->GenerateIR(*this, code);
    logger.UnSetFunc("GenUnaryExp");
    return res;
}

std::string IRGenerator::GenLVal(LValAST &lval, std::string &code) {
    logger.SetFunc("GenLVal");
    if (lval.getType() == VarType::VAR) {
        if (currentFunc.empty()) {
            return "T" + std::to_string(SymbolTable[lval.getName()]);
        } else {
            return "T" + std::to_string(FuncVarTable[currentFunc][lval.getName()]);
        }
    } else {
        std::vector<int> dim;
        if (currentFunc.empty()) {
            dim = ArrayTable[lval.getName()];
        } else {
            dim = FuncArrayTable[currentFunc][lval.getName()];
        }
        int tmp;
        for (size_t i = 0; i < lval.getPosition().size(); i++) {
            std::string var = lval.getPosition()[i]->GenerateIR(*this, code);
            logger.UnSetFunc("GenLVal");
            if (i < lval.getPosition().size() - 1) {
                code += (tab + "t" + std::to_string(t_num) + " = " + var + " * " + std::to_string(dim[i+1]) + "\n");
            }
            code += (tab + "t" + std::to_string(t_num) + "= t" + std::to_string(tmp) + " + t" + std::to_string(t_num) + "\n");
            tmp = t_num++;
        }
        code += (tab + "t" + std::to_string(tmp) + " = 4 * t" + std::to_string(tmp) + "\n");
        std::string name;
        if (currentFunc.empty()) {
            name = "T" + std::to_string(SymbolTable[lval.getName()]);
        } else {
            name = "T" + std::to_string(FuncVarTable[currentFunc][lval.getName()]);
        }
        std::string res = name + "[t" + std::to_string(tmp) + "]";
        return res;
    }
}

void IRGenerator::GenFuncDef(FuncDefAST &funcDef, std::string &code) {
    logger.SetFunc("GenFuncDef");
    currentFunc = funcDef.getName();
    for (size_t i = 0; i < funcDef.getArgs().size(); i++) {
        FuncArgTable[currentFunc][dynamic_cast<ProcessedIdAST*>(funcDef.getArgs()[i].get())->getName()] = i;
    }
    code += ("f_" + funcDef.getName() + "[" + std::to_string(funcDef.getArgs().size()) + "]\n");
    int T_tmp = T_num;
    int t_tmp = t_num;
    std::string code2;
    tab += "\t";
    funcDef.getBody()->GenerateIR(*this, code2);
    logger.UnSetFunc("GenFuncDef");
    for (int i = T_tmp; i < T_num; i++) {
        if (ReverseSymbolTable[i].second == VarType::VAR) {
            code += (tab + "var T" + std::to_string(i) + "\n");
        } else {
            std::vector<int> dim = FuncArrayTable[funcDef.getName()][ReverseSymbolTable[i].first];
            int size = 4;
            for (int d: dim) {
                size *= d;
            }
            code += (tab + "var " + std::to_string(size) + " " + ReverseSymbolTable[i].first + "\n");
        }
    }
    for (int i = t_tmp; i < t_num; i++) {
        code += (tab + "var t" + std::to_string(i));
    }
    code += code2;
    code += (tab + "return" + (funcDef.getType() == Type::INT? " 0 ":" ") + "\nend f_" + funcDef.getName() + "\n");
    tab = "";
}

void IRGenerator::GenStmt(StmtAST &stmt, std::string &code) {
    logger.SetFunc("GenStmt");
    if (stmt.getStmt()) {
        stmt.getStmt()->GenerateIR(*this, code);
        logger.UnSetFunc("GenStmt");
    }
}

void IRGenerator::GenBlock(BlockAST &block, std::string &code) {
    logger.SetFunc("GenBlock");
    for (const auto & stmt : block.getStmts()) {
        stmt->GenerateIR(*this, code);
        logger.UnSetFunc("GenBlock");
    }
}

void IRGenerator::GenIfElse(IfElseAST &stmt ,std::string &code) {
    logger.SetFunc("GenIfElse");
    std::string cond = stmt.getCond()->GenerateIR(*this, code);
    logger.UnSetFunc("GenIfElse");
    code += (tab + "if " + cond + " == 0 goto l" + std::to_string(l_num++) + "\n");
    stmt.getThenStmt()->GenerateIR(*this, code);
    logger.UnSetFunc("GenIfElse");
    code += ("l" + std::to_string(--l_num) + ":\n");
    stmt.getElseStmt()->GenerateIR(*this, code);
    logger.UnSetFunc("GenIfElse");
}

void IRGenerator::GenWhile(WhileAST &stmt, std::string &code) {
    logger.SetFunc("GenWhile");
    code += ("l" + std::to_string(l_num++) + "\n");
    std::string cond = stmt.getCond()->GenerateIR(*this, code);
    logger.UnSetFunc("GenWhile");
    code += (tab + "if " + cond + " == 0 goto l" + std::to_string(l_num++) + "\n");
    stmt.getStmt()->GenerateIR(*this, code);
    logger.UnSetFunc("GenWhile");
    code += ("l" + std::to_string(--l_num) + ":\n");
}

void IRGenerator::GenControl(ControlAST &stmt, std::string &code) {
    logger.SetFunc("GenControl");
    switch(stmt.getControl()) {
        case Token::CONTINUE: {
            code += (tab + "goto l" + std::to_string(l_num - 2) + "\n");
            break;
        }
        case Token::BREAK: {
            code += (tab + "goto l" + std::to_string(l_num - 1) + "\n");
            break;
        }
        case Token::RETURN: {
            if (stmt.getReturnExp()) {
                std::string ret = stmt.getReturnExp()->GenerateIR(*this, code);
                logger.UnSetFunc("GenControl");
                code += (tab + "return " + ret + "\n");
            }
            else code += (tab + "return\n");
        }
        default:
            break;
    }
}
