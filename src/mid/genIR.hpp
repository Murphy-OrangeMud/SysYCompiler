#ifndef GENIR_INCLUDED
#define GENIR_INCLUDED

#include <iostream>
#include <map>
#include "../define/ast.hpp"
#include "../front/logger.hpp"

class IRGenerator {
private:
    int t_num;
    int T_num;
    int l_if_num;
    int l_while_num;
    std::string tab;
    // 所有全局符号
    std::map<std::string, int> SymbolTable;
    std::vector <std::pair<std::string, VarType>> ReverseSymbolTable;
    std::string currentFunc;
    std::map <std::string, Type> FuncTable;
    std::map <std::string, std::map<std::string, std::string> /*编号*/> FuncVarTable;
    std::map <std::string, std::vector<int> /*dims*/> ArrayTable;
    std::map <std::string, std::map<std::string, std::pair<std::string, /*编号*/ std::vector < int> /*dims*/>>> FuncArrayTable;

    Logger logger;
public:
    IRGenerator(std::map <std::string, std::vector<int>> _table1,
                std::map <std::string, std::map<std::string, std::vector < int>>

    > _table2, const std::string& i):

    ArrayTable (std::move(_table1)) {
        for (auto & iter1 : _table2) {
            for (auto iter2 = iter1.second.begin(); iter2 != iter1.second.end(); iter2++) {
                FuncArrayTable[iter1.first][iter2->first].second = iter2->second;
            }
        }
        t_num = 0;
        T_num = 0;
        l_if_num = 0;
        l_while_num = 0;
        std::string path = R"(../../logs/log_generator_)" + i;
        logger = Logger(path);
        FuncTable["getint"] = Type::INT;
        FuncTable["getch"] = Type::INT;
        FuncTable["getarray"] = Type::INT;
        FuncTable["putint"] = Type::VOID;
        FuncTable["putch"] = Type::VOID;
        FuncTable["putarray"] = Type::VOID;
        // std::cout << (FuncArrayTable["main"].find("a") == FuncArrayTable["main"].end()) << std::endl;
    }

    void GenerateValue(const std::string& varName, int &idx, InitValAST *init, std::vector<int> dim, int i, std::string &code);

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