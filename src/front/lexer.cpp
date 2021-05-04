#include <iostream>
#include <string>
#include "lexer.hpp"

Token Lexer::parseInt() {
    char c = std::cin.peek();
    int val = 0;
    if (c == '0') {
        c = std::cin.get();
        c = std::cin.peek();
        if (c == 'x' || c == 'X') {
            // hexadecimal const
            c = std::cin.get();
            while (true) {
                c = std::cin.peek();
                if (c >= '0' && c <= '9') {
                    c = std::cin.get();
                    val = val * 16 + c - '0';
                } else if (c >= 'A' && c <= 'F') {
                    c = std::cin.get();
                    val = val * 16 + c - 'A' + 10;
                } else if (c >= 'a' && c <= 'f') {
                    c = std::cin.get();
                    val = val * 16 + c - 'a' + 10;
                } else {
                    value = val;
                    return Token::NUMBER;
                }
            }
        } else {
            // octal number
            while (true) {
                if (c >= '0' && c <= '7') {
                    c = std::cin.get();
                    val = val * 8 + c - '0';
                } else {
                    value = val;
                    return Token::NUMBER;
                }
                c = std::cin.peek();
            }
        }
    } else {
        while (c >= '0' && c <= '9') {
            c = std::cin.get();
            val = val * 10 + (int)(c - '0');
            c = std::cin.peek();
        }
        value = val;
        return Token::NUMBER;
    }
}

Token Lexer::parseIDKeyword() {
    std::string s;
    char c;
    while (true) {
        c = std::cin.get();
        s += c;
        c = std::cin.peek();
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_') || (c >= '0' && c <= '9'))) {
            if (s == "while") {
                return Token::WHILE;
            }
            if (s == "break") {
                return Token::BREAK;
            }
            if (s == "continue") {
                return Token::CONTINUE;
            }
            if (s == "return") {
                return Token::RETURN;
            }
            if (s == "int") {
                type = Type::INT;
                return Token::TYPE;
            }
            if (s == "void") {
                type = Type::VOID;
                return Token::TYPE;
            }
            if (s == "if") {
                return Token::IF;
            }
            if (s == "else") {
                return Token::ELSE;
            }
            if (s == "const")  {
                return Token::CONST;
            }
            name = std::move(s);
            return Token::IDENTIFIER;
        }
    }
}

Token Lexer::parseComment() {
    char c;
    c = std::cin.get();
    if (c == '*') {
        // multi-line comment
        while (true) {
            c = std::cin.peek();
            if (c == '*') {
                c = std::cin.get();
                c = std::cin.peek();
                if (c == '/') {
                    c = std::cin.get();
                    break;
                }
            }
            c = std::cin.get();
        }
    } else if (c == '/') {
        // single-line comment
        while ((c = std::cin.peek()) != '\n') {
            c = std::cin.get();
        }
    }
    return Token::COMMENT;
}

Token Lexer::NextToken() {
    char c;
    while (true) {
        c = std::cin.peek();
        if (c >= '0' && c <= '9') {
            return parseInt();
        }
        else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')) {
            return parseIDKeyword();
        }
        else {
            if (c == EOF) {
                return Token::END;
            }
            switch(c) {
                case '+': {
                    c = std::cin.get();
                    op = Operator::ADD;
                    return Token::OPERATOR;
                }
                case '-': {
                    c = std::cin.get();
                    op = Operator::SUB;
                    return Token::OPERATOR;
                }
                case '*': {
                    c = std::cin.get();
                    op = Operator::MUL;
                    return Token::OPERATOR;
                }
                case '/': {
                    c = std::cin.get();
                    c = std::cin.peek();
                    if (c == '/' || c == '*') {
                        return parseComment();
                    }
                    op = Operator::DIV;
                    return Token::OPERATOR;
                }
                case '%': {
                    c = std::cin.get();
                    op = Operator::MOD;
                    return Token::OPERATOR;
                }
                case '>': {
                    c = std::cin.get();
                    c = std::cin.peek();
                    if (c == '=') {
                        c = std::cin.get();
                        op = Operator::GE;
                        return Token::OPERATOR;
                    } else {
                        op = Operator::GT;
                        return Token::OPERATOR;
                    }
                }
                case '<': {
                    c = std::cin.get();
                    c = std::cin.peek();
                    if (c == '=') {
                        c = std::cin.get();
                        op = Operator::LE;
                        return Token::OPERATOR;
                    } else {
                        op = Operator::LT;
                        return Token::OPERATOR;
                    }
                }
                case '=': {
                    c = std::cin.get();
                    c = std::cin.peek();
                    if (c == '=') {
                        c = std::cin.get();
                        op = Operator::EQ;
                        return Token::OPERATOR;
                    } else {
                        return Token::ASSIGN;
                    }
                }
                case '!': {
                    c = std::cin.get();
                    c = std::cin.peek();
                    if (c == '=') {
                        c = std::cin.get();
                        op = Operator::NEQ;
                        return Token::OPERATOR;
                    } else {
                        op = Operator::NOT;
                        return Token::OPERATOR;
                    }
                }
                case '&': {
                    c = std::cin.get();
                    c = std::cin.peek();
                    if (c == '&') {
                        c = std::cin.get();
                        op = Operator::AND;
                        return Token::OPERATOR;
                    }
                    //return Token::ERROR;
                    exit(11);
                }
                case '|': {
                    c = std::cin.get();
                    c = std::cin.peek();
                    if (c == '|') {
                        c = std::cin.get();
                        op = Operator::OR;
                        return Token::OPERATOR;
                    }
                    //return Token::ERROR;
                    exit(12);
                }
                case '(': {
                    c = std::cin.get();
                    return Token::LP;
                }
                case ')': {
                    c = std::cin.get();
                    return Token::RP;
                }
                case '[': {
                    c = std::cin.get();
                    return Token::LSB;
                }
                case ']': {
                    c = std::cin.get();
                    return Token::RSB;
                }
                case '{': {
                    c = std::cin.get();
                    return Token::LB;
                }
                case '}': {
                    c = std::cin.get();
                    return Token::RB;
                }
                case '\'': {
                    c = std::cin.get();
                    return Token::SQM;
                }
                case '"': {
                    c = std::cin.get();
                    return Token::DQM;
                }
                case ';': {
                    c = std::cin.get();
                    return Token::SC;
                }
                case ',': {
                    c = std::cin.get();
                    return Token::CO;
                }
                case '\t':
                case ' ':
                case '\n':
                case '\b': {
                    c = std::cin.get();
                    break;
                }
                default:
                    //return Token::ERROR;
                    exit(13);
            }
        }
    }
}
