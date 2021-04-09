%{
    #include <iostream>
    using namespace std;
%}

%token PLUS MINUS MUL DIV MOD EQ NE NOT LT GT LE GE
%token IF WHILE BREAK CONTINUE RETURN ELSE OR AND
%token VOID INT CONST 
%%

Number: INT_CONST;

FuncRParams: Exp {"," Exp}
        |
        ;
LVal: IDENT { "[" Exp "]" };
PrimaryExp: "(" Exp ")"
        | LVal
        | Number
        ;
UnaryOp: PLUS
        | MINUS
        | NOT
        ;
UnaryExp: PrimaryExp 
        | IDENT "(" FuncRParams ")" 
        | UnaryOp UnaryExp
        ;
MulOp: MUL
        |
        DIV
        |
        MOD
        ;
RMulExp: MulOp UnaryExp RMulExp 
        |
        ;
MulExp: UnaryExp RMulExp
        ;
RAddExp: "+" MulExp RAddExp 
        |
        ;
AddExp: MulExp RAddExp
        ;
RelOp: LT
        |
        LE
        |
        GT
        |
        GE
        ;
RRelExp: RelOp AddExp RRelExp
        |
        ;
RelExp: AddExp RRelExp;
EqOp: EQ
        |
        NEQ
        ;
REqExp: EqOp RelExp REqExp
        |
        ;
EqExp: RelExp REqExp
        ;
Exp: AddExp;
ConstExp: AddExp;
RLAndExp: AND EqExp RLAndExp
        |
        ;
LAndExp: EqExp RLAndExp
        ;
RLOrExp: OR LAndExp RLOrExp
        |
        ;
LOrExp: LAndExp RLOrExp
        ;
Cond: LOrExp
        ;
IfStmt: IF "(" Cond ")" Stmt
        ;
IfElseStmt: IfStmt ELSE Stmt
        ;
Stmt: LVal "=" Exp ";"
        | Exp ";"
        | ";"
        | Block
        | IfStmt
        | IfElseStmt
        | WHILE "(" Cond ")" Stmt
        | BREAK ";"
        | CONTINUE ";"
        | RETURN ";"
        | RETURN Exp ";"
        ;
BlockItem: Decl
        | Stmt
        ;
Block: "{" BlockItem "}"
        ;
FuncFParam: BType IDENT 
        | BType IDENT "[" "]" {"[" ConstExp "]"};
FuncFParams: FuncFParam {"," FuncFParam}
        |
        ;
FuncType: VOID
        | INT
        ;
FuncDef: FuncType IDENT "(" FuncFParams ")" Block;
InitVal: Exp 
        | "{" [InitVal {"," InitVal}] "}"
        ;
LVarDef: IDENT {"[" ConstExp "]"}
        ;
VarDef: LVarDef | LVarDef "=" InitVal
        ;
VarDecl: BType VarDef {"," VarDef} ";"
        ;
ConstInitVal: ConstExp
        | "{" [ConstInitVal {"," ConstInitVal}] "}"
        ;
ConstDef: IDENT {"[" ConstExp "]"} "=" ConstInitVal
        ;
BType: INT
        ;
ConstDecl: CONST BType ConstDef {"," ConstDef} ";"
        ;
Decl: ConstDecl
        |
        VarDecl
        ;
CompUnit: Decl CompUnit
        | FuncDef CompUnit
        | Decl
        | FuncDef
        ;

%%

int main() {
    yyparse();
}