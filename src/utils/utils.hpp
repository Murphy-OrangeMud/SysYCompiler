#ifndef UTIL_INCLUDED
#define UTIL_INCLUDED

#include "../define/ast.hpp"
#include "../define/token.hpp"

struct Seg {
    int begin;
    int end;
    Seg() {
        begin = 0;
        end = 2e9;
    }
    Seg(int _b, int _e=2e9): begin(_b), end(_e) {}
    Seg(int _e, int _b=0): begin(_b), end(_e) {}
    bool operator < (const Seg & other) const {
        if (begin < other.begin) return true;
        else if (begin == other.begin) {
            return end < other.end;
        }
        return false;
    }
};


struct GenVar {
    std::string name;
    VarType argType;
    std::vector<int> dims;
    std::string id;

    GenVar() {}
    GenVar(std::string _n, VarType _t, std::vector<int> _d = std::vector<int>{}, std::string _id = "")
            : name(std::move(_n)), argType(_t), dims(std::move(_d)), id(std::move(_id)) {}
};

struct Var {
    std::string name;
    VarType type;
    bool isConst;
    std::vector<int> dims;
    int val;

    Var() {}
    Var(std::string _n, VarType _t, bool _c, std::vector<int> _d = std::vector<int>{}, int _v = 0) : name(
            std::move(_n)), type(_t), isConst(_c), dims(std::move(_d)), val(_v) {}
};

struct Function {
    std::string funcName;
    Type funcType;
    std::vector<Var> argTable;

    Function() {}
    Function(std::string _n, Type _t, std::vector<Var> _a) : funcName(std::move(_n)), funcType(_t),
                                                             argTable(std::move(_a)) {}
};

#endif