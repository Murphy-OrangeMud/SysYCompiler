#ifndef GENIR_INCLUDED
#define GENIR_INCLUDED

#include <iostream>
#include <map>
#include <utility>
#include "../define/ast.hpp"
#include "../front/logger.hpp"
#include "utils.hpp"
#include "../oj.hpp"

class IRGenerator {
private:
    int t_num;
    int T_num;
    int l_num;
    int cur_break_l;
    int cur_continue_l;
    Logger logger;
    int currentDepth;
    int currentBlock;
    std::string currentFunc;

    std::map<int, std::map<std::string, GenVar>> BlockSymbolTable;
    std::vector<int> parentBlock;
    std::map<std::string, Function> FuncTable;
    std::vector<GenVar> ReverseSymbolTable;

public:
    IRGenerator(std::map<std::string, Function> __FuncTable, const std::map<int, std::map<std::string, Var>>& BlockVars, std::string i="") :FuncTable(std::move(__FuncTable)) {
        for (auto &item1 : BlockVars) {
            for (auto &item2 : item1.second) {
                if (item2.second.isConst && item2.second.type == VarType::VAR) continue;
                BlockSymbolTable[item1.first][item2.first] = GenVar(item2.second.name, item2.second.type, item2.second.dims);
            }
        }

        t_num = 0;
        T_num = 0;
        l_num = 0;
        cur_break_l = -1;
        cur_continue_l = -1;
        currentDepth = 0;
        currentBlock = 0;
        currentFunc = "";
#ifndef OJ
        std::string path = R"(../../logs/log_generator_)" + i;
        logger = Logger(path);
#else
        logger = Logger();
#endif
    }

    void GenerateValue(const std::string &varName, int &idx, InitValAST *init, std::vector<int> dim, int i,
                       std::string &code);

    void GenVarDecl(VarDeclAST &varDecl, std::string &code);

    std::string GenId(ProcessedIdAST &id, std::string &code);

    std::string GenNumber(NumberAST &num, std::string &code);

    std::string GenVarDef(VarDefAST &varDef, std::string &code);

    std::string GenAssign(AssignAST &assign, std::string &code);

    std::string GenBinaryExp(BinaryExpAST &exp, std::string &code);

    std::string GenInitVal(InitValAST &init, std::string &code);

    static std::string op2char(Operator op);

    void GenBlock(BlockAST &block, std::string &code);

    std::string GenFuncCall(FuncCallAST &func, std::string &code);

    std::string GenLVal(LValAST &lval, std::string &code);

    std::string GenUnaryExp(UnaryExpAST &exp, std::string &code);

    std::string GenLAndExp(BinaryExpAST &exp, std::string &code);

    std::string GenLOrExp(BinaryExpAST &exp, std::string &code);

    void GenFuncDef(FuncDefAST &funcDef, std::string &code);

    void GenCompUnit(CompUnitAST &unit, std::string &code);

    void GenIfElse(IfElseAST &stmt, std::string &code);

    void GenWhile(WhileAST &stmt, std::string &code);

    void GenControl(ControlAST &stmt, std::string &code);

    void GenStmt(StmtAST &stmt, std::string &code);
};

#endif