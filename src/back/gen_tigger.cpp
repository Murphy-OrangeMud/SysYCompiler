#include "gen_tigger.hpp"

// TODO：这个版本的code generation没有寄存器分配，后面如果要考虑性能的话要把它和寄存器分配代码连接起来
namespace EeyoreToTigger {
    std::string TiggerGenerator::op2char(Operator op) {
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
            case NOT:
                c = "!";
                break;
        }
        return c;
    }

    std::string TiggerGenerator::GenDecl(DeclIR &decl, std::string &code) {
        logger.SetFunc("GenDecl");
        if (currentFunc.empty()) {
            // global decl
            if (decl.getType() == VarType::ARRAY) {
                globalVars[decl.getName()] = Variable(v_num, VarType::ARRAY);
                code += "v" + std::to_string(v_num++) + " = malloc " + std::to_string(decl.getSize()) + "\n";
            } else {
                globalVars[decl.getName()] = Variable(v_num, VarType::VAR);
                code += "v" + std::to_string(v_num++) + " = 0\n";
            }
        } else {
            // local var
            if (decl.getType() == VarType::ARRAY) {
                varStack[decl.getName()] = StackVar(funcStack[currentFunc], funcStack[currentFunc] + decl.getSize() / 4,
                                                    currentFunc, VarType::ARRAY);
                funcStack[currentFunc] += decl.getSize() / 4;
            } else {
                varStack[decl.getName()] = StackVar(funcStack[currentFunc], funcStack[currentFunc], currentFunc, VarType::VAR);
                funcStack[currentFunc]++;
            }
        }
        return {};
    }

    std::string TiggerGenerator::GenInit(InitIR &init, std::string &code) {
        logger.SetFunc("GenInit");
        if (currentFunc.empty()) {
            if (init.getType() == VarType::VAR) {
                globalVars[init.getName()] = Variable(v_num, VarType::VAR);
                code += "v" + std::to_string(v_num++) + " = " + std::to_string(init.getVal()) + "\n";
            } else {
                code += "loadaddr v" + std::to_string(globalVars[init.getName()].v_num) + " s" +
                        std::to_string(reg_num) + " " + "\n";
                code += "s" + std::to_string(reg_num) + "[" + std::to_string(init.getPos()) + "] = " +
                        std::to_string(init.getVal()) + "\n";
                //reg_num--;
            }
        } else {
            code += "\ts" + std::to_string(reg_num) + " = " + std::to_string(init.getVal()) + "\n";
            code += "\tstore s" + std::to_string(reg_num) + " "
                    + std::to_string(
                    varStack[init.getName()].pos_min + (init.getType() == VarType::ARRAY ? init.getPos() : 0)) + "\n";
            //reg_num--;
        }
        return {};
    }

    std::string TiggerGenerator::GenFuncDef(FuncDefIR &funcDef, std::string &code) {
        logger.SetFunc("GenFuncDef");
        currentFunc = funcDef.getName();
        funcStack[currentFunc] = funcDef.getParamNum();
        for (int i = 0; i < funcDef.getParamNum(); i++) {
            varStack["p" + std::to_string(i)].pos_min = varStack["p" + std::to_string(i)].pos_max = i;
        }
        std::string funcHeader = currentFunc + " [" + std::to_string(funcDef.getParamNum()) + "] [";
        std::string funcBody;
        std::string funcEnd = "end " + currentFunc + "\n";
        funcDef.getBody()->Generate(*this, funcBody);
        logger.UnSetFunc("GenFuncDef");
        funcHeader += std::to_string(funcStack[currentFunc]) + "]\n";
        reg_num = 1;
        currentFunc = "";
        code += funcHeader;
        for (int i = 0; i < funcDef.getParamNum(); i++) {
            code += "\tstore a" + std::to_string(i) + " " + std::to_string(i) + "\n";
        }
        code += funcBody + funcEnd;
        return {};
    }

    std::string TiggerGenerator::GenFuncCall(FuncCallIR &funcCall, std::string &code) {
        logger.SetFunc("GenFuncCall");
        code += "\tcall " + funcCall.getName() + "\n";
        return {};
    }

    std::string TiggerGenerator::GenLVal(LValIR &lval, std::string &code) {
        logger.SetFunc("GenLVal");
        if (lval.getName()[0] == 'T') {
            if (globalVars.find(lval.getName()) != globalVars.end()) {
                // global var
                code += "\tloadaddr v" + std::to_string(globalVars[lval.getName()].v_num) + " s" +
                        std::to_string(reg_num) + "\n";
                if (lval.getType() == VarType::VAR) {
                    std::string ret = "s" + std::to_string(reg_num) + "[0]";
                    reg_num++;
                    return ret;
                } else {
                    if (dynamic_cast<RightValIR *>(lval.getPos().get())->getType() == Token::NUMBER) {
                        std::string ret = "s" + std::to_string(reg_num) + "[" +
                                          std::to_string(dynamic_cast<RightValIR *>(lval.getPos().get())->getVal()) +
                                          "]";
                        reg_num++;
                        return ret;
                    } else {
                        code += "\tload " + std::to_string(
                                varStack[dynamic_cast<RightValIR *>(lval.getPos().get())->getName()].pos_min) + " t0\n";
                        code += "\ts" + std::to_string(reg_num) + " = s" + std::to_string(reg_num) + " + t0\n";
                        std::string ret = "s" + std::to_string(reg_num) + "[0]";
                        reg_num++;
                        return ret;
                    }
                }
            } else {
                if (lval.getType() == VarType::ARRAY) {
                    code += "\tloadaddr " + std::to_string(varStack[lval.getName()].pos_min) + " s" +
                            std::to_string(reg_num) + "\n";
                    if (dynamic_cast<RightValIR *>(lval.getPos().get())->getType() == Token::NUMBER) {
                        std::string ret = "s" + std::to_string(reg_num) + "[" +
                                          std::to_string(dynamic_cast<RightValIR *>(lval.getPos().get())->getVal()) +
                                          "]";
                        reg_num++;
                        return ret;
                    } else {
                        code += "\tload " + std::to_string(
                                varStack[dynamic_cast<RightValIR *>(lval.getPos().get())->getName()].pos_min) + " t0\n";
                        code += "\ts" + std::to_string(reg_num) + " = s" + std::to_string(reg_num) + " + t0\n";
                        std::string ret = "s" + std::to_string(reg_num) + "[0]";
                        reg_num++;
                        return ret;
                    }
                } else {
                    std::string ret = "s" + std::to_string(reg_num);
                    code += "\tload " + std::to_string(varStack[lval.getName()].pos_min) + " s" +
                            std::to_string(reg_num) + "\n";
                    reg_num++;
                    return ret;
                }
            }
        } else if (lval.getName()[0] == 'p') {
            if (lval.getType() == VarType::VAR) {
                std::string ret = lval.getName();
                ret[0] = 'a';
                code += "\tload " + std::to_string(varStack[lval.getName()].pos_min) + " " + ret + "\n";
                return ret;
            } else {
                std::string ret = lval.getName();
                ret[0] = 'a';
                code += "\tload " + std::to_string(varStack[lval.getName()].pos_min) + " " + ret + "\n";
                std::string pos = lval.getPos()->Generate(*this, code);
                code += "\ts" + std::to_string(reg_num) + " = " + ret + " + " + pos + "\n";
                ret = "s" + std::to_string(reg_num);
                reg_num++;
                ret = ret + "[0]";
                return ret;
            }
        } else {
            std::string ret = "s" + std::to_string(reg_num);
            code += "\tload " + std::to_string(varStack[lval.getName()].pos_min) + " s" + std::to_string(reg_num) +
                    "\n";
            reg_num++;
            return ret;
        }
    }

    std::string TiggerGenerator::GenAssign(AssignIR &assign, std::string &code) {
        logger.SetFunc("GenAssign");
        std::string lhs = assign.getLHS()->Generate(*this, code);
        logger.UnSetFunc("GenAssign");
        std::string rhs = assign.getRHS()->Generate(*this, code);
        logger.UnSetFunc("GenAssign");
        if (dynamic_cast<FuncCallIR *>(assign.getRHS().get())) {
            code += "\t" + lhs + " = a0\n";
            if (lhs.find("[") == std::string::npos) {
                code += "\tstore " + lhs + " " + std::to_string(varStack[dynamic_cast<LValIR*>(assign.getLHS().get())->getName()].pos_min) + "\n";
            }
            if (lhs[0] == 's') {
                //reg_num--;
                reg_num = 1;
            }
            return {};
        } else {
            if (!(rhs[0] == 'a' || rhs[0] == 's' || rhs[0] == 't')) {
                code += "\ts" + std::to_string(reg_num) + " = " + rhs + "\n";
                rhs = "s" + std::to_string(reg_num);
                reg_num++;
            }
            if (dynamic_cast<RightValIR*>(assign.getRHS().get()) && rhs[0] == 's') {
                //reg_num--;
            }
            if (dynamic_cast<LValIR*>(assign.getRHS().get()) && rhs[0] == 's') {
                //reg_num--;
            }
            code += "\t" + lhs + " = " + rhs + "\n";
            if (lhs.find("[") == std::string::npos) {
                code += "\tstore " + lhs + " " + std::to_string(varStack[dynamic_cast<LValIR*>(assign.getLHS().get())->getName()].pos_min) + "\n";
            }
            if (lhs[0] == 's') {
                //reg_num--;
                reg_num = 1;
            }
            return {};
        }
    }

    std::string TiggerGenerator::GenUnaryExp(UnaryExpIR &unary, std::string &code) {
        logger.SetFunc("GenUnaryExp");
        // 这里既然是unary exp的话一定是symbol
        if (dynamic_cast<RightValIR*>(unary.getExp().get())->getType() == Token::SYMBOL) {
            code += "\tload " +
                    std::to_string(varStack[dynamic_cast<RightValIR *>(unary.getExp().get())->getName()].pos_min) +
                    " s" + std::to_string(reg_num) + "\n";
            if (unary.getOp() == Operator::ADD) {
                return "s" + std::to_string(reg_num);
            }
            return op2char(unary.getOp()) + "s" + std::to_string(reg_num);
        } else {
            if (unary.getOp() == Operator::SUB) {
                return std::to_string(-dynamic_cast<RightValIR*>(unary.getExp().get())->getVal());
            } else if (unary.getOp() == Operator::NOT) {
                return std::to_string(!dynamic_cast<RightValIR*>(unary.getExp().get())->getVal());
            } else {
                return std::to_string(dynamic_cast<RightValIR*>(unary.getExp().get())->getVal());
            }
        }
    }

    std::string TiggerGenerator::GenBinaryExp(BinaryExpIR &binary, std::string &code) {
        logger.SetFunc("GenBinaryExp");
        std::string lhs = binary.getLHS()->Generate(*this, code);
        logger.UnSetFunc("GenBinaryExp");
        std::string rhs = binary.getRHS()->Generate(*this, code);
        logger.UnSetFunc("GenBinaryExp");
        std::set<std::string> logicOp{">=", "<=", "==", "!=", ">", "<"};
        if (dynamic_cast<RightValIR*>(binary.getRHS().get())->getType() == Token::NUMBER
        && logicOp.find(op2char(binary.getOp())) != logicOp.end()) {
            code += "\ts" + std::to_string(reg_num) + " = " + rhs + "\n";
            rhs = "s" + std::to_string(reg_num);
            reg_num++;
        }
        if (dynamic_cast<RightValIR*>(binary.getLHS().get())->getType() == Token::NUMBER
        && dynamic_cast<RightValIR*>(binary.getRHS().get())->getType() == Token::SYMBOL) {
            code += "\ts" + std::to_string(reg_num) + " = " + lhs + "\n";
            lhs = "s" + std::to_string(reg_num);
            reg_num++;
        }
        if (rhs[0] == 's') {
            //reg_num--;
        }
        if (lhs[0] == 's') {
            //reg_num--;
        }
        return lhs + " " + op2char(binary.getOp()) + " " + rhs;
    }

    std::string TiggerGenerator::GenLabel(Label &label, std::string &code) {
        logger.SetFunc("GenLabel");
        code += "l" + std::to_string(label.getNum()) + ":\n";
        return {};
    }

    std::string TiggerGenerator::GenCondGoto(CondGotoIR &cond, std::string &code) {
        logger.SetFunc("GenCondGoto");
        std::string conds = cond.getCond()->Generate(*this, code);
        logger.UnSetFunc("GenCondGoto");
        code += "\tif " + conds + " goto l" + std::to_string(cond.getLabel()) + "\n";
        return {};
    }

    std::string TiggerGenerator::GenGoto(GotoIR &gt, std::string &code) {
        logger.SetFunc("GenGoto");
        code += "\tgoto l" + std::to_string(gt.getLabel()) + "\n";
        return {};
    }

    std::string TiggerGenerator::GenRightVal(RightValIR &rightval, std::string &code) {
        logger.SetFunc("GenRightVal");
        if (rightval.getType() == Token::NUMBER) {
            return std::to_string(rightval.getVal());
        } else {
            std::string ret;
            if (rightval.getName()[0] == 'p') {
                ret = rightval.getName();
                ret[0] = 'a';
                code += "\tload " + std::to_string(varStack[rightval.getName()].pos_min) + " " + ret + "\n";
                return ret;
            }
            if (globalVars.find(rightval.getName()) == globalVars.end()) {
                if (varStack[rightval.getName()].type == VarType::VAR) {
                    code += "\tload " + std::to_string(varStack[rightval.getName()].pos_min) + " s" + std::to_string(reg_num) + "\n";
                    ret = "s" + std::to_string(reg_num);
                    reg_num++;
                    return ret;
                } else {
                    code += "\tloadaddr " + std::to_string(varStack[rightval.getName()].pos_min) + " s" + std::to_string(reg_num) + "\n";
                    ret = "s" + std::to_string(reg_num);
                    reg_num++;
                    return ret;
                }
            } else {
                if (globalVars[rightval.getName()].varType == VarType::VAR) {
                    code += "\tload v" + std::to_string(globalVars[rightval.getName()].v_num) + " s" + std::to_string(reg_num) + "\n";
                    ret = "s" + std::to_string(reg_num);
                    reg_num++;
                    return ret;
                } else {
                    code += "\tloadaddr v" + std::to_string(globalVars[rightval.getName()].v_num) + " s" + std::to_string(reg_num) + "\n";
                    ret = "s" + std::to_string(reg_num);
                    reg_num++;
                    return ret;
                }
            }
        }
    }

    std::string TiggerGenerator::GenReturn(ReturnIR &ret, std::string &code) {
        logger.SetFunc("GenReturn");
        if (ret.getReturnValue()) {
            std::string r = ret.getReturnValue()->Generate(*this, code);
            logger.UnSetFunc("GenReturn");
            code += "\ta0 = " + r + "\n";
        }
        code += "\treturn\n";
        return {};
    }

    std::string TiggerGenerator::GenParamList(ParamListIR &params, std::string &code) {
        logger.SetFunc("GenParamList");
        for (int i = 0; i < params.getParams().size(); i++) {
            std::string param = params.getParams()[i]->Generate(*this, code);
            code += "\ta" + std::to_string(i) + " = " + param + "\n";
        }
        return {};
    }

    std::string TiggerGenerator::GenStatements(StatementsIR &stmts, std::string &code) {
        logger.SetFunc("GenStatements");
        for (const auto &stmt : stmts.getStmts()) {
            stmt->Generate(*this, code);
            logger.UnSetFunc("GenStatements");
        }
        return {};
    }

    std::string TiggerGenerator::GenProgram(ProgramIR &program, std::string &code) {
        logger.SetFunc("GenProgram");
        for (const auto &stmt : program.getNodes()) {
            stmt->Generate(*this, code);
            logger.UnSetFunc("GenStatements");
        }
        return {};
    }
}
