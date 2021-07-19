#include "../define/ast.hpp"
#include "genIR.hpp"
#include <functional>
#include <vector>

namespace SysYToEeyore {
    void
    IRGenerator::GenerateValue(const std::string &varName, int &idx, int indx, InitValAST *init, std::vector<int> dim,
                               int i,
                               std::string &code) {
        logger.SetFunc("GenerateValue");
        int elem = 1;
        for (int j = i + 1; j < dim.size(); j++) {
            elem *= dim[j];
        }
        if (init) {
            int i_idx = 0;
            int index = 0;
            for (const auto &initval: init->getValues()) {
                if (dynamic_cast<NumberAST *>(initval.get())) {
                    index++;
                    if (index == elem) {
                        index = 0;
                        i_idx++;
                    }
                    for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                    code += (varName + "[" + std::to_string(idx * 4) + "] = " +
                             std::to_string(dynamic_cast<NumberAST *>(initval.get())->getVal()) + "\n");
                    idx++;
                } else if (dynamic_cast<InitValAST *>(initval.get())) {
                    if (dynamic_cast<InitValAST *>(initval.get())->getType() == VarType::VAR) {
                        //列表中只有一个元素
                        if (dynamic_cast<NumberAST *>(dynamic_cast<InitValAST *>(initval.get())->getValues()[0].get())) {
                            index++;
                            if (index == elem) {
                                index = 0;
                                i_idx++;
                            }
                            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                            code += (varName + "[" + std::to_string(idx * 4) + "] = " + std::to_string(
                                    dynamic_cast<NumberAST *>(dynamic_cast<InitValAST *>(initval.get())->getValues()[0].get())->getVal()) +
                                     "\n");
                            idx++;
                        } else {
                            std::string res = dynamic_cast<InitValAST *>(initval.get())->getValues()[0]->GenerateIR(
                                    *this, code);
                            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                            code += ("t" + std::to_string(t_num) + " = " + res + "\n");
                            res = "t" + std::to_string(t_num);
                            t_num++;
                            index++;
                            if (index == elem) {
                                index = 0;
                                i_idx++;
                            }
                            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                            code += (varName + "[" + std::to_string(idx * 4) + "] = " + res + "\n");
                            idx++;
                        }
                    } else {
                        // 新的一维初始化，和前面那一维没关系
                        i_idx++;
                        GenerateValue(varName, idx, 0, dynamic_cast<InitValAST *>(initval.get()), dim, i + 1, code);
                        logger.UnSetFunc("GenerateValue");
                    }
                }
            }
            for (int j = i_idx; j < dim[i]; j++) {
                GenerateValue(varName, idx, index, nullptr, dim, i + 1, code);
                logger.UnSetFunc("GenerateValue");
            }
        } else {
            if (i == dim.size()) {
                for (int k = 0; k < currentDepth; k++) { code += "\t"; }
                code += (varName + "[" + std::to_string(idx * 4) + "] = 0\n");
                idx++;
            } else if (i == dim.size() - 1) {
                for (int j = indx; j < dim[i]; j++) {
                    for (int k = 0; k < currentDepth; k++) { code += "\t"; }
                    code += (varName + "[" + std::to_string(idx * 4) + "] = 0\n");
                    idx++;
                }
            } else {
                for (int j = indx; j < dim[i]; j++) {
                    GenerateValue(varName, idx, 0, nullptr, dim, i + 1, code);
                    logger.UnSetFunc("GenerateValue");
                }
            }
        }
    }

    std::string IRGenerator::op2char(Operator op) {
        std::string c;
        switch (op) {
            case Operator::ADD:
                c = "+";
                break;
            case Operator::SUB:
                c = "-";
                break;
            case Operator::DIV:
                c = "/";
                break;
            case Operator::MOD:
                c = "%";
                break;
            case Operator::MUL:
                c = "*";
                break;
            case Operator::AND:
                c = "&&";
                break;
            case Operator::EQ:
                c = "==";
                break;
            case Operator::GE:
                c = ">=";
                break;
            case Operator::NEQ:
                c = "!=";
                break;
            case Operator::LE:
                c = "<=";
                break;
            case Operator::LT:
                c = "<";
                break;
            case Operator::GT:
                c = ">";
                break;
            case Operator::OR:
                c = "||";
                break;
            case Operator::NONE:
                c = "";
                break;
            case Operator::NOT:
                c = "!";
                break;
        }
        return c;
    }

    std::string IRGenerator::GenBinaryExp(BinaryExpAST &exp, std::string &code) {
        logger.SetFunc("GenBinaryExp");
        std::string t1 = exp.getLHS()->GenerateIR(*this, code);
        logger.UnSetFunc("GenBinaryExp");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("t" + std::to_string(t_num) + " = " + t1 + "\n");
        t1 = "t" + std::to_string(t_num);
        t_num++;
        std::string t2 = exp.getRHS()->GenerateIR(*this, code);
        logger.UnSetFunc("GenBinaryExp");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("t" + std::to_string(t_num) + " = " + t2 + "\n");
        t2 = "t" + std::to_string(t_num);
        t_num++;
        std::string res = "t" + std::to_string(t_num++);
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += (res + " = " + t1 + " " + op2char(exp.getOp()) + " " + t2 + "\n");
        return res;
    }

    std::string IRGenerator::GenNumber(NumberAST &num, std::string &code) {
        logger.SetFunc("GenNumber");
        return std::to_string(num.getVal());
    }

    std::string IRGenerator::GenVarDef(VarDefAST &varDef, std::string &code) {
        logger.SetFunc("GenVarDef");
        std::string var = varDef.getVar()->GenerateIR(*this, code);
        logger.UnSetFunc("GenVarDef");
        if (varDef.getInitVal()) {
            varDef.getInitVal()->GenerateIR(*this, code);
            logger.UnSetFunc("GenVarDef");
        } else {
            if (dynamic_cast<ProcessedIdAST *>(varDef.getVar().get())->getType() == VarType::VAR) {
                if (currentFunc.empty()) {
                    for (int i = 0; i < currentDepth; i++) { code += "\t"; }
                    code += (var + " = 0\n");
                }
            } else {
                if (currentFunc.empty()) {
                    int idx = 0;
                    GenerateValue(var, idx, 0, nullptr, dynamic_cast<ProcessedIdAST *>(varDef.getVar().get())->getDim(),
                                  0, code);
                    logger.UnSetFunc("GenVarDef");
                }
            }
        }
        return {};
    }

// ID：全是变量，没有函数，和左值也是分开的
    std::string IRGenerator::GenId(ProcessedIdAST &id, std::string &code) {
        logger.SetFunc("GenId");
        std::map<std::string, GenVar>::iterator iter;
        int tmpCurrentBlock = currentBlock;
        while (tmpCurrentBlock != -1) {
            iter = BlockSymbolTable[tmpCurrentBlock].find(id.getName());
            if (iter != BlockSymbolTable[tmpCurrentBlock].end()) {
                break;
            }
            tmpCurrentBlock = parentBlock[tmpCurrentBlock];
        }
        if (iter->second.id.empty()) {
            iter->second.id = "T" + std::to_string(T_num++);
            ReverseSymbolTable.push_back(iter->second); // ReverseSymbolTable实际上就是T_num到var信息的一个map
        }
        return iter->second.id;
    }

    std::string IRGenerator::GenInitVal(InitValAST &init, std::string &code) {
        logger.SetFunc("GenInitVal");
        if (init.getType() == VarType::VAR) {
            std::string res = init.getValues()[0]->GenerateIR(*this, code);
            logger.SetFunc("GenInitVal");
            for (int i = 0; i < currentDepth; i++) { code += "\t"; }
            code += ("T" + std::to_string(T_num - 1) + " = " + res + "\n");
        } else {
            int idx = 0;
            GenerateValue("T" + std::to_string(T_num - 1), idx, 0, &init, init.getDims(), 0, code);
            logger.SetFunc("GenInitVal");
        }
        return "T" + std::to_string(T_num - 1);
    }

    std::string IRGenerator::GenAssign(AssignAST &assign, std::string &code) {
        logger.SetFunc("GenAssign");
        std::string l = assign.getLeft()->GenerateIR(*this, code);
        logger.UnSetFunc("GenAssign");
        std::string r = assign.getRight()->GenerateIR(*this, code);
        logger.UnSetFunc("GenAssign");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("t" + std::to_string(t_num) + " = " + r + "\n");
        r = "t" + std::to_string(t_num);
        t_num++;
        for (int i = 0; i < currentDepth; i++) { code += "\t"; }
        code += (l + " = " + r + "\n");
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
        parentBlock.push_back(-1);
        for (const auto &node: unit.getNodes()) {
            if (dynamic_cast<VarDeclAST *>(node.get())) {
                currentDepth += 1;
                node->GenerateIR(*this, str);
                logger.UnSetFunc("GenCompUnit");
                currentDepth -= 1;
            }
        }
        for (int i = tmp; i < T_num; i++) {
            if (ReverseSymbolTable[i].argType == VarType::ARRAY) {
                std::vector<int> dim = ReverseSymbolTable[i].dims;
                int size = 4;
                for (int d : dim) {
                    size *= d;
                }
                for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                code += ("var " + std::to_string(size) + " T" + std::to_string(i) + "\n");
            } else {
                for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                code += ("var T" + std::to_string(i) + "\n");
            }
        }
        for (const auto &node: unit.getNodes()) {
            if (!dynamic_cast<VarDeclAST *>(node.get())) {
                if (dynamic_cast<FuncDefAST *>(node.get()) && dynamic_cast<FuncDefAST *>(node.get())->getName() == "main") {
                    code += "f_main [0]\n";
                    code += str;
                }
                node->GenerateIR(*this, code);
                logger.UnSetFunc("GenCompUnit");
            }
        }
    }

    std::string IRGenerator::GenFuncCall(FuncCallAST &func, std::string &code) {
        logger.SetFunc("GenFuncCall");
        std::vector<std::string> args;
        for (const auto &arg: func.getArgs()) {
            std::string res = arg->GenerateIR(*this, code);
            logger.UnSetFunc("GenFuncCall");
            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
            code += ("t" + std::to_string(t_num) + " = " + res + "\n");
            res = "t" + std::to_string(t_num);
            t_num++;
            args.push_back(res);
        }
        for (const auto &res: args) {
            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
            code += ("param " + res + "\n");
        }
        if (FuncTable[func.getName()].funcType == Type::VOID) {
            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
            code += ("call f_" + func.getName() + "\n");
            return {};
        } else {
            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
            code += ("t" + std::to_string(t_num) + " = call f_" + func.getName() + "\n");
            std::string res = "t" + std::to_string(t_num);
            t_num++;
            return res;
        }
    }

    std::string IRGenerator::GenUnaryExp(UnaryExpAST &exp, std::string &code) {
        logger.SetFunc("GenUnaryExp");
        std::string ret;
        if (exp.getOp() != Operator::ADD) {
            ret += op2char(exp.getOp());
        }
        std::string res = exp.getNode()->GenerateIR(*this, code);
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("t" + std::to_string(t_num) + " = " + res + "\n");
        res = "t" + std::to_string(t_num);
        t_num++;
        ret += res;
        if (exp.getOp() != Operator::NONE) {
            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
            code += ("t" + std::to_string(t_num) + " = " + ret + "\n");
            ret = "t" + std::to_string(t_num);
            t_num++;
        }
        logger.UnSetFunc("GenUnaryExp");
        return ret;
    }

    std::string IRGenerator::GenLVal(LValAST &lval, std::string &code) {
        logger.SetFunc("GenLVal");
        std::map<std::string, GenVar>::iterator iter;
        int tmpCurrentBlock = currentBlock;
        while (tmpCurrentBlock != -1) {
            iter = BlockSymbolTable[tmpCurrentBlock].find(lval.getName());
            if (iter != BlockSymbolTable[tmpCurrentBlock].end() && !iter->second.id.empty()) {
                break;
            }
            tmpCurrentBlock = parentBlock[tmpCurrentBlock];
        }
        if (lval.getType() == VarType::VAR) {
            return iter->second.id;
        } else {
            std::string name = iter->second.id;
            std::vector<int> dim = iter->second.dims;
            int tmp;
            for (size_t i = 0; i < lval.getPosition().size(); i++) {
                std::string var = lval.getPosition()[i]->GenerateIR(*this, code);
                logger.UnSetFunc("GenLVal");
                for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                code += ("t" + std::to_string(t_num) + " = " + var + "\n");
                var = "t" + std::to_string(t_num);
                t_num++;
                if (i < lval.getPosition().size() - 1) {
                    for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                    code += ("t" + std::to_string(t_num) + " = " + var + " * " + std::to_string(dim[i + 1]) + "\n");
                    for (int k = i + 2; k < lval.getPosition().size(); k++) {
                        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                        code += ("t" + std::to_string(t_num) + " = " + "t" + std::to_string(t_num) + " * " +
                                 std::to_string(dim[k]) + "\n");
                    }
                } else {
                    for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                    code += ("t" + std::to_string(t_num) + " = " + var + "\n");
                }
                if (i > 0) {
                    for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                    code += ("t" + std::to_string(t_num) + " = t" + std::to_string(tmp) + " + t" +
                             std::to_string(t_num) + "\n");
                }
                tmp = t_num++;
            }
            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
            code += ("t" + std::to_string(tmp) + " = 4 * t" + std::to_string(tmp) + "\n");
            std::string res = name + "[t" + std::to_string(tmp) + "]";
            return res;
        }
    }

    void IRGenerator::GenFuncDef(FuncDefAST &funcDef, std::string &code) {
        logger.SetFunc("GenFuncDef");
        currentFunc = funcDef.getName();
        for (size_t i = 0; i < funcDef.getArgs().size(); i++) {
            BlockSymbolTable[parentBlock.size()][dynamic_cast<ProcessedIdAST *>(funcDef.getArgs()[i].get())->getName()].id =
                    "p" + std::to_string(i);
        }
        if (funcDef.getName() != "main") {
            code += ("f_" + funcDef.getName() + " [" + std::to_string(funcDef.getArgs().size()) + "]\n");
        }
        int T_tmp = T_num;
        int t_tmp = t_num;
        std::string code2;
        funcDef.getBody()->GenerateIR(*this, code2);
        logger.UnSetFunc("GenFuncDef");
        for (int i = T_tmp; i < T_num; i++) {
            if (ReverseSymbolTable[i].argType == VarType::VAR) {
                for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
                code += ("var T" + std::to_string(i) + "\n");
            } else {
                std::vector<int> dim = ReverseSymbolTable[i].dims;
                int size = 4;
                for (int d: dim) {
                    size *= d;
                }
                for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
                code += ("var " + std::to_string(size) + " " + ReverseSymbolTable[i].id + "\n");
            }
        }
        for (int i = t_tmp; i < t_num; i++) {
            for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
            code += ("var t" + std::to_string(i) + "\n");
        }
        code += code2;
        for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
        code += "return";
        code += (funcDef.getType() == Type::INT ? " 0\n" : " \n");
        code += "\nend f_" + funcDef.getName() + "\n";
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
        ++currentDepth;
        parentBlock.push_back(currentBlock);
        currentBlock = parentBlock.size() - 1;
        for (const auto &stmt : block.getStmts()) {
            stmt->GenerateIR(*this, code);
            logger.UnSetFunc("GenBlock");
        }
        --currentDepth;
        currentBlock = parentBlock[currentBlock];
    }

    void IRGenerator::GenIfElse(IfElseAST &stmt, std::string &code) {
        logger.SetFunc("GenIfElse");
        std::string cond = stmt.getCond()->GenerateIR(*this, code);
        logger.UnSetFunc("GenIfElse");
        int tmp1 = l_num;
        if (cond.find("[") != std::string::npos) {
            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
            code += ("t" + std::to_string(t_num) + " = " + cond + "\n");
            cond = "t" + std::to_string(t_num);
            t_num++;
        }
        if (!(cond[0] >= 'a' && cond[0] <= 'z')) {
            for (int j = 0; j < currentDepth; j++) { code += "\t"; }
            code += ("t" + std::to_string(t_num) + " = " + cond + "\n");
            cond = "t" + std::to_string(t_num);
            t_num++;
        }
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("if " + cond + " == 0 goto l" + std::to_string(tmp1) + "\n");
        l_num++;
        stmt.getThenStmt()->GenerateIR(*this, code);
        logger.UnSetFunc("GenIfElse");
        if (stmt.getElseStmt()) {
            int tmp = tmp1;
            std::string branch;
            stmt.getElseStmt()->GenerateIR(*this, branch);
            for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
            code += ("goto l" + std::to_string(l_num) + "\n");
            code += ("l" + std::to_string(tmp) + ":\n");
            code += branch;
            logger.UnSetFunc("GenIfElse");
            code += ("l" + std::to_string(l_num) + ":\n");
            l_num++;
        } else {
            code += ("l" + std::to_string(tmp1) + ":\n");
        }
    }

    void IRGenerator::GenWhile(WhileAST &stmt, std::string &code) {
        logger.SetFunc("GenWhile");

        int old_break = cur_break_l;
        int old_continue = cur_continue_l;

        int begin_loop = l_num;
        code += ("l" + std::to_string(begin_loop) + ":\n");
        l_num++;
        std::string cond = stmt.getCond()->GenerateIR(*this, code);
        logger.UnSetFunc("GenWhile");
        int out_loop = l_num;
        l_num++;

        cur_break_l = out_loop;
        cur_continue_l = begin_loop;

        for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
        code += ("if " + cond + " == 0 goto l" + std::to_string(out_loop) + "\n");
        stmt.getStmt()->GenerateIR(*this, code);
        for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
        code += ("goto l" + std::to_string(begin_loop) + "\n");
        logger.UnSetFunc("GenWhile");
        code += ("l" + std::to_string(out_loop) + ":\n");

        cur_break_l = old_break;
        cur_continue_l = old_continue;
    }

    void IRGenerator::GenControl(ControlAST &stmt, std::string &code) {
        logger.SetFunc("GenControl");
        switch (stmt.getControl()) {
            case Token::CONTINUE: {
                for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                code += ("goto l" + std::to_string(cur_continue_l) + "\n");
                break;
            }
            case Token::BREAK: {
                for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                code += ("goto l" + std::to_string(cur_break_l) + "\n");
                break;
            }
            case Token::RETURN: {
                if (stmt.getReturnExp()) {
                    std::string ret = stmt.getReturnExp()->GenerateIR(*this, code);
                    logger.UnSetFunc("GenControl");
                    for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                    code += ("t" + std::to_string(t_num) + " = " + ret + "\n");
                    ret = ("t" + std::to_string(t_num));
                    t_num++;
                    for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                    code += ("return " + ret + "\n");
                } else {
                    for (int j = 0; j < currentDepth; j++) { code += "\t"; }
                    code += ("return \n");
                }
            }
            default:
                break;
        }
    }

    std::string IRGenerator::GenLAndExp(BinaryExpAST &exp, std::string &code) {
        logger.SetFunc("GenLAndExp");
        std::string t1 = exp.getLHS()->GenerateIR(*this, code);
        logger.UnSetFunc("GenLAndExp");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("t" + std::to_string(t_num) + " = " + t1 + "\n");
        t1 = "t" + std::to_string(t_num);
        t_num++;
        int shorthand = l_num;
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("if " + t1 + " == 0 goto l" + std::to_string(l_num++) + "\n");
        std::string t2 = exp.getRHS()->GenerateIR(*this, code);
        logger.UnSetFunc("GenLAndExp");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("t" + std::to_string(t_num) + " = " + t2 + "\n");
        t2 = "t" + std::to_string(t_num);
        t_num++;
        code += ("goto l" + std::to_string(l_num++) + "\n");
        code += ("l" + std::to_string(shorthand) + ":\n");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += (t2 + " = 0\n");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("l" + std::to_string(l_num - 1) + ":\n");
        return t2;
    }

    std::string IRGenerator::GenLOrExp(BinaryExpAST &exp, std::string &code) {
        logger.SetFunc("GenLOrExp");
        std::string t1 = exp.getLHS()->GenerateIR(*this, code);
        logger.UnSetFunc("GenLOrExp");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("t" + std::to_string(t_num) + " = " + t1 + "\n");
        t1 = "t" + std::to_string(t_num);
        t_num++;
        int shorthand = l_num;
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("if " + t1 + " == 1 goto l" + std::to_string(l_num++) + "\n");
        std::string t2 = exp.getRHS()->GenerateIR(*this, code);
        logger.UnSetFunc("GenLOrExp");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += ("t" + std::to_string(t_num) + " = " + t2 + "\n");
        t2 = "t" + std::to_string(t_num);
        t_num++;
        code += ("goto l" + std::to_string(l_num++) + "\n");
        code += ("l" + std::to_string(shorthand) + ":\n");
        for (int j = 0; j < currentDepth; j++) { code += "\t"; }
        code += (t2 + " = 1\n");
        code += ("l" + std::to_string(l_num - 1) + ":\n");
        return t2;
    }
}
