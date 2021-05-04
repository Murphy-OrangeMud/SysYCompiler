#ifndef TYPECHECK_INCLUDED
#define TYPECHECK_INCLUDED

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "../define/token.hpp"
#include "../front/logger.hpp"
#include "../define/ast.hpp"
#include "utils.hpp"
#include "../oj.hpp"

class TypeCheck {
private:
    Logger logger;
    int currentBlock;
    std::string currentFunc;
public:

    std::vector<int> parentBlock;

    std::map<std::string, Function> FuncTable;
    std::map<int, std::map<std::string, Var>> BlockVars;

    TypeCheck(const std::string &i="") {
#ifndef OJ
        std::string path = R"(../../logs/log_checker_)" + i;
        logger = Logger(path);
#else
        logger = Logger();
#endif
        currentFunc = "";
        currentBlock = 0;

        FuncTable["getint"] = Function("getint", Type::INT, std::vector<Var>{});
        FuncTable["getch"] = Function("getch", Type::INT, std::vector<Var>{});
        FuncTable["getarray"] = Function("getarray", Type::INT,
                                          std::vector<Var>{Var("a", VarType::ARRAY, false, std::vector<int>{0})});
        FuncTable["putint"] = Function("putint", Type::VOID, std::vector<Var>{Var("a", VarType::VAR, false)});
        FuncTable["putch"] = Function("putch", Type::VOID, std::vector<Var>{Var("a", VarType::VAR, false)});
        FuncTable["putarray"] = Function("putarray", Type::VOID, std::vector<Var>{Var("a", VarType::VAR, false),
                                                                                  Var("b", VarType::ARRAY, false,
                                                                                      std::vector<int>{0})});
    }

    ~TypeCheck() = default;

    bool FillInValue(int *memory, InitValAST *init, std::vector<int> &dim, size_t i);

    std::unique_ptr<VarDeclAST> EvalVarDecl(VarDeclAST &varDecl);

    std::unique_ptr<ProcessedIdAST> EvalId(IdAST &id);

    std::unique_ptr<VarDefAST> EvalVarDef(VarDefAST &varDef);

    std::unique_ptr<FuncCallAST> EvalFuncCall(FuncCallAST &func);

    std::unique_ptr<BlockAST> EvalBlock(BlockAST &block);

    std::unique_ptr<IfElseAST> EvalIfElse(IfElseAST &stmt);

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

    std::unique_ptr<NumberAST> EvalNumber(NumberAST &num);
};

#endif