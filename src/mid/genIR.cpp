#include "../define/ast.hpp"
#include "genIR.hpp"
#include <functional>
#include <vector>

void IRGenerator::GenerateValue(int idx, InitValAST *init, std::vector<int> dim, int i, std::string &code) {
    if (!init) {
        if (i == dim.size() - 1) {
            for (int j = 0; j < dim[i]; j++) {
                code += (tab + "T" + std::to_string(T_num - 1) + "[" + std::to_string(idx++) + "] = 0\n");
            }
        } else {
            GenerateValue(idx, nullptr, dim, i + 1, code);
        }
    } else{
        for (const auto &initval: init->getValues()) {
            if (dynamic_cast<InitValAST*>(initval.get())) {
                GenerateValue(idx, dynamic_cast<InitValAST*>(initval.get()), dim, i+1, code);
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
    std::string t1 = exp.getLHS()->GenerateIR(*this, code);
    std::string t2 = exp.getRHS()->GenerateIR(*this, code);
    std::string res = "t" + std::to_string(t_num++);
    code += (tab + res + " = " + t1 + " " + op2char(exp.getOp()) + " " + t2 + "\n");
    return res;
}

std::string IRGenerator::GenNumber(NumberAST &num, std::string &code) {
    return std::to_string(num.getVal());
}

std::string IRGenerator::GenVarDef(VarDefAST &varDef, std::string &code) {
    if (varDef.getInitVal()) {
        varDef.getInitVal()->GenerateIR(*this, code);
    }
    return "T" + std::to_string(T_num++);
}

// ID：全是变量，没有函数，和左值也是分开的
std::string IRGenerator::GenId(ProcessedIdAST &id, std::string &code) {
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
    if (init.getType() == VarType::VAR) {
        std::string res = init.getValues()[0]->GenerateIR(*this, code);
        code += (tab + "T" + std::to_string(T_num) + " = " + res + "\n");
    } else {
        GenerateValue(0, &init, init.getDims(), 0, code);
    }
    return "T" + std::to_string(T_num);
}

std::string IRGenerator::GenAssign(AssignAST &assign, std::string &code) {
    std::string l = assign.getLeft()->GenerateIR(*this, code);
    std::string r = assign.getRight()->GenerateIR(*this, code);
    code += (tab + l + " = " + r + "\n");
    return l;
}

void IRGenerator::GenVarDecl(VarDeclAST &varDecl, std::string &code) {
    for (const auto &varDef: varDecl.getVarDefs()) {
        varDef->GenerateIR(*this, code);
    }
}

void IRGenerator::GenCompUnit(CompUnitAST &unit, std::string &code) {
    std::string str;
    int tmp = T_num;
    for (const auto &node: unit.getNodes()) {
        if (dynamic_cast<VarDeclAST*>(node.get()))
            node->GenerateIR(*this, str);
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
            code += (tab + "var T" + std::to_string(i));
        }
    }
    code += str;
    for (const auto &node: unit.getNodes()) {
        if (!dynamic_cast<VarDeclAST*>(node.get()))
            node->GenerateIR(*this, code);
    }
}

void IRGenerator::GenFuncCall(FuncCallAST &func, std::string &code) {
    std::vector<std::string> args;
    for (const auto &arg: func.getArgs()) {
        std::string res = arg->GenerateIR(*this, code);
        args.push_back(res);
    }
    for (const auto &res: args) {
        code += ( tab + "param " + res + "\n");
    }
    code += ( tab + "call f_" + func.getName() + "\n");
}

std::string IRGenerator::GenUnaryExp(UnaryExpAST &exp, std::string &code) {
    std::string res;
    res += op2char(exp.getOp());
    res += exp.getNode()->GenerateIR(*this, code);
    return res;
}

std::string IRGenerator::GenLVal(LValAST &lval, std::string &code) {
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
    for (int i = T_tmp; i < T_num; i++) {
        if (ReverseSymbolTable[i].second == VarType::VAR) {
            code += (tab + "var T" + std::to_string(i));
        } else {
            std::vector<int> dim = FuncArrayTable[funcDef.getName()][ReverseSymbolTable[i].first];
            int size = 4;
            for (int d: dim) {
                size *= d;
            }
            code += (tab + "var " + std::to_string(size) + " " + ReverseSymbolTable[i].first);
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
    if (stmt.getStmt()) {
        stmt.getStmt()->GenerateIR(*this, code);
    }
}

void IRGenerator::GenBlock(BlockAST &block, std::string &code) {
    for (const auto & stmt : block.getStmts()) {
        stmt->GenerateIR(*this, code);
    }
}

void IRGenerator::GenIfElse(IfElseAST &stmt ,std::string &code) {
    std::string cond = stmt.getCond()->GenerateIR(*this, code);
    code += (tab + "if " + cond + " == 0 goto l" + std::to_string(l_num++) + "\n");
    stmt.getThenStmt()->GenerateIR(*this, code);
    code += ("l" + std::to_string(--l_num) + ":\n");
    stmt.getElseStmt()->GenerateIR(*this, code);
}

void IRGenerator::GenWhile(WhileAST &stmt, std::string &code) {
    code += ("l" + std::to_string(l_num++) + "\n");
    std::string cond = stmt.getCond()->GenerateIR(*this, code);
    code += (tab + "if " + cond + " == 0 goto l" + std::to_string(l_num++) + "\n");
    stmt.getStmt()->GenerateIR(*this, code);
    code += ("l" + std::to_string(--l_num) + ":\n");
}

void IRGenerator::GenControl(ControlAST &stmt, std::string &code) {
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
                code += (tab + "return " + ret + "\n");
            }
            else code += (tab + "return\n");
        }
        default:
            break;
    }
}
