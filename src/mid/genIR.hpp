#ifndef GENIR_INCLUDED
#define GENIR_INCLUDED

#include "../define/ast.hpp"
#include <iostream>
#include <map>
#include <stack>

class IRGenerator {
private:
    int t_num;
    int T_num;
    int l_num;
    std::map<std::string, int> SymbolTable;
    std::vector<std::pair<std::string, VarType>> ReverseSymbolTable;
    std::string currentFunc;
    std::map<std::string, std::map<std::string, int>> FuncArgTable;
    std::map<std::string, std::map<std::string, int> /*编号*/> FuncVarTable;
    std::map<std::string, std::vector<int> /*dims*/> ArrayTable;
    std::map<std::string, std::map<std::string, std::vector<int> /*dims*/>> FuncArrayTable;
public:
    // TODO: initialize ArrayTable using Typecheck's ArrayTable, ConstArrayTable, FuncArrayTable and FuncConstArrayTable, in main.cpp
    IRGenerator(std::map<std::string, std::vector<int>> _table): ArrayTable(_table) {
        t_num = 0;
        T_num = 0;
        l_num = 0;
    }

    void GenerateValue(int idx, InitValAST &init, std::vector<int> &dim, int i);

    void GenVarDecl(VarDeclAST &varDecl);

    std::string GenId(ProcessedIdAST &id);

    std::string GenNumber(NumberAST &num);

    std::string GenVarDef(VarDefAST &varDef);

    std::string GenAssign(AssignAST &assign);

    std::string GenBinaryExp(BinaryExpAST &exp);

    std::string GenInitVal(InitValAST &init);

    std::string op2char(Operator op);

    void GenBlock(BlockAST &block);

    void GenFuncCall(FuncCallAST &func);

    std::string GenLVal(LValAST &lval);

    std::string GenUnaryExp(BinaryExpAST &exp);

    std::string GenFuncDef(FuncDefAST &funcDef);

    void GenCompUnit(CompUnitAST &unit);

    void GenIfElse(IfElseAST &stmt);

    void GenWhile(WhileAST &stmt);

    void GenControl(ControlAST &stmt);

    void GenStmt(StmtAST &stmt);
};

#endif