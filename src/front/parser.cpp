#include <iostream>
#include <vector>
#include <string_view>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include "../define/token.hpp"
#include "parser.hpp"

void Parser::NextToken() {
    current = lexer.NextToken();
}

ASTPtr Parser::ParseBinary(std::function<ASTPtr()> parser, std::initializer_list<Operator> ops) {
    auto lhs = parser();
    if (!lhs)
        return nullptr;
    while (current == Token::OPERATOR && std::find(ops.begin(), ops.end(), lexer.getOp()) != ops.end()) {
        NextToken();
        auto rhs = parser();
        if (!rhs)
            return nullptr;
        lhs = std::make_unique<BinaryExpAST>(lexer.getOp(), std::move(lhs), std::move(rhs));
    }
    return lhs;
}

/*
 * RelExp := AddExp RRelExp
 * RRelExp := (">" | "<" | ">=" | "<=") AddExp RRelExp
 */
ASTPtr Parser::ParseRelExp() {
    return ParseBinary([this] { return ParseAddExp(); },
                       {Operator::GT, Operator::GE, Operator::LT, Operator::LE});
}

/*
 * REqExp := ("==" | "!=") RelExp REqExp | %empty;
 * EqExp := RelExp REqExp;
 */
ASTPtr Parser::ParseEqExp(bool isConst) {
    return ParseBinary([this] { return ParseRelExp(); },
                       {Operator::EQ, Operator::NEQ});
}

/*
 * AddExp := MulExp RAddExp
 * RAddExp := ("+" | "-") MulExp RAddExp | %empty;
 */
ASTPtr Parser::ParseAddExp(bool isConst) {
    return ParseBinary([this] { return ParseMulExp(); },
                       {Operator::ADD, Operator::SUB});
}

/*
 * MulExp := UnaryExp RMulExp
 * RMulExp := MulOp UnaryExp RMulExp | %empty;
 */
ASTPtr Parser::ParseMulExp(bool isConst) {
    return ParseBinary([this] { return ParseUnaryExp(); },
                       {Operator::MUL, Operator::DIV, Operator::MOD});
}

/*
 * LAndExp := EqExp RLAndExp
 * RLAndExp := AND EqExp RLAndExp | %empty;
 */
ASTPtr Parser::ParseLAndExp(bool isConst) {
    return ParseBinary([this] { return ParseEqExp(); },
                       {Operator::AND});
}

/*
 * LOrExp := LAndExp RLOrExp;
 * RLOrExp := OR LAndExp RLOrExp | %empty;
 */
ASTPtr Parser::ParseLOrExp(bool isConst) {
    return ParseBinary([this] { return ParseLAndExp(); },
                       {Operator::OR});
}

/*
 * UnaryExp := "(" Exp ")"
 * | IDENTIFIER {"[" Exp "]"} // LVal
 * | NUMBER
 * | IDENTIFIER "(" [FunRParams] ")" // FuncCall
 * | ("+" | "-" | "!") UnaryExp;
 */
ASTPtr Parser::ParseUnaryExp() {
    NextToken();
    if (current == Token::LP) {
        // UnaryExp := "(" Exp ")"
        ASTPtr exp = ParseAddExp();
        NextToken(); // ")" RP
        if (current != Token::RP)
            return nullptr;
        NextToken();
        return exp;
    }
    else if (current == Token::NUMBER) {
        // UnaryExp := NUMBER
        ASTPtr num = std::make_unique<NumberAST>(lexer.getVal());
        return num;
    }
    else if (current == Token::OPERATOR) {
        // UnaryExp := ("+" | "-" | "!") UnaryExp
        Operator op = lexer.getOp();
        if (op != Operator::ADD && op != Operator::SUB && op != Operator::NOT)
            return nullptr;
        ASTPtr exp = ParseUnaryExp();
        if (!exp)
            return nullptr;
        sonNodes.push_back(std::move(exp));
        return std::make_unique<UnaryExpAST>(std::move(exp), op);
    }
    else if (current == Token::IDENTIFIER) {
        std::string name = lexer.getName();
        NextToken();
        if (current == Token::LP) {
            // parse FuncCall
            // sonNodes.push_back(std::make_unique<IdAST>(name));
            NextToken();
            if (current == Token::RP) {
                ASTPtr funcCall = std::make_unique<FuncCallAST>(name);
                return funcCall;
            }
            else {
                ASTPtrList args;
                while (true) {
                    ASTPtr exp = ParseAddExp();
                    if (!exp)
                        return nullptr;
                    args.push_back(std::move(exp));
                    NextToken();
                    if (current != Token::CO)
                        break;
                    NextToken();
                }
                if (current != Token::RP)
                    return nullptr;
                NextToken();
                return std::make_unique<FuncCallAST>(name, std::move(args));
            }
        }
        else if (current == Token::LSB){
            // parseLVal
            ASTPtrList pos;
            while (current == Token::LSB) {
                NextToken();
                ASTPtr exp = ParseAddExp();
                pos.push_back(std::move(exp));
                NextToken();
                if (current != Token::RSB)
                    return nullptr;
                NextToken();
            }
            ASTPtr LVal = std::make_unique<LValAST>(name, pos);
            return LVal;
        } else {
            return std::make_unique<LValAST>(name);
        }
    }
}

/*
 * IfElseStmt := "if" "(" Cond ")" Stmt ["else" Stmt]
 * Cond := LOrExp
 */
ASTPtr Parser::ParseIfElseStmt() {
    NextToken();
    if (current != Token::LP)
        return nullptr;
    NextToken();
    ASTPtr cond = ParseLOrExp();
    if (!cond)
        return nullptr;
    if (current != Token::RP)
        return nullptr;
    NextToken();
    ASTPtr thenStmt = ParseStmt();
    if (!thenStmt)
        return nullptr;
    if (current == Token::ELSE) {
        NextToken();
        ASTPtr elseStmt = ParseStmt();
        return std::make_unique<IfElseAST>(std::move(cond), std::move(thenStmt), std::move(elseStmt));
    } else {
        NextToken();
        return std::make_unique<IfElseAST>(std::move(cond), std::move(thenStmt));
    }
}

/*
 * WhileStmt := "while" "(" Cond ")" Stmt
 * Cond := LOrExp
 */
ASTPtr Parser::ParseWhileStmt() {
    NextToken();
    if (current != Token::LP)
        return nullptr;
    NextToken();
    ASTPtr cond = ParseLOrExp();
    if (!cond)
        return nullptr;
    if (current != Token::RP)
        return nullptr;
    NextToken();
    ASTPtr thenStmt = ParseStmt();
    if (!thenStmt)
        return nullptr;
    return std::make_unique<WhileAST>(std::move(cond), std::move(thenStmt));
}

/*
 * Stmt := LVal "=" Exp ";"
 * | Exp ";"
 * | ";"
 * | Block
 * | IfStmt
 * | IfElseStmt
 * | WHILE "(" Cond ")" Stmt
 * | BREAK ";"
 * | CONTINUE ";"
 * | RETURN ";"
 * | RETURN Exp ";"
 */
ASTPtr Parser::ParseStmt() {
    if (current == Token::IF) {
        ASTPtr stmt = ParseIfElseStmt();
        if (current != Token::SC)
            return nullptr;
        NextToken();
        return std::make_unique<StmtAST>(stmt);
    }
    else if (current == Token::WHILE) {
        ASTPtr stmt = ParseWhileStmt();
        if (current != Token::SC)
            return nullptr;
        NextToken();
        return std::make_unique<StmtAST>(stmt);
    }
    else if (current == Token::BREAK || current == Token::CONTINUE || current == Token::RETURN) {
        Token temp = current;
        NextToken();
        ASTPtr stmt;
        if (current == Token::SC)
            stmt = std::make_unique<ControlAST>(temp);
        else {
            ASTPtr retExp = ParseAddExp();
            stmt = std::make_unique<ControlAST>(Token::RETURN, std::move(retExp));
        }
        NextToken();
        return std::make_unique<StmtAST>(stmt);
    }
    else if (current == Token::SC) {
        NextToken();
        return std::make_unique<StmtAST>(nullptr);
    }
    else if (current == Token::LB) {
        ASTPtr block = ParseBlock();
        if (!block)
            return nullptr;
        NextToken(); // consume ;
        return std::make_unique<StmtAST>(block)
    }
    else {
        ASTPtr exp = ParseAddExp();
        if (!exp)
            return nullptr;
        NextToken();
        if (std::dynamic_pointer_cast<LValAST>(exp)) {
            if (current == Token::ASSIGN) {
                NextToken();
                ASTPtr rhs = ParseAddExp();
                ASTPtr stmt = std::make_unique<AssignAST>(exp, rhs);
                NextToken();
                if (current != Token::SC)
                    return nullptr;
                return std::make_unique<StmtAST>(stmt);
            } else if (current == Token::SC) {
                return std::make_unique<StmtAST>(exp);
            } else {
                return nullptr;
            }
        } else {
            if (current != Token::SC)
                return nullptr;
        }
    }
}

/*
 * Block: "{" {BlockItem} "}"
 * BlockItem: Decl | Stmt;
 */
ASTPtr Parser::ParseBlock() {
    NextToken();
    if (current == Token::RB) {
        return std::make_unique<BlockAST>(ASTPtrList{});
    } else {
        ASTPtrList list;
        while (current != Token::RB) {
            if (current == Token::CONST || current == Token::TYPE) {
                ASTPtr decl = ParseDecl();
                if (!decl)
                    return nullptr;
                list.push_back(decl);
            } else {
                ASTPtr stmt = ParseStmt();
                if (!stmt)
                    return nullptr;
                list.push_back(stmt);
            }
        }
        return std::make_unique<BlockAST>(list);
    }
}

/*
 * InitVal : Exp | "{" [InitVal {"," InitVal}] "}";
 */
ASTPtr Parser::ParseInitVal() {
    if (current == Token::LB) {
        NextToken();
        if (current == Token::RB) {
            NextToken();
            return std::make_unique<InitValAST>(VarType::ARRAY, ASTPtrList{});
        } else {
            ASTPtrList initVals;
            while (true) {
                ASTPtr init = ParseInitVal();
                initVals.push_back(init);
                if (!init)
                    return nullptr;
                if (current != Token::CO)
                    break;
                NextToken();
            }
            if (current != Token::RB)
                return nullptr;
            return std::make_unique<InitValAST>(VarType::ARRAY, initVals);
        }
    } else {
        ASTPtr exp = ParseAddExp();
        return std::make_unique<InitValAST>(VarType::VAR, ASTPtrList{std::move(exp)});
    }
}

/*
 * FuncDef := FuncType IDENT "(" [FuncFParams] ")" Block;
 * FuncType := INT | VOID
 * FuncFParams := FuncFParam {"," FuncFParam};
 * FuncFParam := BType IDENT ["[" "]" {"[" ConstExp "]"}];
 * BType := INT
 */
ASTPtr Parser::ParseFuncDef() {
    Type type = lexer.getType();
    NextToken();
    if (current != Token::IDENTIFIER)
        return nullptr;
    std::string name = lexer.getName();
    NextToken();
    if (current != Token::LP)
        return nullptr;
    NextToken();
    ASTPtrList args;
    if (current != Token::RP) {
        while (true) {
            NextToken();
            if (current != Token::TYPE || lexer.getType() != Type::INT)
                return nullptr;
            NextToken();
            std::string argName = lexer.getName();
            // args.push_back(lexer.getName());
            NextToken();
            if (current == Token::LSB) {
                ASTPtrList dim = {std::make_unique<NumberAST>(0)};
                NextToken();
                if (current != Token::RSB)
                    return nullptr;
                NextToken();
                if (current == Token::LSB) {
                    while (true) {
                        NextToken();
                        ASTPtr _dim = ParseAddExp();
                        if (!_dim)
                            return nullptr;
                        dim.push_back(_dim);
                        if (current != Token::RSB)
                            return nullptr;
                        NextToken();
                        if (current != Token::LSB)
                            break;
                    }
                }
                args.push_back(std::make_unique<IdAST>(argName, VarType::ARRAY, std::move(dim)));
            } else {
                args.push_back(std::make_unique<IdAST>(argName, VarType::VAR));
            }
            if (current != Token::CO)
                break;
        }
        if (current != Token::RP)
            return nullptr;
    }
    NextToken();
    ASTPtr body = ParseBlock();
    return std::make_unique<FuncDefAST>(type, std::move(name), std::move(args), std::move(body));
}

/*
 * VarDecl := [CONST] BType VarDef {"," VarDef} ";";
 * BType := INT
 */
ASTPtr Parser::ParseVarDecl() {
    bool isConst = false;
    if (current == Token::CONST)
        isConst = true;
    if (lexer.getType() != Type::INT)
        return nullptr;
    NextToken();
    ASTPtrList vars;
    ASTPtr varDef = ParseVarDef(isConst);
    if (!varDef)
        return nullptr;
    vars.push_back(std::move(varDef));
    while (current == Token::CO) {
        NextToken();
        varDef = ParseVarDef(isConst);
        if (!varDef)
            return nullptr;
        vars.push_back(std::move(VarDef));
    }
    NextToken();
    if (current != Token::SC)
        return nullptr;
    NextToken();
    return std::make_unique<VarDeclAST>(isConst, std::move(vars));
}

/*
 * VarDef := IDENT {"[" ConstExp "]"} ["=" InitVal]
 * ConstDef: IDENT {"[" ConstExp "]"} "=" ConstInitVal;
 */
ASTPtr Parser::ParseVarDef(bool isConst) {
    if (current != Token::IDENTIFIER)
        return nullptr;
    std::string name = lexer.getName();
    ASTPtrList dims;
    NextToken();
    while (current == Token::LSB) {
        NextToken();
        ASTPtr exp = ParseAddExp();
        if (!exp)
            return nullptr;
        dims.push_back(exp);
        if (current != Token::RSB)
            return nullptr;
        NextToken();
    }
    ASTPtr var;
    if (dims.empty())
        var = std::make_unique<IdAST>(name, VarType::VAR, isConst);
    else
        var = std::make_unique<IdAST>(name, VarType::ARRAY, isConst, dims);
    if (current == Token::ASSIGN) {
        NextToken();
        ASTPtr init = ParseInitVal();
        if (!init)
            return nullptr;
        return std::make_unique<VarDefAST>(isConst, var, init);
    } else {
        if (isConst)
            return nullptr;
        return std::make_unique<VarDefAST>(isConst, var);
    }
}

/*
 * CompUnit := (Decl | FuncDef) [CompUnit];
 */
ASTPtr Parser::ParseCompUnit() {
    NextToken();
    ASTPtrList nodes;
    while (current != Token::EOF) {
        if (current == Token::CONST) {
            ASTPtr decl = ParseVarDecl();
            if (!decl)
                return nullptr;
            nodes.push_back(decl);
        }
        if (current == Token::TYPE) {
            if (lexer.getType() == Type::VOID) {
                ASTPtr func = ParseFuncDef();
                if (!func)
                    return nullptr;
                nodes.push_back(func);
            }
            else {
                Type type = lexer.getType();
                NextToken();
                if (current != Token::IDENTIFIER)
                    return nullptr;
                std::string name = lexer.getName();
                NextToken();
                if (current == Token::LP) {
                    // parse funcDef
                    NextToken();
                    ASTPtrList args;
                    if (current != Token::RP) {
                        while (true) {
                            NextToken();
                            if (current != Token::TYPE || lexer.getType() != Type::INT)
                                return nullptr;
                            NextToken();
                            std::string argName = lexer.getName();
                            // args.push_back(lexer.getName());
                            NextToken();
                            if (current == Token::LSB) {
                                ASTPtrList dim = {std::make_unique<NumberAST>(0)};
                                NextToken();
                                if (current != Token::RSB)
                                    return nullptr;
                                NextToken();
                                if (current == Token::LSB) {
                                    while (true) {
                                        NextToken();
                                        ASTPtr _dim = ParseAddExp();
                                        if (!_dim)
                                            return nullptr;
                                        dim.push_back(_dim);
                                        if (current != Token::RSB)
                                            return nullptr;
                                        NextToken();
                                        if (current != Token::LSB)
                                            break;
                                    }
                                }
                                args.push_back(std::make_unique<IdAST>(argName, VarType::ARRAY, std::move(dim)));
                            } else {
                                args.push_back(std::make_unique<IdAST>(argName, VarType::VAR));
                            }
                            if (current != Token::CO)
                                break;
                        }
                        if (current != Token::RP)
                            return nullptr;
                    }
                    NextToken();
                    ASTPtr body = ParseBlock();
                    ASTPtr func = std::make_unique<FuncDefAST>(type, std::move(name), std::move(args), std::move(body));
                    nodes.push_back(std::move(func));
                }
                else {
                    ASTPtrList varDefs;
                    // parse first var def
                    ASTPtrList dims;
                    while (current == Token::LSB) {
                        NextToken();
                        ASTPtr exp = ParseAddExp();
                        if (!exp)
                            return nullptr;
                        dims.push_back(exp);
                        if (current != Token::RSB)
                            return nullptr;
                        NextToken();
                    }
                    ASTPtr var;
                    ASTPtr varDef;
                    if (dims.empty())
                        var = std::make_unique<IdAST>(name, VarType::VAR, false);
                    else
                        var = std::make_unique<IdAST>(name, VarType::ARRAY, false, dims);
                    if (current == Token::ASSIGN) {
                        NextToken();
                        ASTPtr init = ParseInitVal();
                        if (!init)
                            return nullptr;
                        varDef = std::make_unique<VarDefAST>(false, var, init);
                    } else {
                        varDef = std::make_unique<VarDefAST>(false, var);
                    }
                    varDefs.push_back(std::move(varDef));
                    while (current == Token::CO) {
                        NextToken();
                        varDef = ParseVarDef(false);
                        if (!varDef)
                            return nullptr;
                        varDefs.push_back(std::move(VarDef));
                    }
                    NextToken();
                    if (current != Token::SC)
                        return nullptr;
                    ASTPtr decl = std::make_unique<VarDeclAST>(false, std::move(varDefs));
                    nodes.push_back(std::move(decl));
                    NextToken();
                }
            }
        }
    }
    return std::make_unique<CompUnitAST>(std::move(nodes));
}
