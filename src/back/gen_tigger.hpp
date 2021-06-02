#ifndef GEN_TIGGER_INCLUDED
#define GEN_TIGGER_INCLUDED

#include <define/ir.hpp>
#include <map>
#include <utility>
#include <utils/logger.hpp>

namespace EeyoreToTigger {
    class TiggerGenerator {
    private:
        std::string currentFunc;
        int v_num;
        int reg_num;
        Logger logger;

        struct Variable {
            int v_num;
            VarType varType;
            Variable() {}
            Variable(int num, VarType type): v_num(num), varType(type) {}
        };

        struct StackVar {
            int pos_min;
            int pos_max;
            std::string func;
            StackVar() {}
            StackVar(int _p_min, int _p_max, std::string _f): pos_min(_p_min), pos_max(_p_max), func(std::move(_f)) {}
        };

        std::map<std::string, Variable> globalVars;
        std::map<std::string, int> funcStack;
        std::map<std::string, StackVar> varStack;
    public:
        TiggerGenerator() {
            v_num = 0;
            currentFunc = "";
            reg_num = 1;
        }

        std::string op2char(Operator op);

        std::string GenDecl(DeclIR &decl, std::string &code);

        std::string GenInit(InitIR &init, std::string &code);

        std::string GenFuncDef(FuncDefIR &funcDef, std::string &code);

        std::string GenStatements(StatementsIR &stmts, std::string &code);

        std::string GenBinaryExp(BinaryExpIR &binary, std::string &code);

        std::string GenUnaryExp(UnaryExpIR &unary, std::string &code);

        std::string GenAssign(AssignIR &assign, std::string &code);

        std::string GenCondGoto(CondGotoIR &cond, std::string &code);

        std::string GenLVal(LValIR &lval, std::string &code);

        std::string GenGoto(GotoIR &gt, std::string &code);

        std::string GenLabel(Label &label, std::string &code);

        std::string GenParamList(ParamListIR &params, std::string &code);

        std::string GenFuncCall(FuncCallIR &funcCall, std::string &code);

        std::string GenReturn(ReturnIR &ret, std::string &code);

        std::string GenRightVal(RightValIR &rightval, std::string &code);

        std::string GenProgram(ProgramIR &program, std::string &code);
    };
}

#endif