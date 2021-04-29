#ifndef GENIR_INCLUDED
#define GENIR_INCLUDED

#include <iostream>
#include <map>
#include <utility>
#include "../define/ast.hpp"
#include "../front/logger.hpp"
#include "utils.hpp"

class IRGenerator {
private:
    int t_num;
    int T_num;
    int l_if_num;
    Logger logger;
    int currentBlock;
    std::string currentFunc;

    std::map<int, std::map<std::string, GenVar>> BlockSymbolTable;
    std::vector<int> parentBLock;
    std::map<std::string, Function> FuncTable;
    std::vector<GenVar> ReverseSymbolTable;

    /*
    std::map<std::string, int> SymbolTable;
    std::vector<std::pair<std::string, VarType>> ReverseSymbolTable;

    std::map<std::string, std::map<std::string, std::string>> FuncVarTable;
    std::map<std::string, std::vector<int>> ArrayTable;
    std::map<std::string, std::map<std::string, std::pair<std::string, std::vector<int>>>> FuncArrayTable;
     */


public:
    IRGenerator(std::string &i, std::vector<int> _pB, std::map<std::string, Function> __FuncTable, const std::map<int, std::map<std::string, Var>>& BlockVars) :parentBLock(std::move(_pB)), FuncTable(std::move(__FuncTable)) {
        for (auto &item1 : BlockVars) {
            for (auto &item2 : item1.second) {
                if (item2.second.isConst && item2.second.argType == VarType::VAR) continue;
                BlockSymbolTable[item1.first][item2.first] = GenVar(item2.second.name, item2.second.argType, item2.second.dims);
            }
        }

        t_num = 0;
        T_num = 0;
        l_if_num = 0;
        std::string path = R"(../../logs/log_generator_)" + i;
        logger = Logger(path);
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

    void GenFuncDef(FuncDefAST &funcDef, std::string &code);

    void GenCompUnit(CompUnitAST &unit, std::string &code);

    void GenIfElse(IfElseAST &stmt, std::string &code);

    void GenWhile(WhileAST &stmt, std::string &code);

    void GenControl(ControlAST &stmt, std::string &code);

    void GenStmt(StmtAST &stmt, std::string &code);
};

#endif