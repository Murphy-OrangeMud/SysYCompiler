#ifndef TOKEN_INCLUDED
#define TOKEN_INCLUDED

#include <iostream>

enum Token {
    COMMENT,
    IDENTIFIER,
    ERROR,
    OPERATOR,
    NUMBER,
    TYPE,
    CONST,
    END,
    BREAK,
    CONTINUE,
    RETURN,
    IF,
    ELSE,
    WHILE,
    ASSIGN,
    LP,
    RP,
    LSB,
    RSB,
    LB,
    RB,
    DQM,
    SQM,
    SC,
    CO
};

enum Operator {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    EQ,
    NEQ,
    LT,
    GT,
    LE,
    GE,
    OR,
    AND,
    NOT,
    NONE
};

enum Type {
    VOID,
    INT
};

enum VarType {
    ARRAY,
    VAR
};

#endif
