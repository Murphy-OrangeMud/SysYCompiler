#include <back/eeyore_parser.hpp>
#include <string>
#include <utility>
#include <memory>
#include <iostream>
#include <define/ir.hpp>

namespace EeyoreToTigger {
    void Parser::NextToken() {
        current = lexer.NextToken();
        logger.Info("NextToken: " + std::to_string(current));
        if (current == Token::SYMBOL) {
            logger.Info("Symbol name: " + lexer.getName());
        }
        if (current == Token::LOGICOP || current == Token::OP) {
            logger.Info("Operator: " + std::to_string(lexer.getOp()));
        }
    }

    /*
     * Declaration ::= "var" [NUM] SYMBOL;
     */
    IRPtr Parser::ParseDecl() {
        logger.SetFunc("ParseDecl");
        if (current != Token::VARDECL) {
            logger.Error("Declaration::= \"var\" [NUM] SYMBOL lack var");
            exit(201);
        }
        NextToken();
        std::string name;
        if (current == Token::NUMBER) {
            int size = lexer.getVal();
            NextToken();
            name = lexer.getName();
            NextToken(); // consume name
            return std::make_unique<DeclIR>(VarType::ARRAY, size, name, lexer.getLineno() - 1);
        } else {
            name = lexer.getName();
            NextToken(); // consume name
            return std::make_unique<DeclIR>(VarType::VAR, 0, name, lexer.getLineno() - 1);
        }
    }

    /*
     * Initialization  ::= SYMBOL "=" NUM
                  | SYMBOL "[" NUM "]" "=" NUM;
     */
    IRPtr Parser::ParseInit() {
        logger.SetFunc("ParseInit");
        if (current != Token::SYMBOL) {
            logger.Error("Parse Init lack symbol");
            exit(202);
        }
        std::string name = lexer.getName();
        NextToken();
        VarType type = VarType::VAR;
        int pos = -1;
        if (current == Token::LSB) {
            type = VarType::ARRAY;
            NextToken();
            if (current != Token::NUMBER) {
                logger.Error("Initialization pos must be number");
                exit(203);
            }
            pos = lexer.getVal();
            NextToken();
            if (current != Token::RSB) {
                logger.Error("Initialization lack right square bracket");
                exit(204);
            }
            NextToken(); // consume RSB
        }
        if (current != Token::ASSIGN) {
            logger.Error("initialization lack =");
            exit(205);
        }
        NextToken(); // consume assign
        if (current != Token::NUMBER) {
            logger.Error("initialization rhs not number");
            exit(206);
        }
        int value = lexer.getVal();
        NextToken(); // consume value

        return std::make_unique<InitIR>(type, name, pos, value, lexer.getLineno() - 1);
    }

    /*
     * Program ::= {Declaration | Initialization | FunctionDef};
     */
    IRPtr Parser::ParseProgram() {
        logger.SetFunc("ParseProgram");
        NextToken(); // get first token
        IRPtrList nodes;
        while (current != Token::END) {
            if (current == Token::VARDECL) {
                nodes.push_back(ParseDecl());
                logger.UnSetFunc("ParseProgram");
            } else if (current == Token::SYMBOL) {
                std::string name = lexer.getName();
                if (name[0] == 'f' && name[1] == '_') {
                    nodes.push_back(ParseFuncDef());
                    logger.UnSetFunc("ParseProgram");
                } else {
                    nodes.push_back(ParseInit());
                    logger.UnSetFunc("ParseProgram");
                }
            } else {
                logger.Error("Wrong program block");
                exit(207);
            }
        }
        return std::make_unique<ProgramIR>(std::move(nodes), lexer.getLineno());
    }

    /*
     * FunctionDef     ::= FunctionHeader Statements FunctionEnd;
     * FunctionHeader  ::= FUNCTION "[" NUM "]";
     * Statements      ::= {Statement};
     * FunctionEnd     ::= "end" FUNCTION;
     */
    IRPtr Parser::ParseFuncDef() {
        logger.SetFunc("ParseFuncDef");
        if (current != Token::SYMBOL || lexer.getName()[0] != 'f' || lexer.getName()[1] != '_') {
            logger.Error("Function name lacked");
            exit(208);
        }
        std::string funcName = lexer.getName();
        NextToken();
        if (current != Token::LSB) {
            logger.Error("function param number lacked lsb");
            exit(209);
        }
        NextToken();
        if (current != Token::NUMBER) {
            logger.Error("function param number define wrong");
            exit(210);
        }
        int param = lexer.getVal();
        NextToken();
        if (current != Token::RSB) {
            logger.Error("function param number lacked rsb");
            exit(211);
        }
        NextToken(); // consume RSB
        IRPtr body = ParseStatements();
        logger.UnSetFunc("ParseFuncDef");
        if (current != Token::FUNCEND) {
            logger.Error("function end wrong lacked end");
            exit(lexer.getName()[0]);
        }
        NextToken();
        if (current != Token::SYMBOL || lexer.getName() != funcName) {
            logger.Error("Function end wrong function name");
            exit(213);
        }
        NextToken(); // consume name

        return std::make_unique<FuncDefIR>(funcName, param, std::move(body), lexer.getLineno() - 1);
    }

    /*
     * Statements ::= {Statement};
     */
    IRPtr Parser::ParseStatements() {
        logger.SetFunc("ParseStatements");
        IRPtrList stmts;
        while (true) {
            if (current == Token::VARDECL) {
                stmts.push_back(ParseDecl());
                logger.UnSetFunc("ParseStatements");
            } else if (current == Token::SYMBOL) {
                if (lexer.getName()[0] == 'l') {
                    stmts.push_back(ParseLabel());
                    logger.UnSetFunc("ParseStatements");
                } else {
                    stmts.push_back(ParseAssign());
                    logger.UnSetFunc("ParseStatements");
                }
            } else if (current == Token::IF) {
                stmts.push_back(ParseCondGoto());
                logger.UnSetFunc("ParseStatements");
            } else if (current == Token::GOTO) {
                stmts.push_back(ParseGoto());
                logger.UnSetFunc("ParseStatements");
            } else if (current == Token::PARAM) {
                stmts.push_back(ParseParams());
                logger.UnSetFunc("ParseStatements");
            } else if (current == Token::CALL) {
                stmts.push_back(ParseFuncCall());
                logger.UnSetFunc("ParseStatements");
            } else if (current ==  Token::RETURN) {
                stmts.push_back(ParseReturn());
                logger.UnSetFunc("ParseStatements");
            } else {
                break;
            }
        }
        return std::make_unique<StatementsIR>(std::move(stmts), lexer.getLineno() - 1);
    }

    /*
     * SYMBOL "=" RightValue BinOp RightValue
     * SYMBOL "=" OP RightValue
     * LVal "=" RightValue
     * SYMBOL "=" SYMBOL "[" RightValue "]"
     * SYMBOL "=" "call" FUNCTION
     */

    IRPtr Parser::ParseAssign() {
        logger.SetFunc("ParseAssign");
        if (current != Token::SYMBOL) {
            logger.Error("assign lhs not symbol");
            exit(214);
        }
        IRPtr lhs = ParseLVal();
        logger.UnSetFunc("ParseAssign");
        if (current != Token::ASSIGN) {
            logger.Error("assign not =");
            exit(215);
        }
        NextToken(); // consume =
        IRPtr rhs;
        if (current == Token::OP) {
            // SYMBOL "=" OP RightValue
            Operator op = lexer.getOp();
            NextToken(); // consume OP
            IRPtr rValue;
            if (current == Token::SYMBOL) {
                if (dynamic_cast<LValIR *>(lhs.get())->getType() == VarType::ARRAY) {
                    logger.Error("Wrong type, lhs must be variable when rhs is not rightvalue");
                    exit(216);
                }
                rValue = std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno() - 1);
            } else if (current == Token::NUMBER) {
                rValue = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno() - 1);
            } else if (current == Token::OP) {
                Operator op2 = lexer.getOp();
                NextToken();
                if (current != Token::NUMBER) {
                    logger.Error("Wrong unary op with var, not right value");
                    exit(241);
                }
                switch(op2) {
                    case Operator::ADD: {
                        rValue = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                        break;
                    }
                    case Operator::SUB: {
                        rValue = std::make_unique<RightValIR>(current, -lexer.getVal(), lexer.getLineno());
                        break;
                    }
                    case Operator::NOT: {
                        rValue = std::make_unique<RightValIR>(current, !lexer.getVal(), lexer.getLineno());
                        break;
                    }
                    default: {
                        logger.Error("Wrong unary op, Not right value");
                        exit(242);
                    }
                }
            } else {
                logger.Error("Not right value");
                exit(217);
            }
            NextToken(); // consume right value
            rhs = std::make_unique<UnaryExpIR>(std::move(rValue), op, lexer.getLineno() - 1);
            return std::make_unique<AssignIR>(std::move(lhs), std::move(rhs), lexer.getLineno() - 1);
        } else if (current == Token::NUMBER) {
            IRPtr rValue_1 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
            NextToken();
            if (current == Token::OP || current == Token::LOGICOP) {
                // SYMBOL "=" RightValue BinOp RightValue
                if (dynamic_cast<LValIR *>(lhs.get())->getType() == VarType::ARRAY) {
                    logger.Error("Wrong type, lhs must be variable when rhs is not rightvalue");
                    exit(218);
                }
                Operator op = lexer.getOp();
                NextToken();
                IRPtr rValue_2;
                if (current == Token::SYMBOL) {
                    rValue_2 = std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno());
                } else if (current == Token::NUMBER) {
                    rValue_2 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                } else if (current == Token::OP) {
                    Operator op2 = lexer.getOp();
                    NextToken();
                    if (current != Token::NUMBER) {
                        logger.Error("Wrong unary op with var, not right value");
                        exit(241);
                    }
                    switch(op2) {
                        case Operator::ADD: {
                            rValue_2 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                            break;
                        }
                        case Operator::SUB: {
                            rValue_2 = std::make_unique<RightValIR>(current, -lexer.getVal(), lexer.getLineno());
                            break;
                        }
                        case Operator::NOT: {
                            rValue_2 = std::make_unique<RightValIR>(current, !lexer.getVal(), lexer.getLineno());
                            break;
                        }
                        default: {
                            logger.Error("Wrong unary op, Not right value");
                            exit(242);
                        }
                    }
                } else {
                    logger.Error("Not right value");
                    exit(219);
                }
                NextToken();
                rhs = std::make_unique<BinaryExpIR>(std::move(rValue_1), std::move(rValue_2), op, lexer.getLineno() - 1);
                return std::make_unique<AssignIR>(std::move(lhs), std::move(rhs), lexer.getLineno() - 1);
            } else {
                // LVal "=" RightValue
                rhs = std::move(rValue_1);
                return std::make_unique<AssignIR>(std::move(lhs), std::move(rhs), lexer.getLineno() - 1);
            }
        } else if (current == Token::SYMBOL) {
            std::string name = lexer.getName();
            NextToken();
            if (current == Token::LSB) {
                // SYMBOL "=" SYMBOL "[" RightValue "]"
                if (dynamic_cast<LValIR *>(lhs.get())->getType() == VarType::ARRAY) {
                    logger.Error("Wrong type, lhs must be variable when rhs is not rightvalue");
                    exit(220);
                }
                IRPtr rValue;
                NextToken(); // consume LSB
                if (current == Token::SYMBOL) {
                    rValue = std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno());
                } else if (current == Token::NUMBER) {
                    rValue = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                } else if (current == Token::OP) {
                    Operator op2 = lexer.getOp();
                    NextToken();
                    if (current != Token::NUMBER) {
                        logger.Error("Wrong unary op with var, not right value");
                        exit(241);
                    }
                    switch(op2) {
                        case Operator::ADD: {
                            rValue = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                            break;
                        }
                        case Operator::SUB: {
                            rValue = std::make_unique<RightValIR>(current, -lexer.getVal(), lexer.getLineno());
                            break;
                        }
                        case Operator::NOT: {
                            rValue = std::make_unique<RightValIR>(current, !lexer.getVal(), lexer.getLineno());
                            break;
                        }
                        default: {
                            logger.Error("Wrong unary op, Not right value");
                            exit(242);
                        }
                    }
                } else {
                    logger.Error("Not right value");
                    exit(221);
                }
                NextToken();
                if (current != Token::RSB) {
                    logger.Error("LVal pos lacked rsb");
                    exit(222);
                }
                NextToken(); // consume RSB
                rhs = std::make_unique<LValIR>(VarType::ARRAY, name, lexer.getLineno() - 1, std::move(rValue));
                return std::make_unique<AssignIR>(std::move(lhs), std::move(rhs), lexer.getLineno() - 1);
            } else {
                IRPtr rValue_1 = std::make_unique<RightValIR>(Token::SYMBOL, name, lexer.getLineno());
                if (current == Token::OP || current == Token::LOGICOP) {
                    // SYMBOL "=" RightValue BinOp RightValue
                    if (dynamic_cast<LValIR *>(lhs.get())->getType() == VarType::ARRAY) {
                        logger.Error("Wrong type, lhs must be variable when rhs is not rightvalue");
                        exit(223);
                    }
                    Operator op = lexer.getOp();
                    NextToken(); // consume OP
                    IRPtr rValue_2;
                    if (current == Token::SYMBOL) {
                        rValue_2 = std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno());
                    } else if (current == Token::NUMBER) {
                        rValue_2 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                    } else if (current == Token::OP) {
                        Operator op2 = lexer.getOp();
                        NextToken();
                        if (current != Token::NUMBER) {
                            logger.Error("Wrong unary op with var, not right value");
                            exit(241);
                        }
                        switch(op2) {
                            case Operator::ADD: {
                                rValue_2 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                                break;
                            }
                            case Operator::SUB: {
                                rValue_2 = std::make_unique<RightValIR>(current, -lexer.getVal(), lexer.getLineno());
                                break;
                            }
                            case Operator::NOT: {
                                rValue_2 = std::make_unique<RightValIR>(current, !lexer.getVal(), lexer.getLineno());
                                break;
                            }
                            default: {
                                logger.Error("Wrong unary op, Not right value");
                                exit(242);
                            }
                        }
                    } else {
                        logger.Error("Not right value");
                        exit(224);
                    }
                    NextToken();
                    rhs = std::make_unique<BinaryExpIR>(std::move(rValue_1), std::move(rValue_2), op, lexer.getLineno() - 1);
                    return std::make_unique<AssignIR>(std::move(lhs), std::move(rhs), lexer.getLineno() - 1);
                } else {
                    // LVal "=" RightValue
                    rhs = std::move(rValue_1);
                    return std::make_unique<AssignIR>(std::move(lhs), std::move(rhs), lexer.getLineno() - 1);
                }
            }
        } else if (current == Token::CALL) {
            // SYMBOL "=" "call" FUNCTION
            if (dynamic_cast<LValIR *>(lhs.get())->getType() == VarType::ARRAY) {
                logger.Error("Wrong type, lhs must be variable when rhs is not rightvalue");
                exit(225);
            }
            rhs = ParseFuncCall();
            logger.UnSetFunc("ParseAssign");
            return std::make_unique<AssignIR>(std::move(lhs), std::move(rhs), lexer.getLineno() - 1);
        }
    }

    /*
     * LVal := SYMBOL "[" RightValue "]" | SYMBOL
     */
    IRPtr Parser::ParseLVal() {
        logger.SetFunc("ParseLVal");
        if (current != Token::SYMBOL) {
            logger.Error("LVal lacked symbol");
            exit(226);
        }
        std::string name = lexer.getName();
        NextToken();
        if (current == Token::LSB) {
            IRPtr rValue;
            NextToken(); // consume LSB
            if (current == Token::SYMBOL) {
                rValue = std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno());
            } else if (current == Token::NUMBER) {
                rValue = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
            } else if (current == Token::OP) {
                Operator op2 = lexer.getOp();
                NextToken();
                if (current != Token::NUMBER) {
                    logger.Error("Wrong unary op with var, not right value");
                    exit(241);
                }
                switch(op2) {
                    case Operator::ADD: {
                        rValue = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                        break;
                    }
                    case Operator::SUB: {
                        rValue = std::make_unique<RightValIR>(current, -lexer.getVal(), lexer.getLineno());
                        break;
                    }
                    case Operator::NOT: {
                        rValue = std::make_unique<RightValIR>(current, !lexer.getVal(), lexer.getLineno());
                        break;
                    }
                    default: {
                        logger.Error("Wrong unary op, Not right value");
                        exit(242);
                    }
                }
            } else {
                logger.Error("Not right value");
                exit(227);
            }
            NextToken();
            if (current != Token::RSB) {
                logger.Error("LVal pos lacked rsb");
                exit(228);
            }
            NextToken(); // consume RSB
            return std::make_unique<LValIR>(VarType::ARRAY, name, lexer.getLineno(), std::move(rValue));
        } else {
            return std::make_unique<LValIR>(VarType::VAR, name, lexer.getLineno());
        }
    }

    /*
     * "call" FUNCTION
     */
    IRPtr Parser::ParseFuncCall() {
        logger.SetFunc("ParseFuncCall");
        if (current != Token::CALL) {
            logger.Error("Func call lack call");
            exit(229);
        }
        NextToken();
        if (current != Token::SYMBOL) {
            logger.Error("Func call lack symbol");
            exit(230);
        }
        std::string funcName = lexer.getName();
        NextToken(); // consume symbol
        return std::make_unique<FuncCallIR>(funcName, lexer.getLineno() - 1);
    }

    /*
     * "return" RightValue
     * "return"
     */
    IRPtr Parser::ParseReturn() {
        logger.SetFunc("ParseReturn");
        if (current != Token::RETURN) {
            logger.Error("Return statement lack return");
            exit(231);
        }
        NextToken();
        IRPtr exp;
        if (current == Token::SYMBOL) {
            exp = std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno());
            NextToken();
        } else if (current == Token::NUMBER) {
            exp = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
            NextToken();
        } else {
            exp = nullptr;
        }
        return std::make_unique<ReturnIR>(std::move(exp), lexer.getLineno() - 1);
    }

    /*
     * param RightValue
     */
    IRPtr Parser::ParseParams() {
        logger.SetFunc("ParseParams");
        IRPtrList params;
        while (current == Token::PARAM) {
            NextToken();
            if (current == Token::SYMBOL) {
                params.push_back(std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno()));
            } else if (current == Token::NUMBER) {
                params.push_back(std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno()));
            } else if (current == Token::OP) {
                Operator op2 = lexer.getOp();
                NextToken();
                if (current != Token::NUMBER) {
                    logger.Error("Wrong unary op with var, not right value");
                    exit(241);
                }
                switch(op2) {
                    case Operator::ADD: {
                        params.push_back(std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno()));
                        break;
                    }
                    case Operator::SUB: {
                        params.push_back(std::make_unique<RightValIR>(current, -lexer.getVal(), lexer.getLineno()));
                        break;
                    }
                    case Operator::NOT: {
                        params.push_back(std::make_unique<RightValIR>(current, !lexer.getVal(), lexer.getLineno()));
                        break;
                    }
                    default: {
                        logger.Error("Wrong unary op, Not right value");
                        exit(242);
                    }
                }
            }
            NextToken();
        }
        return std::make_unique<ParamListIR>(std::move(params), lexer.getLineno() - 1);
    }

    IRPtr Parser::ParseGoto() {
        logger.SetFunc("ParseGoto");
        if (current != Token::GOTO) {
            logger.Error("Parse goto lack goto");
            exit(232);
        }
        NextToken();
        if (current != Token::SYMBOL || lexer.getName()[0] != 'l') {
            logger.Error("goto missed label");
            exit(233);
        }
        int num = stoi(lexer.getName().substr(1, lexer.getName().length() - 1));
        NextToken();
        return std::make_unique<GotoIR>(num, lexer.getLineno() - 1);
    }

    IRPtr Parser::ParseCondGoto() {
        logger.SetFunc("ParseCondGoto");
        if (current != Token::IF) {
            logger.Error("conditional goto lack if");
            exit(234);
        }
        NextToken(); // conusme if
        IRPtr r1, r2;
        if (current == Token::SYMBOL) {
            r1 = std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno());
        } else if (current == Token::NUMBER) {
            r1 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
        } else if (current == Token::OP) {
            Operator op2 = lexer.getOp();
            NextToken();
            if (current != Token::NUMBER) {
                logger.Error("Wrong unary op with var, not right value");
                exit(241);
            }
            switch(op2) {
                case Operator::ADD: {
                    r1 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                    break;
                }
                case Operator::SUB: {
                    r1 = std::make_unique<RightValIR>(current, -lexer.getVal(), lexer.getLineno());
                    break;
                }
                case Operator::NOT: {
                    r1 = std::make_unique<RightValIR>(current, !lexer.getVal(), lexer.getLineno());
                    break;
                }
                default: {
                    logger.Error("Wrong unary op, Not right value");
                    exit(242);
                }
            }
        } else {
            logger.Error("Not right value");
            exit(235);
        }
        NextToken(); // consume symbol
        if (current != Token::LOGICOP) {
            logger.Error("Not logical operation in condition");
            exit(236);
        }
        Operator op = lexer.getOp();
        NextToken(); // consume op
        if (current == Token::SYMBOL) {
            r2 = std::make_unique<RightValIR>(current, lexer.getName(), lexer.getLineno());
        } else if (current == Token::NUMBER) {
            r2 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
        } else if (current == Token::OP) {
            Operator op2 = lexer.getOp();
            NextToken();
            if (current != Token::NUMBER) {
                logger.Error("Wrong unary op with var, not right value");
                exit(243);
            }
            switch(op2) {
                case Operator::ADD: {
                    r1 = std::make_unique<RightValIR>(current, lexer.getVal(), lexer.getLineno());
                    break;
                }
                case Operator::SUB: {
                    r1 = std::make_unique<RightValIR>(current, -lexer.getVal(), lexer.getLineno());
                    break;
                }
                case Operator::NOT: {
                    r1 = std::make_unique<RightValIR>(current, !lexer.getVal(), lexer.getLineno());
                    break;
                }
                default: {
                    logger.Error("Wrong unary op, Not right value");
                    exit(244);
                }
            }
        } else {
            logger.Error("not right value");
            exit(237);
        }

        IRPtr cond = std::make_unique<BinaryExpIR>(std::move(r1), std::move(r2), op, lexer.getLineno());
        NextToken(); // consume rightvalue
        if (current != Token::GOTO) {
            logger.Error("Parse cond goto lack goto");
            exit(238);
        }
        NextToken();
        if (current != Token::SYMBOL || lexer.getName()[0] != 'l') {
            logger.Error("cond goto missed label");
            exit(239);
        }
        int num = stoi(lexer.getName().substr(1, lexer.getName().length() - 1));
        NextToken();

        return std::make_unique<CondGotoIR>(std::move(cond), num, lexer.getLineno() - 1);
    }

    IRPtr Parser::ParseLabel() {
        logger.SetFunc("ParseLabel");
        if (current != Token::SYMBOL || lexer.getName()[0] != 'l') {
            logger.Error("label missed symbol");
            exit(239);
        }
        int num = stoi(lexer.getName().substr(1, lexer.getName().length() - 1));
        IRPtr label = std::make_unique<Label>(num, lexer.getLineno());
        NextToken();
        if (current != Token::COLON) {
            logger.Error("label missed colon");
            exit(240);
        }
        NextToken(); // consume colon
        return label;
    }
}
