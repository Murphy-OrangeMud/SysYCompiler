#ifndef IRTOK_INCLUDED
#define IRTOK_INCLUDED

namespace EeyoreToTigger {

    enum Token {
        VARDECL,
        NUMBER,
        SYMBOL,
        LSB,
        RSB,
        ASSIGN,
        FUNCEND,
        END,
        GOTO,
        IF,
        PARAM,
        CALL,
        RETURN,
        OP,
        COLON,
        LOGICOP,
        COMMENT
    };

    enum Operator {
        GT, GE, LT, LE, EQ, NEQ,
        ADD, SUB, DIV, MOD, MUL, NOT
    };

    enum VarType {
        ARRAY, VAR
    };
}

#endif