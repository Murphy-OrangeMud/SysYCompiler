#ifndef UTIL_INCLUDED
#define UTIL_INCLUDED

#include "../define/ast.hpp"
#include "../define/token.hpp"


struct GenVar {
    std::string name;
    VarType argType;
    std::vector<int> dims;
    std::string id;

    GenVar(std::string _n, VarType _t, std::vector<int> _d = std::vector<int>{}, std::string _id = "")
            : name(std::move(_n)), argType(_t), dims(std::move(_d)), id(std::move(_id)) {}
};

struct Var {
    std::string name;
    VarType argType;
    std::vector<int> dims;
    bool isConst;
    int val;

    Var(std::string _n, VarType _t, bool _c, std::vector<int> _d = std::vector<int>{}, int _v = 0) : name(
            std::move(_n)), argType(_t), isConst(_c), dims(std::move(_d)), val(_v) {}
};

struct Function {
    std::string funcName;
    Type funcType;
    std::vector<Var> argTable;

    Function(std::string _n, Type _t, std::vector<Var> _a) : funcName(std::move(_n)), funcType(_t),
                                                             argTable(std::move(_a)) {}
};

#endif