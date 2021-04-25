#include <iostream>
#include <string>
#include "lexer.hpp"

Token Lexer::parseInt() {
    char c;
    long long int val = 0;
    std::cin >> c;
    if (c == '0') {
        c = std::cin.peek();
        if (c == 'x' || c == 'X') {
            // hexadecimal const
            std::cin >> c;
            while (true) {
                c = std::cin.peek();
                if (c >= '0' && c <= '9') {
                    std::cin >> c;
                    val = val * 16 + c - '0';
                } else if (c >= 'A' && c <= 'F') {
                    std::cin >> c;
                    val = val * 16 + c - 'A' + 10;
                } else if (c >= 'a' && c <= 'f') {
                    std::cin >> c;
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
                    std::cin >> c;
                    val = val * 8 + c - '0';
                } else {
                    value = val;
                     return Token::NUMBER;
                }
                c = std::cin.peek();
            }
        }
    } else {
        while (true) {
            if (c >= '0' && c <= '9') {
                std::cin >> c;
                val = val * 10 + c - '0';
            } else {
                value = val;
                return Token::NUMBER;
            }
            c = std::cin.peek();
        }
    }
}

Token Lexer::parseIDKeyword() {
    std::string s;
    char c;
    while (true) {
        std::cin >> c;
        s += c;
        c = std::cin.peek();
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'))) {
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
                type = Type::INT;
                return Token::TYPE;
            }
            if (s == "if") {
                return Token::IF;
            }
            if (s == "else") {
                return Token::ELSE;
            }
            name = std::move(s);
            return Token::IDENTIFIER;
        }
    }
}

Token Lexer::parseComment() {
    char c;
    std::cin >> c;
    if (c == '*') {
        // multi-line comment
        while (true) {
            do {
                std::cin >> c;
            } while (c != '*');
            std::cin >> c;
            if ((c = std::cin.peek()) == '/') {
                std::cin >> c;
                break;
            }
        }
    } else if (c == '/') {
        // single-line comment
        while ((c = std::cin.peek()) != '\n') {
            std::cin >> c;
        }
    }
    return Token::COMMENT;
}

Token Lexer::NextToken() {
    char c = std::cin.peek();
    while (true) {
        if (c >= '0' && c <= '9') {
            return parseInt();
        }
        else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')) {
            return parseIDKeyword();
        }
        else {
            switch(c) {
                case '+': {
                    std::cin >> c;
                    op = Operator::ADD;
                    return Token::OPERATOR;
                }
                case '-': {
                    std::cin >> c;
                    op = Operator::SUB;
                    return Token::OPERATOR;
                }
                case '*': {
                    std::cin >> c;
                    op = Operator::MUL;
                    return Token::OPERATOR;
                }
                case '/': {
                    std::cin >> c;
                    c = std::cin.peek();
                    if (c == '/' || c == '*') {
                        return parseComment();
                    }
                    op = Operator::DIV;
                    return Token::OPERATOR;
                }
                case '%': {
                    std::cin >> c;
                    op = Operator::MOD;
                    return Token::OPERATOR;
                }
                case '>': {
                    std::cin >> c;
                    c = std::cin.peek();
                    if (c == '=') {
                        std::cin >> c;
                        op = Operator::GE;
                        return Token::OPERATOR;
                    } else {
                        op = Operator::GT;
                        return Token::OPERATOR;
                    }
                }
                case '<': {
                    std::cin >> c;
                    c = std::cin.peek();
                    if (c == '=') {
                        std::cin >> c;
                        op = Operator::LE;
                        return Token::OPERATOR;
                    } else {
                        op = Operator::LT;
                        return Token::OPERATOR;
                    }
                }
                case '=': {
                    std::cin >> c;
                    c = std::cin.peek();
                    if (c == '=') {
                        std::cin >> c;
                        op = Operator::EQ;
                        return Token::OPERATOR;
                    } else {
                        return Token::ASSIGN;
                    }
                }
                case '!': {
                    std::cin >> c;
                    c = std::cin.peek();
                    if (c == '=') {
                        std::cin >> c;
                        op = Operator::NEQ;
                        return Token::OPERATOR;
                    } else {
                        op = Operator::NOT;
                        return Token::OPERATOR;
                    }
                }
                case '&': {
                    std::cin >> c;
                    c = std::cin.peek();
                    if (c == '&') {
                        std::cin >> c;
                        op = Operator::AND;
                        return Token::OPERATOR;
                    }
                    return Token::ERROR;
                }
                case '|': {
                    std::cin >> c;
                    c = std::cin.peek();
                    if (c == '|') {
                        std::cin >> c;
                        op = Operator::OR;
                        return Token::OPERATOR;
                    }
                    return Token::ERROR;
                }
                case '(': {
                    std::cin >> c;
                    return Token::LP;
                }
                case ')': {
                    std::cin >> c;
                    return Token::RP;
                }
                case '[': {
                    std::cin >> c;
                    return Token::LSB;
                }
                case ']': {
                    std::cin >> c;
                    return Token::RSB;
                }
                case '{': {
                    std::cin >> c;
                    return Token::LB;
                }
                case '}': {
                    std::cin >> c;
                    return Token::RB;
                }
                case '\'': {
                    std::cin >> c;
                    return Token::SQM;
                }
                case '"': {
                    std::cin >> c;
                    return Token::DQM;
                }
                case ';': {
                    std::cin >> c;
                    return Token::SC;
                }
                case ',': {
                    std::cin >> c;
                    return Token::CO;
                }
                case '\t':
                case ' ':
                case '\n':
                    break;
                default:
                    return Token::ERROR;
            }
        }
    }
}
