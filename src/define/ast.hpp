#ifndef AST_INCLUDED
#define AST_INCLUDED

#include <iostream>
#include <vector>
#include <memory>
#include <utility>
#include <optional>
#include <string>
#include "token.hpp"
#include "../mid/typeck.hpp"

class TypeCheck;
class IRGenerator;

class BaseAST {
    public:
    virtual ~BaseAST() = default;
    virtual std::optional<int> Eval(Interpreter &intp) const = 0;
    virtual ValPtr GenerateIR(IRGenerator &gen) const = 0; 
};

using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrList = std::vector<ASTPtr>;

class FuncDefAST: public BaseAST {
private:
    Type type;
    std::string name;
    ASTPtrList args;
    ASTPtr body;
public:
    FuncDefAST(Type _type, const std::string &_name, ASTPtrList _args, ASTPtr _body)
    :type(_type), name(_name), args(_args), body(std::move(_body)) {}

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalFuncDef(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;

    const Type &getType() const { return type; }
    const std::string &getName() const { return name; }
    const ASTPtrList &getArgs() const { return args; }
    const ASTPtr &getBody() const { return body; }
};

class BlockAST: public BaseAST {
private:
    ASTPtrList stmts;
public:
    BlockAST(ASTPtrList _stmts): stmts(std::move(_stmts)) {};

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalBlock(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;

    const ASTPtrList &getStmts() const { return stmts; };
};

class BinaryExpAST: public BaseAST {
public:
    BinaryExpAST(Operator _opcode, ASTPtr _left, ASTPtr _right)
    : op(_opcode), left(std::move(_left)), right(std::move(_right)) {}

    std::optional<int> Eval(TypeCheck &checker) const override {
        std::initializer_list<Operator> opAdd = {Operator::ADD, Operator::SUB};
        std::initializer_list<Operator> opMul = {Operator::MUL, Operator::DIV, Operator::MOD};
        std::initializer_list<Operator> opRel = {Operator::LE, Operator::GE, Operator::LT, Operator::LE};
        std::initializer_list<Operator> opEq = {Operator::EQ, Operator::NEQ};
        std::initializer_list<Operator> opLAnd = {Operator::AND};
        std::initializer_list<Operator> opLOr = {Operator::OR};

        if (std::find(opLOr.begin(), opLOr.end(), op)) {
            return checker.EvalLOrExp(*this);
        }
        else if (std::find(opLAnd.begin(), opLAnd.end(), op)) {
            return checker.EvalLAndExp(*this);
        }
        else if (std::find(opEq.begin(), opEq.end(), op)) {
            return checker.EvalEqExp(*this);
        }
        else if (std::find(opRel.begin(), opRel.end(), op)) {
            return checker.EvalRelExp(*this);
        }
        else if (std::find(opAdd.begin(), opAdd.end(), op)) {
            return checker.EvalAddExp(*this);
        }
        else if (std::find(opMul.begin(), opMul.end(), op)) {
            return checker.EvalMulExp(*this);
        }
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;

    const Operator &getOp() const { return op; }
    const ASTPtr &getLHS() const { return left; }
    const ASTPtr &getRHS() const { return right; }
private:
    Operator op;
    ASTPtr left;
    ASTPtr right;
};

class IfElseAST: public BaseAST {
private:
    ASTPtr cond;
    ASTPtr thenStmt;
    ASTPtr elseStmt;

public:
    IfElseAST(ASTPtr _cond, ASTPtr _then, ASTPtr _else=nullptr)
    : cond(std::move(_cond)), thenStmt(std::move(_then)), elseStmt(std::move(_else)) {}

    std::optional<int> Eval(Interpreter &intp) const override;
    ValPtr GenerateIR(IRGenerator &gen) const override;

    const ASTPtr &getCond() const { return cond; }
    const ASTPtr &getThenStmt() const { return thenStmt; }
    const ASTPtr &getElseStmt() const { return elseStmt; }
};

class WhileAST: public BaseAST {
private:
    ASTPtr cond;
    ASTPtr stmt;

public:
    WhileAST(ASTPtr _cond, ASTPtr _stmt)
    : cond(std::move(_cond)), stmt(std::move(_stmt)) {}

    std::optional<int> Eval(Interpreter &intp) const override;
    ValPtr GenerateIR(IRGenerator &gen) const override;

    const ASTPtr &getCond() const { return cond; }
    const ASTPtr &getStmt() const { return stmt; }
};

class NumberAST: public BaseAST {
private:
    long long value;
public:
    NumberAST(long long _val): value(_val) {}

    std::optional<int> Eval(Interpreter &intp) const override;
    ValPtr GenerateIR(IRGenerator &gen) const override;

    const long long &getVal() const { return value; }
};

class ProcessedIdAST: public BaseAST {
private:
    std::string name;
    VarType type;
    std::vector<int> dim; // exps: binaryExp, unaryExp
    bool Const;
public:
    ProcessedIdAST(std::string _name, VarType _type, bool _const=false, std::vector<int> _dim=std::vector<int>{}):
    Const(_const), name(std::move(_name)), type(_type), dim(std::move(_dim)) {}

    std::optional<int> Eval(Interpreter &intp) const override;
    ValPtr GenerateIR(IRGenerator &gen) const override;

    const std::string &getName() const { return std::move(name); }
    const VarType &getType() const { return type; }
    const std::vector<int> &getDim() const { return dim; }
    const bool isConst() const { return Const; }
};

class IdAST: public BaseAST {
private:
    std::string name;
    VarType type;
    ASTPtrList dim; // exps: binaryExp, unaryExp
    bool Const;
public:
     IdAST(std::string _name, VarType _type, bool _const=false, ASTPtrList _dim=ASTPtrList{}):
     Const(_const), name(std::move(_name)), type(_type), dim(std::move(_dim)) {}

     std::optional<int> Eval(Interpreter &intp) const override;
     ValPtr GenerateIR(IRGenerator &gen) const override;

     const std::string &getName() const { return std::move(name); }
     const VarType getType() const { return type; }
     const ASTPtrList &getDim() const { return dim; }
     const bool isConst() const { return Const; }
};

class UnaryExpAST: public BaseAST {
private:
    ASTPtr unaryExp;
    Operator op;
public:
    UnaryExpAST(ASTPtr unary, Operator _op=Operator::NONE):
    unaryExp(std::move(unary)), op(_op) {}
    const ASTPtr &getNode() const { return unaryExp; }
    const Operator getOp() const { return op; }

    std::optional<int> Eval(Interpreter &intp) const override;
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class ControlAST: public BaseAST {
private:
    enum class Control { Continue, Break, Return };
    Control controlType;
    ASTPtr returnExp;
public:
    ControlAST(Token controlToken, ASTPtr _return=nullptr): returnExp(std::move(_return)) {
        switch (controlToken) {
            case Token::CONTINUE:{
                controlType = Control::Continue;
                break;
            }
            case Token::BREAK: {
                controlType = Control::Break;
                break;
            }
            case Token::RETURN:{
                controlType = Control::Return;
                break;
            }
            default:break;
        }
    }

    const Token getControl() {
        switch (controlType) {
            case Control::Continue:
                return Token::CONTINUE;
            case Control::Return:
                return Token::RETURN;
            case Control::Break:
                return Token::BREAK;
        }
    }

    const ASTPtr &getReturnExp() const { return returnExp; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalControl(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class AssignAST: public BaseAST {
private:
    ASTPtr left;
    ASTPtr right;
public:
    AssignAST(ASTPtr _left, ASTPtr _right): left(std::move(_left)), right(std::move(_right)) {}

    const ASTPtr &getLeft() const { return left; }
    const ASTPtr &getRight() const { return right; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        // return checker
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class StmtAST: public BaseAST {
private:
    ASTPtr stmt;
    // type
public:
    StmtAST(ASTPtr _stmt): stmt(std::move(_stmt)) {}

    const ASTPtr &getStmt() const { return stmt; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalStmt(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class LValAST: public BaseAST {
private:
    std::string name;
    VarType type;
    ASTPtrList position;
public:
    LValAST(const std::string _name, VarType _type, ASTPtrList _pos=ASTPtrList{}):
    name(std::move(_name)), type(_type), position(std::move(_pos)) {}

    const ASTPtrList &getPosition() const { return position; }
    const std::string &getName() const { return name; }
    const VarType getType() const { return type; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalLVal(*this);
    };
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class FuncCallAST: public BaseAST {
private:
    std::string name;
    ASTPtrList args;
public:
    FuncCallAST(const std::string _name, ASTPtrList _args=ASTPtrList{}):
    name(std::move(_name)), args(std::move(_args)) {}

    const std::string &getName() const { return name; }
    const ASTPtrList &getArgs() const { return args; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalFuncCall(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class VarDeclAST: public BaseAST {
private:
    ASTPtrList varDefs;
    bool Const;
public:
    VarDeclAST(bool _isConst, ASTPtrList &list): Const(_isConst), varDefs(std::move(list)) {}

    const ASTPtrList &getVarDefs() const { return varDefs; }
    const bool isConst() const { return Const; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalVarDecl(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class VarDefAST: public BaseAST {
private:
    ASTPtr var;
    ASTPtr init; // can be nullptr;
    bool Const;
public:
    VarDefAST(bool Const_, ASTPtr _lhs, ASTPtr _rhs= nullptr):
    Const(Const_), var(std::move(_lhs)), init(std::move(_rhs)) {}

    const ASTPtr &getVar() const { return var; }
    const ASTPtr &getInitVal() const { return init; }
    const bool isConst() const { return Const; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalVarDef(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class InitValAST: public BaseAST {
private:
    VarType type;
    ASTPtrList values;
public:
    InitValAST(VarType _type, ASTPtrList list): type(_type), values(std::move(list)) {}

    const VarType getType() const { return type; }
    const ASTPtrList &getValues() const { return values; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalInitVal(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

class CompUnitAST: public BaseAST {
private:
    ASTPtrList nodes;
public:
    CompUnitAST(ASTPtrList &_nodes): nodes(std::move(_nodes)) {}

    const ASTPtrList &getNodes() const { return nodes; }

    std::optional<int> Eval(TypeCheck &checker) const override {
        return checker.EvalCompUnit(*this);
    }
    ValPtr GenerateIR(IRGenerator &gen) const override;
};

#endif