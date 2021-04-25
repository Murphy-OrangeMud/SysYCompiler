#ifndef GENIR_INCLUDED
#define GENIR_INCLUDED

#include <iostream>
#include <map>
#include "../define/ast.hpp"

class IRGenerator {
private:
    int t_num;
    int T_num;
    int l_num;
    std::string tab;
    std::map<std::string, int> SymbolTable;
    std::vector <std::pair<std::string, VarType>> ReverseSymbolTable;
    std::string currentFunc;
    std::map <std::string, std::map<std::string, int>> FuncArgTable;
    std::map <std::string, std::map<std::string, int> /*编号*/> FuncVarTable;
    std::map <std::string, std::vector<int> /*dims*/> ArrayTable;
    std::map <std::string, std::map<std::string, std::vector < int> /*dims*/>> FuncArrayTable;
public:
    IRGenerator(std::map <std::string, std::vector<int>> _table1,
                std::map <std::string, std::map<std::string, std::vector < int>>

    > _table2):

    ArrayTable (std::move(_table1)), FuncArrayTable(std::move(_table2)) {
        t_num = 0;
        T_num = 0;
        l_num = 0;
    }

    void GenerateValue(int idx, InitValAST *init, std::vector<int> dim, int i, std::string &code);

    void GenVarDecl(VarDeclAST &varDecl, std::string &code);

    std::string GenId(ProcessedIdAST &id, std::string &code);

    std::string GenNumber(NumberAST &num, std::string &code);

    std::string GenVarDef(VarDefAST &varDef, std::string &code);

    std::string GenAssign(AssignAST &assign, std::string &code);

    std::string GenBinaryExp(BinaryExpAST &exp, std::string &code);

    std::string GenInitVal(ProcessedInitValAST &init, std::string &code);

    static std::string op2char(Operator op);

    void GenBlock(BlockAST &block, std::string &code);

    void GenFuncCall(FuncCallAST &func, std::string &code);

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