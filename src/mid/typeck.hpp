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

    bool FillInValue(int *memory, InitValAST &init, std::vector<int> &dim, int i);

    std::unique_ptr <VarDeclAST> EvalVarDecl(VarDeclAST &varDecl);

    std::unique_ptr <ProcessedIdAST> EvalId(IdAST &id);

    std::unique_ptr <VarDefAST> EvalVarDef(VarDefAST &varDef);

    std::unique_ptr <FuncCallAST> EvalFuncCall(FuncCallAST &func);

    std::unique_ptr <BlockAST> EvalBlock(BlockAST &block);

    std::unique_ptr <IfElseAST> EvalIfElse(IfElseAST &stmt);

    std::unique_ptr<WhileAST> EvalWhile(WhileAST &stmt);

    std::unique_ptr<ControlAST> EvalControl(ControlAST &stmt);

    std::unique_ptr<AssignAST> EvalAssign(AssignAST &assign);

    ASTPtr EvalLVal(LValAST &lval);

    ASTPtr EvalAddExp(BinaryExpAST &exp);

    ASTPtr EvalMulExp(BinaryExpAST &exp);

    ASTPtr EvalUnaryExp(UnaryExpAST &exp);

    std::unique_ptr<FuncDefAST> EvalFuncDef(FuncDefAST &funcDef);

    ASTPtr EvalRelExp(BinaryExpAST &exp);

    ASTPtr EvalLAndExp(BinaryExpAST &exp);

    ASTPtr EvalLOrExp(BinaryExpAST &exp);

    std::unique_ptr<CompUnitAST> EvalCompUnit(CompUnitAST &unit);

    ASTPtr EvalEqExp(BinaryExpAST &exp);

    std::unique_ptr<StmtAST> EvalStmt(StmtAST &stmt);

    std::unique_ptr<InitValAST> EvalInitVal(InitValAST &init);
};

#endif