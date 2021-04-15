#include <iostream>
#include <vector>
#include <string_view>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include "../define/token.hpp"
#include "parser.hpp"
#include "logger.hpp"

void Parser::NextToken() {
    current = lexer.NextToken();
}

ASTPtr Parser::ParseBinary(std::function<ASTPtr()> parser, std::initializer_list <Operator> ops) {
    logger.SetFunc("ParseBinary");
    auto lhs = parser();
    logger.UnSetFunc("ParseBinary");
    if (!lhs) {
        logger.Warning("Parse lhs failed");
        return nullptr;
    }
    while (current == Token::OPERATOR && std::find(ops.begin(), ops.end(), lexer.getOp()) != ops.end()) {
        NextToken();
        auto rhs = parser();
        logger.UnSetFunc("ParseBinary");
        if (!rhs) {
            logger.Warning("Parse rhs failed");
            return nullptr;
        }
        lhs = std::make_unique<BinaryExpAST>(lexer.getOp(), std::move(lhs), std::move(rhs));
    }
    return lhs;
}

/*
 * RelExp := AddExp RRelExp
 * RRelExp := (">" | "<" | ">=" | "<=") AddExp RRelExp
 */
ASTPtr Parser::ParseRelExp() {
    logger.SetFunc("ParseRelExp");
    return ParseBinary([this] { return ParseAddExp(); },
                       {Operator::GT, Operator::GE, Operator::LT, Operator::LE});
}

/*
 * REqExp := ("==" | "!=") RelExp REqExp | %empty;
 * EqExp := RelExp REqExp;
 */
ASTPtr Parser::ParseEqExp() {
    logger.SetFunc("ParseEqExp");
    return ParseBinary([this] { return ParseRelExp(); },
                       {Operator::EQ, Operator::NEQ});
}

/*
 * AddExp := MulExp RAddExp
 * RAddExp := ("+" | "-") MulExp RAddExp | %empty;
 */
ASTPtr Parser::ParseAddExp() {
    logger.SetFunc("ParseAddExp");
    return ParseBinary([this] { return ParseMulExp(); },
                       {Operator::ADD, Operator::SUB});
}

/*
 * MulExp := UnaryExp RMulExp
 * RMulExp := MulOp UnaryExp RMulExp | %empty;
 */
ASTPtr Parser::ParseMulExp() {
    logger.SetFunc("ParseMulExp");
    return ParseBinary([this] { return ParseUnaryExp(); },
                       {Operator::MUL, Operator::DIV, Operator::MOD});
}

/*
 * LAndExp := EqExp RLAndExp
 * RLAndExp := AND EqExp RLAndExp | %empty;
 */
ASTPtr Parser::ParseLAndExp() {
    logger.SetFunc("ParseLAndExp");
    return ParseBinary([this] { return ParseEqExp(); },
                       {Operator::AND});
}

/*
 * LOrExp := LAndExp RLOrExp;
 * RLOrExp := OR LAndExp RLOrExp | %empty;
 */
ASTPtr Parser::ParseLOrExp() {
    logger.SetFunc("ParseLOrExp");
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
    logger.SetFunc("ParseUnaryExp");
    NextToken();
    if (current == Token::LP) {
        // UnaryExp := "(" Exp ")"
        ASTPtr exp = ParseAddExp();
        logger.UnSetFunc("ParseUnaryExp");
        NextToken(); // ")" RP
        if (current != Token::RP) {
            logger.Warning("Production " + "\"(\" Exp \")\"" + " Lack Right Parenthese");
            return nullptr;
        }
        NextToken();
        return exp;
    } else if (current == Token::NUMBER) {
        // UnaryExp := NUMBER
        ASTPtr num = std::make_unique<NumberAST>(lexer.getVal());
        return num;
    } else if (current == Token::OPERATOR) {
        // UnaryExp := ("+" | "-" | "!") UnaryExp
        Operator op = lexer.getOp();
        if (op != Operator::ADD && op != Operator::SUB && op != Operator::NOT) {
            logger.Warning("Production (\"+\" | \"-\" | \"!\") UnaryExp invalid symbol");
            return nullptr;
        }
        ASTPtr exp = ParseUnaryExp();
        logger.UnSetFunc("ParseUnaryExp");
        if (!exp) {
            logger.Warning("Production (\"+\" | \"-\" | \"!\") UnaryExp parse exp failed");
            return nullptr;
        }
        return std::make_unique<UnaryExpAST>(std::move(exp), op);
    } else if (current == Token::IDENTIFIER) {
        std::string name = lexer.getName();
        NextToken();
        if (current == Token::LP) {
            // parse FuncCall
            NextToken();
            if (current == Token::RP) {
                ASTPtr funcCall = std::make_unique<FuncCallAST>(name);
                return funcCall;
            } else {
                ASTPtrList args;
                while (true) {
                    ASTPtr exp = ParseAddExp();
                    logger.UnSetFunc("ParseUnaryExp");
                    if (!exp) {
                        logger.Warning("Production IDENTIFIER \"(\" [FunRParams] \")\" parse FuncRParams failed");
                        return nullptr;
                    }
                    args.push_back(std::move(exp));
                    NextToken();
                    if (current != Token::CO)
                        break;
                    NextToken(); // consume CO
                }
                if (current != Token::RP) {
                    logger.Warning("Production IDENTIFIER \"(\" [FunRParams] \")\" lack right parentheses");
                    return nullptr;
                }
                NextToken(); // consume RP
                return std::make_unique<FuncCallAST>(name, std::move(args));
            }
        } else if (current == Token::LSB) {
            // parseLVal
            ASTPtrList pos;
            while (current == Token::LSB) {
                NextToken();
                ASTPtr exp = ParseAddExp();
                logger.UnSetFunc("ParseUnaryExp");
                pos.push_back(std::move(exp));
                NextToken();
                if (current != Token::RSB) {
                    logger.Warning("Production IDENTIFIER {\"[\" Exp \"]\"}  lack right square bracket");
                    return nullptr;
                }
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
    logger.SetFunc("ParseIfElseStmt");
    NextToken();
    if (current != Token::LP) {
        logger.Warning("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] lack left parentheses after if");
        return nullptr;
    }
    NextToken();
    ASTPtr cond = ParseLOrExp();
    logger.UnSetFunc("ParseIfElseStmt");
    if (!cond) {
        logger.Warning("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] parse cond failed");
        return nullptr;
    }
    if (current != Token::RP) {
        logger.Warning("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] lack right parentheses after if");
        return nullptr;
    }
    NextToken();
    ASTPtr thenStmt = ParseStmt();
    logger.UnSetFunc("ParseIfElseStmt");
    if (!thenStmt) {
        logger.Warning("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] parse then failed");
        return nullptr;
    }
    if (current == Token::ELSE) {
        NextToken();
        ASTPtr elseStmt = ParseStmt();
        logger.UnSetFunc("ParseIfElseStmt");
        if (!elseStmt) {
            logger.Warning("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] parse else failed");
            return nullptr;
        }
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
    logger.SetFunc("ParseWhileStmt");
    NextToken();
    if (current != Token::LP) {
        logger.Warning("Production \"while\" \"(\" Cond \")\" Stmt lack left parentheses after while");
        return nullptr;
    }
    NextToken();
    ASTPtr cond = ParseLOrExp();
    logger.UnSetFunc("ParseWhileStmt");
    if (!cond) {
        logger.Warning("Production \"while\" \"(\" Cond \")\" Stmt parse cond failed");
        return nullptr;
    }
    if (current != Token::RP) {
        logger.Warning("Production \"while\" \"(\" Cond \")\" Stmt lack right parentheses after while");
        return nullptr;
    }
    NextToken();
    ASTPtr thenStmt = ParseStmt();
    logger.UnSetFunc("ParseWhileStmt");
    if (!thenStmt) {
        logger.Warning("Production \"while\" \"(\" Cond \")\" Stmt parse then failed");
        return nullptr;
    }
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
    logger.SetFunc("ParseStmt");
    if (current == Token::IF) {
        ASTPtr stmt = ParseIfElseStmt();
        logger.UnSetFunc("ParseStmt");
        if (!stmt) {
            logger.Warning("Production IfElseStmt parse IfElseStmt failed");
            return nullptr;
        }
        if (current != Token::SC) {
            logger.Warning("Lack semicolon");
            return nullptr;
        }
        NextToken();
        return std::make_unique<StmtAST>(stmt);
    } else if (current == Token::WHILE) {
        ASTPtr stmt = ParseWhileStmt();
        logger.UnSetFunc("ParseStmt");
        if (!stmt) {
            logger.Warning("Production WhileStmt parse WhileStmt failed");
            return nullptr;
        }
        if (current != Token::SC) {
            logger.Warning("Lack semicolon");
            return nullptr;
        }
        return std::make_unique<StmtAST>(stmt);
    } else if (current == Token::BREAK || current == Token::CONTINUE || current == Token::RETURN) {
        Token temp = current;
        NextToken();
        ASTPtr stmt;
        if (current == Token::SC)
            stmt = std::make_unique<ControlAST>(temp);
        else {
            ASTPtr retExp = ParseAddExp();
            logger.UnSetFunc("ParseStmt");
            if (!retExp) {
                logger.Warning("Production ReturnStmt parse return exp failed");
                return nullptr;
            }
            if (current != Token::SC) {
                logger.Warning("Lack semicolon");
                return nullptr;
            }
            stmt = std::make_unique<ControlAST>(Token::RETURN, std::move(retExp));
        }
        NextToken(); // consume SC
        return std::make_unique<StmtAST>(stmt);
    } else if (current == Token::SC) {
        NextToken();
        return std::make_unique<StmtAST>(nullptr);
    } else if (current == Token::LB) {
        ASTPtr block = ParseBlock();
        logger.UnSetFunc("ParseStmt");
        if (!block) {
            logger.Warning("Production Block parse block failed");
            return nullptr;
        }
        NextToken();
        return std::make_unique<StmtAST>(block)
    } else {
        ASTPtr exp = ParseAddExp();
        logger.UnSetFunc("ParseStmt");
        if (!exp) {
            logger.Warning("Production Exp \";\"/LVal \"=\" Exp \";\" parse lval failed");
            return nullptr;
        }
        NextToken();
        if (std::dynamic_pointer_cast<LValAST>(exp)) {
            // LVal "=" Exp ";"
            if (current == Token::ASSIGN) {
                NextToken();
                ASTPtr rhs = ParseAddExp();
                logger.UnSetFunc("ParseStmt");
                if (!rhs) {
                    logger.Warning("Production Exp \";\"/LVal \"=\" Exp \";\" parse rhs failed");
                    return nullptr;
                }
                ASTPtr stmt = std::make_unique<AssignAST>(exp, rhs);
                NextToken();
                if (current != Token::SC) {
                    logger.Warning("Lack semicolon");
                    return nullptr;
                }
                return std::make_unique<StmtAST>(stmt);
            } else if (current == Token::SC) {
                // Exp;
                return std::make_unique<StmtAST>(exp);
            } else {
                logger.Warning("Lack semicolon");
                return nullptr;
            }
        } else {
            // ;
            if (current != Token::SC) {
                logger.Warning("Lack semicolon");
                return nullptr;
            }
            return std::make_unique<StmtAST>(exp);
        }
    }
}

/*
 * Block: "{" {BlockItem} "}"
 * BlockItem: Decl | Stmt;
 */
ASTPtr Parser::ParseBlock() {
    logger.SetFunc("ParseBlock");
    NextToken();
    if (current == Token::RB) {
        // {}
        return std::make_unique<BlockAST>(ASTPtrList{});
    } else {
        ASTPtrList list;
        while (current != Token::RB) {
            if (current == Token::CONST || current == Token::TYPE) {
                ASTPtr decl = ParseDecl();
                logger.UnSetFunc("ParseBlock");
                if (!decl) {
                    logger.Warning("Production Decl parse decl failed");
                    return nullptr;
                }
                list.push_back(decl);
            } else {
                ASTPtr stmt = ParseStmt();
                logger.UnSetFunc("ParseBlock");
                if (!stmt) {
                    logger.Warning("Production Stmt parse stmt failed");
                    return nullptr;
                }
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
    logger.SetFunc("ParseInitVal");
    if (current == Token::LB) {
        NextToken();
        if (current == Token::RB) {
            NextToken();
            return std::make_unique<InitValAST>(VarType::ARRAY, ASTPtrList{});
        } else {
            ASTPtrList initVals;
            while (true) {
                ASTPtr init = ParseInitVal();
                logger.UnSetFunc("ParseInitVal");
                if (!init) {
                    logger.Warning("Production \"{\" [InitVal {\",\" InitVal}] \"}\" parse InitVal failed");
                    return nullptr;
                }
                initVals.push_back(init);
                if (current != Token::CO)
                    break;
                NextToken();
            }
            if (current != Token::RB) {
                logger.Warning("Production \"{\" [InitVal {\",\" InitVal}] \"}\" lack right brace after InitVals");
                return nullptr;
            }
            return std::make_unique<InitValAST>(VarType::ARRAY, initVals);
        }
    } else {
        ASTPtr exp = ParseAddExp();
        logger.UnSetFunc("ParseInitVal");
        if (!exp) {
            logger.Warning("Production Exp parse exp failed");
            return nullptr;
        }
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
    logger.SetFunc("ParseFuncDef");
    Type type = lexer.getType();
    NextToken();
    if (current != Token::IDENTIFIER) {
        logger.Warning("Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lacked identifier");
        return nullptr;
    }
    std::string name = lexer.getName();
    NextToken();
    if (current != Token::LP) {
        logger.Warning(
                "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lacked left parentheses after identifier");
        return nullptr;
    }
    NextToken();
    ASTPtrList args;
    if (current != Token::RP) {
        while (true) {
            NextToken();
            if (current != Token::TYPE || lexer.getType() != Type::INT) {
                logger.Warning("Production FuncType IDENT \"(\" [FuncFParams] \")\" Block wrong arg type");
                return nullptr;
            }
            NextToken();
            std::string argName = lexer.getName();
            NextToken();
            if (current == Token::LSB) {
                ASTPtrList dim = {std::make_unique<NumberAST>(0)};
                NextToken();
                if (current != Token::RSB) {
                    logger.Warning(
                            "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lack right bracket in array arg");
                    return nullptr;
                }
                NextToken();
                if (current == Token::LSB) {
                    while (true) {
                        NextToken();
                        ASTPtr _dim = ParseAddExp();
                        logger.UnSetFunc("ParseFuncDef");
                        if (!_dim) {
                            logger.Warning(
                                    "Production BType IDENT [\"[\" \"]\" {\"[\" ConstExp \"]\"}] parse exp dim failed");
                            return nullptr;
                        }
                        dim.push_back(_dim);
                        if (current != Token::RSB) {
                            logger.Warning(
                                    "Production BType IDENT [\"[\" \"]\" {\"[\" ConstExp \"]\"}] lack right square bracket");
                            return nullptr;
                        }
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
        if (current != Token::RP) {
            logger.Warning(
                    "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lacked right parentheses after identifier");
            return nullptr;
        }
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
    logger.SetFunc("ParseVarDecl");
    bool isConst = false;
    if (current == Token::CONST)
        isConst = true;
    if (lexer.getType() != Type::INT) {
        logger.Warning("Production [CONST] BType VarDef {\",\" VarDef} \";\" wrong BType");
        return nullptr;
    }
    NextToken();
    ASTPtrList vars;
    ASTPtr varDef = ParseVarDef(isConst);
    logger.UnSetFunc("ParseVarDecl");
    if (!varDef) {
        logger.Warning("Production [CONST] BType VarDef {\",\" VarDef} \";\" parse vardef failed");
        return nullptr;
    }
    vars.push_back(std::move(varDef));
    while (current == Token::CO) {
        NextToken();
        varDef = ParseVarDef(isConst);
        logger.UnSetFunc("ParseVarDecl");
        if (!varDef) {
            logger.Warning("Production [CONST] BType VarDef {\",\" VarDef} \";\" parse vardef failed");
            return nullptr;
        }
        vars.push_back(std::move(VarDef));
    }
    NextToken();
    if (current != Token::SC) {
        logger.Warning("lack semicolon");
        return nullptr;
    }
    NextToken();
    return std::make_unique<VarDeclAST>(isConst, std::move(vars));
}

/*
 * VarDef := IDENT {"[" ConstExp "]"} ["=" InitVal]
 * ConstDef: IDENT {"[" ConstExp "]"} "=" ConstInitVal;
 */
ASTPtr Parser::ParseVarDef(bool isConst) {
    logger.SetFunc("ParseVarDef");
    if (current != Token::IDENTIFIER) {
        logger.Warning("Production IDENT {\"[\" ConstExp \"]\"} [\"=\" InitVal] lacked identifier");
        return nullptr;
    }
    std::string name = lexer.getName();
    ASTPtrList dims;
    NextToken();
    while (current == Token::LSB) {
        NextToken();
        ASTPtr exp = ParseAddExp();
        logger.UnSetFunc("ParseVarDef");
        if (!exp) {
            logger.Warning("Production IDENT {\"[\" ConstExp \"]\"} [\"=\" InitVal] parse exp failed");
            return nullptr;
        }
        dims.push_back(exp);
        if (current != Token::RSB) {
            logger.Warning(
                    "Production IDENT {\"[\" ConstExp \"]\"} [\"=\" InitVal] lack right square bracket after exp");
            return nullptr;
        }
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
        logger.UnSetFunc("ParseVarDef");
        if (!init) {
            logger.Warning("Production IDENT {\"[\" ConstExp \"]\"} [\"=\" InitVal] parse init failed");
            return nullptr;
        }
        return std::make_unique<VarDefAST>(isConst, var, init);
    } else {
        if (isConst) {
            logger.Warning("Production ConstDef: IDENT {\"[\" ConstExp \"]\"} \"=\" ConstInitVal lacked assignment");
            return nullptr;
        }
        return std::make_unique<VarDefAST>(isConst, var);
    }
}

/*
 * CompUnit := (Decl | FuncDef) [CompUnit];
 */
ASTPtr Parser::ParseCompUnit() {
    logger.SetFunc("ParseCompUnit");
    NextToken();
    ASTPtrList nodes;
    while (current != Token::EOF) {
        if (current == Token::CONST) {
            ASTPtr decl = ParseVarDecl();
            logger.UnSetFunc("ParseCompUnit");
            if (!decl) {
                logger.Warning("Production (Decl | FuncDef) [CompUnit] parse decl failed");
                return nullptr;
            }
            nodes.push_back(decl);
        } else if (current == Token::TYPE) {
            if (lexer.getType() == Type::VOID) {
                ASTPtr func = ParseFuncDef();
                if (!func) {
                    logger.Warning("Production (Decl | FuncDef) [CompUnit] parse func failed");
                    return nullptr;
                }
                nodes.push_back(func);
            } else {
                Type type = lexer.getType();
                NextToken();
                if (current != Token::IDENTIFIER) {
                    logger.Warning("lack identifier");
                    return nullptr;
                }
                std::string name = lexer.getName();
                NextToken();
                if (current == Token::LP) {
                    // parse funcDef
                    NextToken();
                    ASTPtrList args;
                    if (current != Token::RP) {
                        while (true) {
                            NextToken();
                            if (current != Token::TYPE || lexer.getType() != Type::INT) {
                                logger.Warning(
                                        "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block wrong arg type");
                                return nullptr;
                            }
                            NextToken();
                            std::string argName = lexer.getName();
                            NextToken();
                            if (current == Token::LSB) {
                                ASTPtrList dim = {std::make_unique<NumberAST>(0)};
                                NextToken();
                                if (current != Token::RSB) {
                                    logger.Warning(
                                            "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lack right bracket in array arg");
                                    return nullptr;
                                }
                                NextToken();
                                if (current == Token::LSB) {
                                    while (true) {
                                        NextToken();
                                        ASTPtr _dim = ParseAddExp();
                                        logger.UnSetFunc("ParseFuncDef");
                                        if (!_dim) {
                                            logger.Warning(
                                                    "Production BType IDENT [\"[\" \"]\" {\"[\" ConstExp \"]\"}] parse exp dim failed");
                                            return nullptr;
                                        }
                                        dim.push_back(_dim);
                                        if (current != Token::RSB) {
                                            logger.Warning(
                                                    "Production BType IDENT [\"[\" \"]\" {\"[\" ConstExp \"]\"}] lack right square bracket");
                                            return nullptr;
                                        }
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
                        if (current != Token::RP) {
                            logger.Warning(
                                    "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lacked right parentheses after identifier");
                            return nullptr;
                        }
                    }
                    NextToken();
                    ASTPtr body = ParseBlock();
                    ASTPtr func = std::make_unique<FuncDefAST>(type, std::move(name), std::move(args), std::move(body));
                    nodes.push_back(std::move(func));
                } else {
                    ASTPtrList varDefs;
                    // parse first var def
                    ASTPtrList dims;
                    while (current == Token::LSB) {
                        NextToken();
                        ASTPtr exp = ParseAddExp();
                        logger.UnSetFunc("ParseVarDef");
                        if (!exp) {
                            logger.Warning("Production IDENT {\"[\" ConstExp \"]\"} [\"=\" InitVal] parse exp failed");
                            return nullptr;
                        }
                        dims.push_back(exp);
                        if (current != Token::RSB) {
                            logger.Warning(
                                    "Production IDENT {\"[\" ConstExp \"]\"} [\"=\" InitVal] lack right square bracket after exp");
                            return nullptr;
                        }
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
                        if (!init) {
                            logger.Warning("Production IDENT {\"[\" ConstExp \"]\"} [\"=\" InitVal] parse init failed");
                            return nullptr;
                        }
                        varDef = std::make_unique<VarDefAST>(false, var, init);
                    } else {
                        varDef = std::make_unique<VarDefAST>(false, var);
                    }
                    varDefs.push_back(std::move(varDef));
                    while (current == Token::CO) {
                        NextToken();
                        varDef = ParseVarDef(false);
                        logger.UnSetFunc("ParseVarDecl");
                        if (!varDef) {
                            logger.Warning("Production [CONST] BType VarDef {\",\" VarDef} \";\" parse vardef failed");
                            return nullptr;
                        }
                        varDefs.push_back(std::move(VarDef));
                    }
                    NextToken();
                    if (current != Token::SC) {
                        logger.Warning("lack semicolon");
                        return nullptr;
                    }
                    ASTPtr decl = std::make_unique<VarDeclAST>(false, std::move(varDefs));
                    nodes.push_back(std::move(decl));
                    NextToken();
                }
            }
        } else {
            logger.Warning("Wrong token");
            return nullptr;
        }
    }
    return std::make_unique<CompUnitAST>(std::move(nodes));
}
