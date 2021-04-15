#ifndef TYPECHECK_INCLUDED
#define TYPECHECK_INCLUDED

#include <map>
#include <set>
#include <string>
#include <vector>
#include "../define/ast.hpp"
#include "../define/token.hpp"
#include "../front/logger.hpp"

class TypeCheck {
private:
    Logger logger;
    std::string currentFunc;
public:
    std::set<std::string> VarTable;
    std::map<std::string, int> ConstVarTable;
    std::map<std::string, std::vector<int>/*dim*/> ArrayTable;
    std::map<std::string, std::pair<std::vector<int>/*dim*/, int*>> ConstArrayTable;

    std::set<std::string> FuncTable;
    std::map<std::string, std::set<std::string>> FuncVarTable;
    std::map<std::string, std::map<std::string, int>> FuncConstVarTable;
    std::map<std::string, std::map<std::string, std::vector<int>>> FuncArrayTable;
    std::map<std::string, std::map<std::string, std::pair<std::vector<int>, int*>>> FuncConstArrayTable;

    TypeCheck() {
        std::string path = "..\\..\\logs\\log_" + std::to_string(rand()%10000);
        logger = Logger(path);
        currentFunc = "";
    }

    ~TypeCheck() {}

    void CheckVarDecl();
    void CheckVarDef();
};

#endif