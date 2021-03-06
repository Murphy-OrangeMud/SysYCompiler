#include "eeyore_lexer.hpp"
#include <define/irtok.hpp>
#include <iostream>

namespace EeyoreToTigger {
    Token Lexer::ParseNum() {
        char c = cinstream.peek();
        int val = 0;
        if (c == '0') {
            c = cinstream.get();
            c = cinstream.peek();
            if (c == 'x' || c == 'X') {
                // hexadecimal const
                c = cinstream.get();
                while (true) {
                    c = cinstream.peek();
                    if (c >= '0' && c <= '9') {
                        c = cinstream.get();
                        val = val * 16 + c - '0';
                    } else if (c >= 'A' && c <= 'F') {
                        c = cinstream.get();
                        val = val * 16 + c - 'A' + 10;
                    } else if (c >= 'a' && c <= 'f') {
                        c = cinstream.get();
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
                        c = cinstream.get();
                        val = val * 8 + c - '0';
                    } else {
                        value = val;
                        return Token::NUMBER;
                    }
                    c = cinstream.peek();
                }
            }
        } else {
            while (c >= '0' && c <= '9') {
                c = cinstream.get();
                val = val * 10 + (int)(c - '0');
                c = cinstream.peek();
            }
            value = val;
            return Token::NUMBER;
        }
    }

    Token Lexer::ParseSymbol() {
        std::string s;
        char c;
        while (true) {
            c = cinstream.get();
            s += c;
            c = cinstream.peek();
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_') || (c >= '0' && c <= '9'))) {
                if (s == "var") {
                    return Token::VARDECL;
                }
                if (s == "end") {
                    return Token::FUNCEND;
                }
                if (s == "if") {
                    return Token::IF;
                }
                if (s == "goto") {
                    return Token::GOTO;
                }
                if (s == "return") {
                    return Token::RETURN;
                }
                if (s == "call") {
                    return Token::CALL;
                }
                if (s == "param") {
                    return Token::PARAM;
                }
                name = s;
                return Token::SYMBOL;
            }
        }
    }

    Token Lexer::ParseComment() {
        char c;
        c = cinstream.get();
        while ((c = cinstream.peek()) != '\n') {
            c = cinstream.get();
        }
        return Token::COMMENT;
    }

    Token Lexer::NextToken() {
        char c;
        while (true) {
            c = cinstream.peek();
            if (c >= '0' && c <= '9') {
                return ParseNum();
            }
            else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')) {
                return ParseSymbol();
            }
            else if (c == EOF) {
                return Token::END;
            }
            else if (c <= 32) {
                if (c == '\n') {
                    lineno++;
                }
                c = cinstream.get();
                continue;
            }
            else {
                switch (c) {
                    case '+': {
                        c = cinstream.get();
                        op = Operator::ADD;
                        return Token::OP;
                    }
                    case '-': {
                        c = cinstream.get();
                        op = Operator::SUB;
                        return Token::OP;
                    }
                    case '*': {
                        c = cinstream.get();
                        op = Operator::MUL;
                        return Token::OP;
                    }
                    case '/': {
                        c = cinstream.get();
                        c = cinstream.peek();
                        if (c == '/') {
                            return ParseComment();
                        }
                        op = Operator::DIV;
                        return Token::OP;
                    }
                    case '%': {
                        c = cinstream.get();
                        op = Operator::MOD;
                        return Token::OP;
                    }
                    case '>': {
                        c = cinstream.get();
                        c = cinstream.peek();
                        if (c == '=') {
                            c = cinstream.get();
                            op = Operator::GE;
                            return Token::LOGICOP;
                        } else {
                            op = Operator::GT;
                            return Token::LOGICOP;
                        }
                    }
                    case '<': {
                        c = cinstream.get();
                        c = cinstream.peek();
                        if (c == '=') {
                            c = cinstream.get();
                            op = Operator::LE;
                            return Token::LOGICOP;
                        } else {
                            op = Operator::LT;
                            return Token::LOGICOP;
                        }
                    }
                    case '=': {
                        c = cinstream.get();
                        c = cinstream.peek();
                        if (c == '=') {
                            c = cinstream.get();
                            op = Operator::EQ;
                            return Token::LOGICOP;
                        } else {
                            return Token::ASSIGN;
                        }
                    }
                    case '!': {
                        c = cinstream.get();
                        c = cinstream.peek();
                        if (c == '=') {
                            c = cinstream.get();
                            op = Operator::NEQ;
                            return Token::LOGICOP;
                        } else {
                            op = Operator::NOT;
                            return Token::OP;
                        }
                    }
                    case '[': {
                        c = cinstream.get();
                        return Token::LSB;
                    }
                    case ']': {
                        c = cinstream.get();
                        return Token::RSB;
                    }
                    case ':': {
                        c = cinstream.get();
                        return Token::COLON;
                    }
                    default:
                        exit(23);
                }
            }
        }

    }
}
