#ifndef AST_INCLUDED
#define AST_INCLUDED

#include <iostream>
#include <utility>
#include <vector>
#include <memory>
#include <utility>
#include <optional>
#include <string>
#include "token.hpp"

class TypeCheck;
class IRGenerator;
class BaseAST;

using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrList = std::vector<ASTPtr>;

class BaseAST {
    public:
    virtual ~BaseAST() = default;
    virtual ASTPtr Eval(TypeCheck &checker) = 0;
    virtual std::string GenerateIR(IRGenerator &gen, std::string &code) = 0;
};

class FuncDefAST: public BaseAST {
private:
    Type type;
    std::string name;
    ASTPtrList args;
    ASTPtr body;
public:
    FuncDefAST(Type _type, const std::string &_name, ASTPtrList _args, ASTPtr _body)
    :type(_type), name(_name), args(std::move(_args)), body(std::move(_body)) {}

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;

    const Type getType() const { return type; }
    const std::string &getName() const { return name; }
    const ASTPtrList &getArgs() const { return args; }
    const ASTPtr &getBody() const { return body; }
};

class BlockAST: public BaseAST {
private:
    ASTPtrList stmts;
public:
    explicit BlockAST(ASTPtrList _stmts): stmts(std::move(_stmts)) {};

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;

    const ASTPtrList &getStmts() const { return stmts; };
};

class BinaryExpAST: public BaseAST {
public:
    BinaryExpAST(Operator _opcode, ASTPtr _left, ASTPtr _right)
    : op(_opcode), left(std::move(_left)), right(std::move(_right)) {}

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;

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

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;

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

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;

    const ASTPtr &getCond() const { return cond; }
    const ASTPtr &getStmt() const { return stmt; }
};

class NumberAST: public BaseAST {
private:
    int value;
public:
    explicit NumberAST(int _val): value(_val) {}

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;

    const int &getVal() const { return value; }
};

class IdAST: public BaseAST {
private:
    bool Const;
    std::string name;
    VarType type;
    ASTPtrList dim; // exps: binaryExp, unaryExp

public:
     IdAST(std::string _name, VarType _type, bool _const=false, ASTPtrList _dim=ASTPtrList{}):
     Const(_const), name(std::move(_name)), type(_type), dim(std::move(_dim)) {}

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;

     const std::string getName() const { return name; }
     const VarType getType() const { return type; }
     const ASTPtrList &getDim() const { return dim; }
     const bool isConst() const { return Const; }
};

class ProcessedIdAST: public BaseAST {
private:
    bool Const;
    std::string name;
    VarType type;
    std::vector<int> dim; // exps: binaryExp, unaryExp
public:
    ProcessedIdAST(std::string _name, VarType _type, bool _const=false, std::vector<int> _dim=std::vector<int>{}):
            Const(_const), name(std::move(_name)), type(_type), dim(std::move(_dim)) {}

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;

    const std::string &getName() const { return std::move(name); }
    const VarType &getType() const { return type; }
    const std::vector<int> &getDim() const { return dim; }
    const bool isConst() const { return Const; }
};

class UnaryExpAST: public BaseAST {
private:
    ASTPtr unaryExp;
    Operator op;
public:
    explicit UnaryExpAST(ASTPtr unary, Operator _op=Operator::NONE):
    unaryExp(std::move(unary)), op(_op) {}
    const ASTPtr &getNode() const { return unaryExp; }
    const Operator getOp() const { return op; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class ControlAST: public BaseAST {
private:
    enum class Control { Continue, Break, Return };
    Control controlType;
    ASTPtr returnExp;
public:
    explicit ControlAST(Token controlToken, ASTPtr _return=nullptr): returnExp(std::move(_return)) {
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

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class AssignAST: public BaseAST {
private:
    ASTPtr left;
    ASTPtr right;
public:
    AssignAST(ASTPtr _left, ASTPtr _right): left(std::move(_left)), right(std::move(_right)) {}

    const ASTPtr &getLeft() const { return left; }
    const ASTPtr &getRight() const { return right; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class StmtAST: public BaseAST {
private:
    ASTPtr stmt;
    // type
public:
    explicit StmtAST(ASTPtr _stmt): stmt(std::move(_stmt)) {}

    const ASTPtr &getStmt() const { return stmt; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class LValAST: public BaseAST {
private:
    std::string name;
    VarType type;
    ASTPtrList position;
public:
    LValAST(const std::string& _name, VarType _type, ASTPtrList _pos=ASTPtrList{}):
    name(_name), type(_type), position(std::move(_pos)) {}

    const ASTPtrList &getPosition() const { return position; }
    const std::string &getName() const { return name; }
    const VarType getType() const { return type; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class FuncCallAST: public BaseAST {
private:
    std::string name;
    ASTPtrList args;
public:
    explicit FuncCallAST(const std::string& _name, ASTPtrList _args=ASTPtrList{}):
    name(_name), args(std::move(_args)) {}

    const std::string &getName() const { return name; }
    const ASTPtrList &getArgs() const { return args; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class VarDeclAST: public BaseAST {
private:
    ASTPtrList varDefs;
    bool Const;
public:
    VarDeclAST(bool _isConst, ASTPtrList list): varDefs(std::move(list)), Const(_isConst) {}

    const ASTPtrList &getVarDefs() const { return varDefs; }
    const bool isConst() const { return Const; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class VarDefAST: public BaseAST {
private:
    ASTPtr var;
    ASTPtr init; // can be nullptr;
    bool Const;
public:
    VarDefAST(bool Const_, ASTPtr _lhs, ASTPtr _rhs= nullptr):
    var(std::move(_lhs)), init(std::move(_rhs)), Const(Const_) {}

    const ASTPtr &getVar() const { return var; }
    const ASTPtr &getInitVal() const { return init; }
    const bool isConst() const { return Const; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class InitValAST: public BaseAST {
private:
    VarType type;
    ASTPtrList values;
public:
    InitValAST(VarType _type, ASTPtrList list): type(_type), values(std::move(list)) {}

    const VarType getType() const { return type; }
    const ASTPtrList &getValues() const { return values; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class ProcessedInitValAST: public InitValAST {
private:
    std::vector<int> dims;
public:
    ProcessedInitValAST(VarType _type, ASTPtrList list, std::vector<int> _dims): InitValAST(_type, std::move(list)), dims(std::move(_dims)) {}

    const std::vector<int> getDims() const { return dims; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

class CompUnitAST: public BaseAST {
private:
    ASTPtrList nodes;
public:
    explicit CompUnitAST(ASTPtrList _nodes): nodes(std::move(_nodes)) {}

    const ASTPtrList &getNodes() const { return nodes; }

    ASTPtr Eval(TypeCheck &checker) override;
    std::string GenerateIR(IRGenerator &gen, std::string &code) override;
};

#endif