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
            }
        } else {
            // local var
            if (decl.getType() == VarType::ARRAY) {
                varStack[decl.getName()] = StackVar(funcStack[currentFunc], funcStack[currentFunc] + decl.getSize(), currentFunc);
                funcStack[currentFunc] += decl.getSize();
            } else {
                varStack[decl.getName()] = StackVar(funcStack[currentFunc], funcStack[currentFunc]++, currentFunc);
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
                code += "loadaddr v" + std::to_string(globalVars[init.getName()].v_num) + " s" + std::to_string(reg_num) + " " + "\n";
                code += "s"+ std::to_string(reg_num) + "[" + std::to_string(init.getPos()) + "] = " + std::to_string(init.getVal()) + "\n";
            }
        } else {
            code += "s"+ std::to_string(reg_num) + " = " + std::to_string(init.getVal());
            code += "store s" + std::to_string(reg_num) + " " + std::to_string(varStack[init.getName()].pos_min + init.getType() == VarType::ARRAY? init.getPos(): 0);
        }
        return {};
    }

    std::string TiggerGenerator::GenFuncDef(FuncDefIR &funcDef, std::string &code) {
        logger.SetFunc("GenFuncDef");
        currentFunc = funcDef.getName();
        funcStack[currentFunc] = 0;
        std::string funcHeader = currentFunc + " [" + std::to_string(funcDef.getParamNum()) + "] [";
        std::string funcBody;
        std::string funcEnd = "end " + currentFunc + "\n";
        funcDef.getBody()->Generate(*this, funcBody);
        funcHeader += std::to_string(funcStack[currentFunc]) + "]\n";
        currentFunc = "";
        code += funcHeader + funcBody + funcEnd;
        return {};
    }

    std::string TiggerGenerator::GenFuncCall(FuncCallIR &funcCall, std::string &code) {
        logger.SetFunc("GenFuncCall");
        code += "call " + funcCall.getName() + "\n";
        return {};
    }

    std::string TiggerGenerator::GenLVal(LValIR &lval, std::string &code) {
        logger.SetFunc("GenLVal");
        if (lval.getName()[0] == 'T') {
            if (globalVars.find(lval.getName()) != globalVars.end()) {
                // global var
                code += "loadaddr " + std::to_string(globalVars[lval.getName()].v_num) + " s" + std::to_string(reg_num) + "\n";
                if (lval.getType() == VarType::VAR) {
                    std::string ret = "s" + std::to_string(reg_num) + "[0]";
                    reg_num++;
                    return ret;
                } else {
                    if (dynamic_cast<RightValIR*>(lval.getPos().get())->getType() == Token::NUMBER) {
                        std::string ret = "s" + std::to_string(reg_num) + "[" + std::to_string(dynamic_cast<RightValIR*>(lval.getPos().get())->getVal()) + "]";
                        reg_num++;
                        return ret;
                    } else {
                        code += "load " + std::to_string(varStack[dynamic_cast<RightValIR*>(lval.getPos().get())->getName()].pos_min) + "t0\n";
                        code += "s" + std::to_string(reg_num) + " = s" + std::to_string(reg_num) + " + t0\n";
                        std::string ret = "s" + std::to_string(reg_num) + "[0]";
                        reg_num++;
                        return ret;
                    }
                }
            } else {
                std::string ret = "s" + std::to_string(reg_num);
                code += "load " + std::to_string(varStack[lval.getName()].pos_min) + " s" + std::to_string(reg_num);
                reg_num++;
                return ret;
            }
        } else {
            std::string ret = "s" + std::to_string(reg_num);
            code += "load " + std::to_string(varStack[lval.getName()].pos_min) + " s" + std::to_string(reg_num);
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
        if (dynamic_cast<LValIR*>(assign.getLHS().get())->getType() == VarType::VAR) {
            if (dynamic_cast<FuncCallIR*>(assign.getRHS().get())) {
                code += lhs + " = a0\n";
            } else {
                code += lhs + " = " + rhs + "\n";
            }
            code += "store " + lhs + std::to_string(varStack[lhs].pos_min) + "\n";
            reg_num--;
        } else {
            if (dynamic_cast<FuncCallIR*>(assign.getRHS().get())) {
                code += lhs + " = a0\n";
            } else {
                code += lhs + " = " + rhs + "\n";
            }
            reg_num--;
        }
        return {};
    }

    std::string TiggerGenerator::GenUnaryExp(UnaryExpIR &unary, std::string &code) {
        logger.SetFunc("GenUnaryExp");
        // 这里既然是unary exp的话一定是symbol
        code += "load " + std::to_string(varStack[dynamic_cast<RightValIR*>(unary.getExp().get())->getName()].pos_min) + "s" + std::to_string(reg_num) + "\n";
        if (unary.getOp() == Operator::ADD) {
            return "s" + std::to_string(reg_num++);
        }
        return op2char(unary.getOp()) + "s" + std::to_string(reg_num++);
    }

    std::string TiggerGenerator::GenBinaryExp(BinaryExpIR &binary, std::string &code) {
        logger.SetFunc("GenBinaryExp");
        std::string lhs = binary.getLHS()->Generate(*this, code);
        logger.UnSetFunc("GenBinaryExp");
        std::string rhs = binary.getRHS()->Generate(*this, code);
        logger.UnSetFunc("GenBinaryExp");
        return lhs + op2char(binary.getOp()) + rhs;
    }

    std::string TiggerGenerator::GenLabel(Label &label, std::string &code) {
        logger.SetFunc("GenLabel");
        code += "l" + std::to_string(label.getNum()) + ":\n";
        return {};
    }

    std::string TiggerGenerator::GenCondGoto(CondGotoIR &cond, std::string &code) {
        logger.SetFunc("GenCondGoto");
        std::string conds = cond.getCond()->Generate(*this, code);
        code += "if " + conds + " goto l" + std::to_string(cond.getLabel()) + "\n";
        return {};
    }

    std::string TiggerGenerator::GenGoto(GotoIR &gt, std::string &code) {
        logger.SetFunc("GenGoto");
        code += "goto " + std::to_string(gt.getLabel()) + "\n";
        return {};
    }

    std::string TiggerGenerator::GenRightVal(RightValIR &rightval, std::string &code) {
        logger.SetFunc("GenRightVal");
        if (rightval.getType() == Token::NUMBER) {
            return std::to_string(rightval.getVal());
        } else {
            return rightval.getName();
        }
    }

    std::string TiggerGenerator::GenReturn(ReturnIR &ret, std::string &code) {
        logger.SetFunc("GenReturn");
        if (ret.getReturnValue()) {
            if (dynamic_cast<RightValIR*>(ret.getReturnValue().get())->getType() == Token::NUMBER) {
                code += "a0 = " + std::to_string(dynamic_cast<RightValIR*>(ret.getReturnValue().get())->getVal()) + "\n";
            } else {
                code += "a0 = " + dynamic_cast<RightValIR*>(ret.getReturnValue().get())->getName() + "\n";
            }
        }
        code += "return\n";
        return {};
    }

    std::string TiggerGenerator::GenParamList(ParamListIR &params, std::string &code) {
        logger.SetFunc("GenParamList");
        for (int i = 0; i < params.getParams().size(); i++) {
            if (dynamic_cast<RightValIR*>(params.getParams()[i].get())->getType() == Token::NUMBER) {
                code += "a" + std::to_string(i) + " = " +std::to_string(dynamic_cast<RightValIR*>(params.getParams()[i].get())->getVal()) + "\n";
            } else {
                code += "a" + std::to_string(i) + " = " + dynamic_cast<RightValIR*>(params.getParams()[i].get())->getName() + "\n";
            }
        }
        return {};
    }

    std::string TiggerGenerator::GenStatements(StatementsIR &stmts, std::string &code) {
        logger.SetFunc("GenStatements");
        for (const auto & stmt : stmts.getStmts()) {
            stmt->Generate(*this, code);
        }
        return {};
    }

    std::string TiggerGenerator::GenProgram(ProgramIR &program, std::string &code) {
        logger.SetFunc("GenProgram");
        for (const auto &stmt : program.getNodes()) {
            stmt->Generate(*this, code);
        }
        return {};
    }
}
