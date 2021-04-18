#ifndef TYPECHECK_INCLUDED
#define TYPECHECK_INCLUDED

#include <map>
#include <set>
#include <string>
#include <vector>
#include "../define/ast.hpp"
#include "../define/token.hpp"
#include "../front/logger.hpp"

struct Arg {
    VarType type;
    vector<int> dim;
};

class TypeCheck {
private:
    Logger logger;
    std::string currentFunc;
public:
    std::map<std::string, int> VarTable;
    std::map<std::string, int> ConstVarTable;
    std::map<std::string, std::pair<std::vector <int>/*dim*/, int *>> ArrayTable;
    std::map<std::string, std::pair<std::vector <int>/*dim*/, int *>> ConstArrayTable;

    std::map <std::string, std::pair<Type,
            std::vector < std::pair < std::string, std::pair < VarType, std::vector < int>/*dims*/>>>/* Arg table */>>
    FuncTable;
    std::map <std::string, std::set<std::string>> FuncVarTable;
    std::map <std::string, std::map<std::string, int>> FuncConstVarTable;
    std::map <std::string, std::map<std::string, std::vector < int>>>
    FuncArrayTable;
    std::map<std::string, std::map<std::string, std::pair < std::vector < int>, int *>>>
    FuncConstArrayTable;

    TypeCheck() {
        std::string path = "..\\..\\logs\\log_" + std::to_string(rand() % 10000);
        logger = Logger(path);
        currentFunc = "";
    }

    ~TypeCheck() {}

    void FillInValue(int *memory, InitValAST &init, std::vector<int> &dim, int i);

    ASTPtr EvalVarDecl(VarDeclAST &varDecl);

    ASTPtr EvalId(IdAST &id);

    ASTPtr EvalVarDef(VarDefAST &varDef);

    ASTPtr EvalFuncCall(FuncCallAST func);

    ASTPtr EvalBlock(BlockAST block);

    ASTPtr EvalIfElse(IfElseAST stmt);

    ASTPtr EvalWhile(WhileAST stmt);

    ASTPtr EvalControl(ControlAST stmt);

    ASTPtr EvalLVal(LValAST lval);

    ASTPtr EvalAddExp(BinaryExpAST exp);

    ASTPtr EvalMulExp(BinaryExpAST exp);

    ASTPtr EvalUnaryExp(UnaryExpAST exp);

    ASTPtr EvalFuncDef(FuncDefAST funcDef);

    ASTPtr EvalRelExp(BinaryExpAST exp);

    ASTPtr EvalLAndExp(BinaryExpAST exp);

    ASTPtr EvalLOrExp(BinaryExpAST exp);

    ASTPtr EvalCompUnit(CompUnitAST unit);

    ASTPtr EvalEqExp(BinaryExpAST exp);

    ASTPtr EvalStmt(StmtAST stmt);

    ASTPtr EvalInitVal(InitValAST init);
};

#endif