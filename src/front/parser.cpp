#include <vector>
#include <string_view>
#include <functional>
#include <initializer_list>
#include <memory>

#include <define/token.hpp>
#include <front/parser.hpp>

namespace SysYToEeyore {
    void Parser::NextToken() {
        current = lexer.NextToken();
        logger.Info("NextToken: " + std::to_string(current));
        if (current == Token::IDENTIFIER) {
            logger.Info("Name: " + lexer.getName());
        }
        if (current == Token::NUMBER) {
            logger.Info("Value: " + std::to_string(lexer.getVal()));
        }
        if (current == Token::OPERATOR) {
            logger.Info("Operator: " + std::to_string(lexer.getOp()));
        }
        if (current == Token::COMMENT) {
            NextToken(); // Consume COMMENT
        }
    }

    ASTPtr Parser::ParseBinary(const std::function<ASTPtr()> &parser, std::initializer_list<Operator> ops) {
        logger.SetFunc("ParseBinary");
        auto lhs = parser();
        logger.UnSetFunc("ParseBinary");
        if (!lhs) {
            logger.Error("Parse lhs failed");
            exit(101);
        }
        while (current == Token::OPERATOR && std::find(ops.begin(), ops.end(), lexer.getOp()) != ops.end()) {
            Operator tmp = lexer.getOp();
            NextToken(); // Consume OPERATOR
            auto rhs = parser();
            logger.UnSetFunc("ParseBinary");
            if (!rhs) {
                logger.Error("Parse rhs failed");
                exit(102);
            }
            lhs = std::make_unique<BinaryExpAST>(tmp, std::move(lhs), std::move(rhs));
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
        if (current == Token::LP) {
            // UnaryExp := "(" Exp ")"
            NextToken();  // Consume LP
            ASTPtr exp = ParseAddExp();
            logger.UnSetFunc("ParseUnaryExp");
            if (current != Token::RP) {
                logger.Error("Production \"(\" Exp \")\" Lack Right Parenthese");
                exit(103);
            }
            NextToken(); // Consume RP
            return exp;
        } else if (current == Token::NUMBER) {
            // UnaryExp := NUMBER
            ASTPtr num = std::make_unique<NumberAST>(lexer.getVal());
            NextToken();  // Consumee NUMBER
            return num;
        } else if (current == Token::OPERATOR) {
            // UnaryExp := ("+" | "-" | "!") UnaryExp
            Operator op = lexer.getOp();
            if (op != Operator::ADD && op != Operator::SUB && op != Operator::NOT) {
                logger.Error(R"(Production ("+" | "-" | "!") UnaryExp invalid symbol)");
                exit(104);
            }
            NextToken(); // Consume OPERATOR
            ASTPtr exp = ParseUnaryExp();
            logger.UnSetFunc("ParseUnaryExp");
            if (!exp) {
                logger.Error(R"(Production ("+" | "-" | "!") UnaryExp parse exp failed)");
                exit(105);
            }
            return std::make_unique<UnaryExpAST>(std::move(exp), op);
        } else if (current == Token::IDENTIFIER) {
            std::string name = lexer.getName();
            NextToken();  // Consume IDENTIFIER
            if (current == Token::LP) {
                // parse FuncCall
                NextToken(); // Consume LP
                if (current == Token::RP) {
                    ASTPtr funcCall = std::make_unique<FuncCallAST>(name);
                    NextToken();  // Consume RP
                    return funcCall;
                } else {
                    ASTPtrList args;
                    while (true) {
                        ASTPtr exp = ParseAddExp();
                        logger.UnSetFunc("ParseUnaryExp");
                        if (!exp) {
                            logger.Error("Production IDENTIFIER \"(\" [FunRParams] \")\" parse FuncRParams failed");
                            exit(106);
                        }
                        args.push_back(std::move(exp));
                        if (current != Token::CO)
                            break;
                        NextToken(); // consume CO
                    }
                    if (current != Token::RP) {
                        logger.Error("Production IDENTIFIER \"(\" [FunRParams] \")\" lack right parentheses");
                        exit(107);
                    }
                    NextToken(); // consume RP
                    return std::make_unique<FuncCallAST>(name, std::move(args));
                }
            } else if (current == Token::LSB) {
                // parseLVal
                ASTPtrList pos;
                while (current == Token::LSB) {
                    NextToken();  // Consume LSB
                    ASTPtr exp = ParseAddExp();
                    logger.UnSetFunc("ParseUnaryExp");
                    pos.push_back(std::move(exp));
                    if (current != Token::RSB) {
                        logger.Error(R"(Production IDENTIFIER {"[" Exp "]"}  lack right square bracket)");
                        exit(108);
                    }
                    NextToken(); // Consume RSB
                }
                ASTPtr LVal = std::make_unique<LValAST>(name, VarType::ARRAY, std::move(pos));
                return LVal;
            } else {
                return std::make_unique<LValAST>(name, VarType::VAR);
            }
        } else {
            logger.Error("No matching production");
            exit(109);
        }
    }

/*
 * IfElseStmt := "if" "(" Cond ")" Stmt ["else" Stmt]
 * Cond := LOrExp
 */
    ASTPtr Parser::ParseIfElseStmt() {
        logger.SetFunc("ParseIfElseStmt");
        NextToken(); // Consume IF
        if (current != Token::LP) {
            logger.Error("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] lack left parentheses after if");
            exit(110);
        }
        NextToken(); // Consume LP
        ASTPtr cond = ParseLOrExp();
        // std::cout << "debug: " << dynamic_cast<BinaryExpAST*>(cond.get())->getOp();
        logger.UnSetFunc("ParseIfElseStmt");
        if (!cond) {
            logger.Error("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] parse cond failed");
            exit(111);
        }
        if (current != Token::RP) {
            logger.Error("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] lack right parentheses after if");
            exit(112);
        }
        NextToken(); // Consume RP
        ASTPtr thenStmt = ParseStmt();
        logger.UnSetFunc("ParseIfElseStmt");
        if (!thenStmt) {
            logger.Error("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] parse then failed");
            exit(113);
        }
        if (current == Token::ELSE) {
            NextToken(); // Consume ELSE
            ASTPtr elseStmt = ParseStmt();
            logger.UnSetFunc("ParseIfElseStmt");
            if (!elseStmt) {
                logger.Error("Production \"if\" \"(\" Cond \")\" Stmt [\"else\" Stmt] parse else failed");
                exit(114);
            }
            return std::make_unique<IfElseAST>(std::move(cond), std::move(thenStmt), std::move(elseStmt));
        } else {
            return std::make_unique<IfElseAST>(std::move(cond), std::move(thenStmt));
        }
    }

/*
 * WhileStmt := "while" "(" Cond ")" Stmt
 * Cond := LOrExp
 */
    ASTPtr Parser::ParseWhileStmt() {
        logger.SetFunc("ParseWhileStmt");
        NextToken(); // Consume WHILE
        if (current != Token::LP) {
            logger.Error("Production \"while\" \"(\" Cond \")\" Stmt lack left parentheses after while");
            exit(115);
        }
        NextToken(); //Consume LP
        ASTPtr cond = ParseLOrExp();
        logger.UnSetFunc("ParseWhileStmt");
        if (!cond) {
            logger.Error("Production \"while\" \"(\" Cond \")\" Stmt parse cond failed");
            exit(116);
        }
        if (current != Token::RP) {
            logger.Error("Production \"while\" \"(\" Cond \")\" Stmt lack right parentheses after while");
            exit(117);
        }
        NextToken(); // Consume RP
        ASTPtr thenStmt = ParseStmt();
        logger.UnSetFunc("ParseWhileStmt");
        if (!thenStmt) {
            logger.Error("Production \"while\" \"(\" Cond \")\" Stmt parse then failed");
            exit(118);
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
                logger.Error("Production IfElseStmt parse IfElseStmt failed");
                exit(119);
            }
            return std::make_unique<StmtAST>(std::move(stmt));
        } else if (current == Token::WHILE) {
            ASTPtr stmt = ParseWhileStmt();
            logger.UnSetFunc("ParseStmt");
            if (!stmt) {
                logger.Error("Production WhileStmt parse WhileStmt failed");
                exit(120);
            }
            return std::make_unique<StmtAST>(std::move(stmt));
        } else if (current == Token::BREAK || current == Token::CONTINUE || current == Token::RETURN) {
            Token temp = current;
            NextToken(); // Consume BREAK/CONTINUE/RETURN
            ASTPtr stmt;
            if (current == Token::SC)
                stmt = std::make_unique<ControlAST>(temp);
            else {
                ASTPtr retExp = ParseAddExp();
                logger.UnSetFunc("ParseStmt");
                if (!retExp) {
                    logger.Error("Production ReturnStmt parse return exp failed");
                    exit(121);
                }
                if (current != Token::SC) {
                    logger.Error("Lack semicolon");
                    exit(122);
                }
                stmt = std::make_unique<ControlAST>(Token::RETURN, std::move(retExp));
            }
            NextToken(); // consume SC
            return std::make_unique<StmtAST>(std::move(stmt));
        } else if (current == Token::SC) {
            NextToken(); // consume SC
            return std::make_unique<StmtAST>(nullptr);
        } else if (current == Token::LB) {
            ASTPtr block = ParseBlock();
            logger.UnSetFunc("ParseStmt");
            if (!block) {
                logger.Error("Production Block parse block failed");
                exit(123);
            }
            return std::make_unique<StmtAST>(std::move(block));
        } else {
            ASTPtr exp = ParseAddExp();
            logger.UnSetFunc("ParseStmt");
            if (!exp) {
                logger.Error(R"(Production Exp ";"/LVal "=" Exp ";" parse lval failed)");
                exit(124);
            }
            if (dynamic_cast<LValAST *>(exp.get())) {
                // LVal "=" Exp ";"
                if (current == Token::ASSIGN) {
                    NextToken(); // Consume =
                    ASTPtr rhs = ParseAddExp();
                    logger.UnSetFunc("ParseStmt");
                    if (!rhs) {
                        logger.Error(R"(Production Exp ";"/LVal "=" Exp ";" parse rhs failed)");
                        exit(125);
                    }
                    ASTPtr stmt = std::make_unique<AssignAST>(std::move(exp), std::move(rhs));
                    if (current != Token::SC) {
                        logger.Error("Lack semicolon");
                        exit(126);
                    }
                    NextToken(); // Consume sc
                    return std::make_unique<StmtAST>(std::move(stmt));
                } else if (current == Token::SC) {
                    // Exp;
                    NextToken(); // consume sc
                    return std::make_unique<StmtAST>(std::move(exp));
                } else {
                    logger.Error("Lack semicolon");
                    exit(127);
                }
            } else {
                // ;
                if (current != Token::SC) {
                    logger.Error("Lack semicolon");
                    exit(128);
                }
                NextToken(); // Consume SC
                return std::make_unique<StmtAST>(std::move(exp));
            }
        }
    }

/*
 * Block: "{" {BlockItem} "}"
 * BlockItem: Decl | Stmt;
 */
    ASTPtr Parser::ParseBlock() {
        logger.SetFunc("ParseBlock");
        NextToken(); // Consume LB
        if (current == Token::RB) {
            // {}
            NextToken();  // Consume RB
            return std::make_unique<BlockAST>(ASTPtrList{});
        } else {
            ASTPtrList list;
            while (current != Token::RB) {
                if (current == Token::CONST || current == Token::TYPE) {
                    ASTPtr decl = ParseVarDecl();
                    logger.UnSetFunc("ParseBlock");
                    if (!decl) {
                        logger.Error("Production Decl parse decl failed");
                        exit(129);
                    }
                    list.push_back(std::move(decl));
                } else {
                    ASTPtr stmt = ParseStmt();
                    logger.UnSetFunc("ParseBlock");
                    if (!stmt) {
                        logger.Error("Production Stmt parse stmt failed");
                        exit(130);
                    }
                    list.push_back(std::move(stmt));
                }
            }
            NextToken(); //consume RB
            return std::make_unique<BlockAST>(std::move(list));
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
                        logger.Error(R"(Production "{" [InitVal {"," InitVal}] "}" parse InitVal failed)");
                        exit(131);
                    }
                    initVals.push_back(std::move(init));
                    if (current != Token::CO)
                        break;
                    NextToken(); // consume CO
                }
                if (current != Token::RB) {
                    logger.Error(R"(Production "{" [InitVal {"," InitVal}] "}" lack right brace after InitVals)");
                    exit(132);
                }
                NextToken(); // consume RB
                return std::make_unique<InitValAST>(VarType::ARRAY, std::move(initVals));
            }
        } else {
            ASTPtr exp = ParseAddExp();
            logger.UnSetFunc("ParseInitVal");
            if (!exp) {
                logger.Error("Production Exp parse exp failed");
                exit(133);
            }
            ASTPtrList list;
            list.push_back(std::move(exp));
            return std::make_unique<InitValAST>(VarType::VAR, std::move(list));
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
        NextToken();  // Consume TYPE
        if (current != Token::IDENTIFIER) {
            logger.Error("Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lacked identifier");
            exit(134);
        }
        std::string name = lexer.getName();
        NextToken(); // Consume IDENTIFIER
        if (current != Token::LP) {
            logger.Error(
                    "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lacked left parentheses after identifier");
            exit(135);
        }
        NextToken(); // Consume LP
        ASTPtrList args;
        if (current != Token::RP) {
            while (true) {
                if (current != Token::TYPE || lexer.getType() != Type::INT) {
                    logger.Error("Production FuncType IDENT \"(\" [FuncFParams] \")\" Block wrong arg type");
                    exit(136);
                }
                NextToken(); // Consume TYPE
                if (current != Token::IDENTIFIER) {
                    logger.Error("BTYPE var lacked Identifier");
                    exit(137);
                }
                std::string argName = lexer.getName();
                NextToken(); // Consume IDENTIFIER
                if (current == Token::LSB) {
                    ASTPtrList dim;
                    dim.push_back(std::make_unique<NumberAST>(0));
                    NextToken(); // Consume LSB
                    if (current != Token::RSB) {
                        logger.Error(
                                "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lack right bracket in array arg");
                        exit(138);
                    }
                    NextToken(); // Consume RSB
                    while (current == Token::LSB) {
                        NextToken(); // Consume LSB
                        ASTPtr _dim = ParseAddExp();
                        logger.UnSetFunc("ParseFuncDef");
                        if (!_dim) {
                            logger.Error(
                                    R"(Production BType IDENT ["[" "]" {"[" ConstExp "]"}] parse exp dim failed)");
                            exit(139);
                        }
                        dim.push_back(std::move(_dim));
                        if (current != Token::RSB) {
                            logger.Error(
                                    R"(Production BType IDENT ["[" "]" {"[" ConstExp "]"}] lack right square bracket)");
                            exit(140);
                        }
                        NextToken(); // Consume RSB
                    }
                    args.push_back(std::make_unique<IdAST>(argName, VarType::ARRAY, false, std::move(dim)));
                } else {
                    args.push_back(std::make_unique<IdAST>(argName, VarType::VAR));
                }
                if (current != Token::CO)
                    break;
                NextToken(); // Consume CO
            }
            if (current != Token::RP) {
                logger.Error(
                        "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lacked right parentheses after identifier");
                exit(141);
            }
        }
        NextToken(); // Consume RP
        ASTPtr body = ParseBlock();
        logger.UnSetFunc("ParseFuncDef");
        return std::make_unique<FuncDefAST>(type, std::move(name), std::move(args), std::move(body));
    }

/*
 * VarDecl := [CONST] BType VarDef {"," VarDef} ";";
 * BType := INT
 */
    ASTPtr Parser::ParseVarDecl() {
        logger.SetFunc("ParseVarDecl");
        bool isConst = false;
        if (current == Token::CONST) {
            isConst = true;
            NextToken(); // Consume CONST
        }
        if (lexer.getType() != Type::INT) {
            logger.Error(R"(Production [CONST] BType VarDef {"," VarDef} ";" wrong BType)");
            exit(142);
        }
        NextToken(); // Consume TYPE
        ASTPtrList vars;
        ASTPtr varDef = ParseVarDef(isConst);
        logger.UnSetFunc("ParseVarDecl");
        if (!varDef) {
            logger.Error(R"(Production [CONST] BType VarDef {"," VarDef} ";" parse vardef failed)");
            exit(143);
        }
        vars.push_back(std::move(varDef));
        while (current == Token::CO) {
            NextToken(); // Consume CO
            varDef = ParseVarDef(isConst);
            logger.UnSetFunc("ParseVarDecl");
            if (!varDef) {
                logger.Error(R"(Production [CONST] BType VarDef {"," VarDef} ";" parse vardef failed)");
                exit(144);
            }
            vars.push_back(std::move(varDef));
        }
        if (current != Token::SC) {
            logger.Error("lack semicolon");
            exit(145);
        }
        NextToken(); // Consume SC
        return std::make_unique<VarDeclAST>(isConst, std::move(vars));
    }

/*
 * VarDef := IDENT {"[" ConstExp "]"} ["=" InitVal]
 * ConstDef: IDENT {"[" ConstExp "]"} "=" ConstInitVal;
 */
    ASTPtr Parser::ParseVarDef(bool isConst) {
        logger.SetFunc("ParseVarDef");
        if (current != Token::IDENTIFIER) {
            logger.Error(R"(Production IDENT {"[" ConstExp "]"} ["=" InitVal] lacked identifier)");
            exit(146);
        }
        std::string name = lexer.getName();
        ASTPtrList dims;
        NextToken(); // Consume IDENTIFIER
        while (current == Token::LSB) {
            NextToken(); // Consume LSB
            ASTPtr exp = ParseAddExp();
            logger.UnSetFunc("ParseVarDef");
            if (!exp) {
                logger.Error(R"(Production IDENT {"[" ConstExp "]"} ["=" InitVal] parse exp failed)");
                exit(147);
            }
            dims.push_back(std::move(exp));
            if (current != Token::RSB) {
                logger.Error(
                        R"(Production IDENT {"[" ConstExp "]"} ["=" InitVal] lack right square bracket after exp)");
                exit(148);
            }
            NextToken(); // Consume RSB
        }
        ASTPtr var;
        if (dims.empty())
            var = std::make_unique<IdAST>(name, VarType::VAR, isConst);
        else
            var = std::make_unique<IdAST>(name, VarType::ARRAY, isConst, std::move(dims));
        if (current == Token::ASSIGN) {
            NextToken(); // Consume ASSIGN
            ASTPtr init = ParseInitVal();
            logger.UnSetFunc("ParseVarDef");
            if (!init) {
                logger.Error(R"(Production IDENT {"[" ConstExp "]"} ["=" InitVal] parse init failed)");
                exit(149);
            }
            return std::make_unique<VarDefAST>(isConst, std::move(var), std::move(init));
        } else {
            if (isConst) {
                logger.Error(R"(Production ConstDef: IDENT {"[" ConstExp "]"} "=" ConstInitVal lacked assignment)");
                exit(150);
            }
            return std::make_unique<VarDefAST>(isConst, std::move(var));
        }
    }

/*
 * CompUnit := (Decl | FuncDef) [CompUnit];
 */
    ASTPtr Parser::ParseCompUnit() {
        logger.SetFunc("ParseCompUnit");
        NextToken(); // read first token
        ASTPtrList nodes;
        while (current != Token::END) {
            if (current == Token::CONST) {
                ASTPtr decl = ParseVarDecl();
                logger.UnSetFunc("ParseCompUnit");
                if (!decl) {
                    logger.Error("Production (Decl | FuncDef) [CompUnit] parse decl failed");
                    exit(151);
                }
                nodes.push_back(std::move(decl));
            } else if (current == Token::TYPE) {
                if (lexer.getType() == Type::VOID) {
                    ASTPtr func = ParseFuncDef();
                    logger.UnSetFunc("ParseCompUnit");
                    if (!func) {
                        logger.Error("Production (Decl | FuncDef) [CompUnit] parse func failed");
                        exit(152);
                    }
                    nodes.push_back(std::move(func));
                } else {
                    Type type = lexer.getType();
                    NextToken(); // Consume TYPE
                    if (current != Token::IDENTIFIER) {
                        logger.Error("lack identifier");
                        exit(153);
                    }
                    std::string name = lexer.getName();
                    NextToken(); // Consume IDENTIFIER
                    if (current == Token::LP) {
                        // parse funcDef
                        NextToken(); // Consume LP
                        ASTPtrList args;
                        if (current != Token::RP) {
                            while (true) {
                                if (current != Token::TYPE || lexer.getType() != Type::INT) {
                                    logger.Error(
                                            "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block wrong arg type");
                                    exit(154);
                                }
                                NextToken(); // Consume TYPE
                                if (current != Token::IDENTIFIER) {
                                    logger.Error("Production BTYPE IDENTIFIER lacked identifier");
                                    exit(155);
                                }
                                std::string argName = lexer.getName();
                                NextToken(); // Consume IDENTIFIER
                                if (current == Token::LSB) {
                                    ASTPtrList dim;
                                    dim.push_back(std::make_unique<NumberAST>(0));
                                    NextToken(); // Consume LSB
                                    if (current != Token::RSB) {
                                        logger.Error(
                                                "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lack right bracket in array arg");
                                        exit(156);
                                    }
                                    NextToken();  // Consume RSB
                                    while (current == Token::LSB) {
                                        NextToken(); // Consume LSB
                                        ASTPtr _dim = ParseAddExp();
                                        logger.UnSetFunc("ParseCompUnit");
                                        if (!_dim) {
                                            logger.Error(
                                                    R"(Production BType IDENT ["[" "]" {"[" ConstExp "]"}] parse exp dim failed)");
                                            exit(157);
                                        }
                                        dim.push_back(std::move(_dim));
                                        if (current != Token::RSB) {
                                            logger.Error(
                                                    R"(Production BType IDENT ["[" "]" {"[" ConstExp "]"}] lack right square bracket)");
                                            exit(158);
                                        }
                                        NextToken(); // Consume RSB
                                    }
                                    args.push_back(std::make_unique<IdAST>(argName, VarType::ARRAY, false, std::move(dim)));
                                } else {
                                    args.push_back(std::make_unique<IdAST>(argName, VarType::VAR));
                                }
                                if (current != Token::CO)
                                    break;
                                NextToken(); // consume CO
                            }
                            if (current != Token::RP) {
                                logger.Error(
                                        "Production FuncType IDENT \"(\" [FuncFParams] \")\" Block lacked right parentheses after identifier");
                                exit(159);
                            }
                        }
                        NextToken();  // Consume RP
                        ASTPtr body = ParseBlock();
                        logger.UnSetFunc("ParseCompUnit");
                        ASTPtr func = std::make_unique<FuncDefAST>(type, std::move(name), std::move(args), std::move(body));
                        nodes.push_back(std::move(func));
                    } else {
                        ASTPtrList varDefs;
                        // parse first var def
                        ASTPtrList dims;
                        while (current == Token::LSB) {
                            NextToken(); // Consume LSB
                            ASTPtr exp = ParseAddExp();
                            logger.UnSetFunc("ParseCompUnit");
                            if (!exp) {
                                logger.Error(R"(Production IDENT {"[" ConstExp "]"} ["=" InitVal] parse exp failed)");
                                exit(160);
                            }
                            dims.push_back(std::move(exp));
                            if (current != Token::RSB) {
                                logger.Error(
                                        R"(Production IDENT {"[" ConstExp "]"} ["=" InitVal] lack right square bracket after exp)");
                                exit(161);
                            }
                            NextToken(); // Consume RSB
                        }
                        ASTPtr var;
                        ASTPtr varDef;
                        if (dims.empty())
                            var = std::make_unique<IdAST>(name, VarType::VAR, false);
                        else
                            var = std::make_unique<IdAST>(name, VarType::ARRAY, false, std::move(dims));
                        if (current == Token::ASSIGN) {
                            NextToken(); // Consume ASSIGN
                            ASTPtr init = ParseInitVal();
                            logger.UnSetFunc("ParseCompUnit");
                            if (!init) {
                                logger.Error(R"(Production IDENT {"[" ConstExp "]"} ["=" InitVal] parse init failed)");
                                exit(162);
                            }
                            varDef = std::make_unique<VarDefAST>(false, std::move(var), std::move(init));
                        } else {
                            varDef = std::make_unique<VarDefAST>(false, std::move(var));
                        }
                        varDefs.push_back(std::move(varDef));
                        while (current == Token::CO) {
                            NextToken(); // Consume CO
                            varDef = ParseVarDef(false);
                            logger.UnSetFunc("ParseCompUnit");
                            if (!varDef) {
                                logger.Error(R"(Production [CONST] BType VarDef {"," VarDef} ";" parse vardef failed)");
                                exit(163);
                            }
                            varDefs.push_back(std::move(varDef));
                        }
                        if (current != Token::SC) {
                            logger.Error("lack semicolon");
                            exit(164);
                        }
                        ASTPtr decl = std::make_unique<VarDeclAST>(false, std::move(varDefs));
                        nodes.push_back(std::move(decl));
                        NextToken();  // Consume SC
                    }
                }
            } else {
                logger.Error("Wrong token");
                exit(165);
            }
        }
        return std::make_unique<CompUnitAST>(std::move(nodes));
    }
}
