#include <iostream>
#include <string>
#include "lexer.hpp"

Token Lexer::parseInt() {
    std::string s;
    std::cin >> s;
    long long int val = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        // hexadecimal const
        for (int i = 2; i < s.length(); i++) {
            if (s[i] >= '0' && s[i] <= '9') {
                val = val * 16 + s[i] - '0';
            }
            else if (s[i] >= 'A' && s[i] <= 'F') {
                val = val * 16 + s[i] - 'A' + 10;
            }
            else if (s[i] >= 'a' && s[i] <= 'f') {
                val = val * 16 + s[i] - 'a' + 10;
            }
            else {
                return Token::ERROR;
            }
        }
    }
    else if (s[0] == '0') {
        // octal const
        for (int i = 1; i < s.length(); i++) {
            if (s[i] >= '0' && s[i] <= '7') {
                val = val * 8 + s[i] - '0';
            }
            else {
                return Token::ERROR;
            }
        }
    }
    else {
        for (int i = 0; i < s.length(); i++) {
            if (s[i] >= '0' && s[i] <= '9') {
                val = val * 10 + s[i] - '0';
            }
            else {
                return Token::ERROR;
            }
        }
    }
    value = val;
    return Token::NUMBER;
}

Token Lexer::parseIDKeyword() {
    std::string s;
    std::cin >> s;
    // check if keyword
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
    // check if legal
    for (int i = 0; i < s.length(); i++) {
        if (s[i] != '_' && !((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'))) {
            return Token::ERROR;
        }
    }
    name = std::move(s);
    return Token::IDENTIFIER;
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
    if (c >= '0' && c <= '9') {
        return parseInt();
    }
    else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')) {
        return parseIDKeyword();
    }
    else {
        switch(c) {
            case '+':{
                std::cin >> c;
                op = Operator::ADD;
                return Token::OPERATOR;
            }
            case '-':{
                std::cin >> c;
                op = Operator::SUB;
                return Token::OPERATOR;
            }
            case '*':{
                std::cin >> c;
                op = Operator::MUL;
                return Token::OPERATOR;
            }
            case '/':{
                std::cin >> c;
                c = std::cin.peek();
                if (c == '/' || c == '*') {
                    return parseComment();
                }
                op = Operator::DIV;
                return Token::OPERATOR;
            }
            case '%':{
                std::cin >> c;
                op = Operator::MOD;
                return Token::OPERATOR;
            }
            case '>':{
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
            case '<':{
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
            case '=':{
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
            case '!':{
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
            case '&':{
                std::cin >> c;
                c = std::cin.peek();
                if (c == '&') {
                    std::cin >> c;
                    op = Operator::AND;
                    return Token::OPERATOR;
                }
                return Token::ERROR;
            }
            case '|':{
                std::cin >> c;
                c = std::cin.peek();
                if (c == '|') {
                    std::cin >> c;
                    op = Operator::OR;
                    return Token::OPERATOR;
                }
                return Token::ERROR;
            }
            case '(':{
                std::cin >> c;
                return Token::LP;
            }
            case ')':{
                std::cin >> c;
                return Token::RP;
            }
            case '[':{
                std::cin >> c;
                return Token::LSB;
            }
            case ']':{
                std::cin >> c;
                return Token::RSB;
            }
            case '{':{
                std::cin >> c;
                return Token::LB;
            }
            case '}':{
                std::cin >> c;
                return Token::RB;
            }
            case '\'':{
                std::cin >> c;
                return Token::SQM;
            }
            case '"':{
                std::cin >> c;
                return Token::DQM;
            }
            case ';':{
                std::cin >> c;
                return Token::SC;
            }
            case ',':{
                std::cin >> c;
                return Token::CO;
            }
        }
        return Token::ERROR;
    }
}
