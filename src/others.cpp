/*
 * ConstExp := AddExp;
 */
ASTPtr Parser::EvalConstExp() {

}

ASTPtr Parser::EvalMulExp() {
    ASTPtr lval = EvalUnaryExp();
    if (!dynamic_pointer_cast<NumberAST>(lval))
        return nullptr;
    Operator op;
    while (current == Token::OPERATOR) {
        op = lexer.getOp();
        NextToken();
        ASTPtr rval = EvalUnaryExp();
        if (!rval || !dynamic_pointer_cast<NumberAST>(rval))
            return nullptr;
        if (!dynamic_pointer_cast<NumberAST>(lval))
            return nullptr;
        switch(op) {
            case Operator::MUL {
                        lval = std::make_unique<NumberAST>(
                                dynamic_pointer_cast<NumberAST>(lval)->getVal()
                                * dynamic_pointer_cast<NumberAST>(rval)->getVal());
                        break;
                }
            case Operator::DIV {
                        if (dynamic_pointer_cast<NumberAST>(rval)->getVal() == 0)
                        return nullptr;
                        lval = std::make_unique<NumberAST>(
                        dynamic_pointer_cast<NumberAST>(lval)->getVal()
                        / dynamic_pointer_cast<NumberAST>(rval)->getVal());
                        break;
                }
            case Operator::MOD {
                        if (dynamic_pointer_cast<NumberAST>(rval)->getVal() == 0)
                        return nullptr;
                        lval = std::make_unique<NumberAST>(
                        dynamic_pointer_cast<NumberAST>(lval)->getVal()
                        % dynamic_pointer_cast<NumberAST>(rval)->getVal());
                        break;
                }
            default: return nullptr;
        }
    }
    return lval;
}

ASTPtr Parser::EvalAddExp() {
    ASTPtr lval = EvalMulExp();
    if (!dynamic_pointer_cast<NumberAST>(lval))
        return nullptr;
    Operator op;
    while (current == Token::OPERATOR) {
        op = lexer.getOp();
        NextToken();
        ASTPtr rval = EvalMulExp();
        if (!rval || !dynamic_pointer_cast<NumberAST>(rval))
            return nullptr;
        if (!dynamic_pointer_cast<NumberAST>(lval))
            return nullptr;
        switch(op) {
            case Operator::ADD {
                        lval = std::make_unique<NumberAST>(
                                dynamic_pointer_cast<NumberAST>(lval)->getVal()
                                + dynamic_pointer_cast<NumberAST>(rval)->getVal());
                        break;
                }
            case Operator::SUB {
                        lval = std::make_unique<NumberAST>(
                                dynamic_pointer_cast<NumberAST>(lval)->getVal()
                                - dynamic_pointer_cast<NumberAST>(rval)->getVal());
                        break;
                }
            default: return nullptr;
        }
    }
    return lval;
}

/*
 * UnaryExp := "(" Exp ")"
 * | IDENTIFIER {"[" Exp "]"}
 * | NUMBER
 * | IDENTIFIER "(" [FunRParams] ")"
 * | ("+" | "-" | "!") UnaryExp;
 */
ASTPtr Parser::EvalUnaryExp() {
    if (current == Token::LP) {
        NextToken();
        ASTPtr exp = EvalAddExp();
        if (!exp || !std::dynamic_pointer_cast<NumberAST>(exp))
            return nullptr;
        if (current != Token::RP)
            return nullptr;
        return exp;
    } else if (current == Token::OPERATOR) {
        Operator op = lexer.getOp();
        NextToken();
        ASTPtr exp = EvalUnaryExp();
        if (!exp || !std::dynamic_pointer_cast<NumberAST>(exp))
            return nullptr;
        long long expVal = std::dynamic_pointer_cast<NumberAST>(exp)->getVal();
        ASTPtr retExp;
        switch(lexer.getOp()) {
            case Operator::ADD:{
                retExp = std::make_unique<NumberAST>(expVal);
                break;
            }
            case Operator::SUB:{
                retExp = std::make_unique<NumberAST>(-expVal);
                break;
            }
            case Operator::NOT:{
                if (expVal == 0)
                    retExp = std::make_unique<NumberAST>(1);
                else
                    retExp = std::make_unique<NumberAST>(0);
            }
        }
        return retExp;
    } else if (current == Token::NUMBER){
        return std::make_unique<NumberAST>(lexer.getVal());
    } else if (current == Token::IDENTIFIER){
        std::string varName = lexer.getName();
        int i;
        for (i = 0; i < constVals.size(); i++) {
            if (constVals[i].first == varName) {
                break;
            }
        }
        if (i == constVals.size())
            return nullptr;

    } else {
        //ERROR
        return nullptr;
    }
}
