%{
    #include <iostream>
    using namespace std;
%}

%%

Number: INT_CONST;

FuncRParams: Exp {"," Exp}
        ;
LVal: IDENT { "[" Exp "]" };
PrimaryExp: "(" Exp ")"
        | LVal
        | Number
        ;
UnaryOp: "+"
        | "-"
        | "!"
        ;
UnaryExp: PrimaryExp 
        | IDENT "(" [FuncRParams] ")" 
        | UnaryOp UnaryExp
        ;
RMulExp: ("*" | "/" | "%%") UnaryExp RMulExp 
        | %empty
        ;
MulExp: UnaryExp RMulExp
        ;
RAddExp: "+" MulExp RAddExp 
        | %empty
        ;
AddExp: MulExp RAddExp
        ;
RRelExp: ("<" | ">" | "<=" | ">=") AddExp RRelExp
        | %empty
        ;
RelExp: AddExp RRelExp;
REqExp: ("==" | "!=") RelExp REqExp
        | %empty
        ;
EqExp: RelExp REqExp
        ;
*Exp: AddExp;
*ConstExp: AddExp;
RLAndExp: "&&" EqExp RLAndExp
        | %empty
        ;
LAndExp: EqExp RLAndExp
        ;
RLOrExp: "||" LAndExp RLOrExp
        | %empty
        ;
LOrExp: LAndExp RLOrExp
        ;
Cond: LOrExp
        ;
Stmt: LVal "=" Exp ";"
        | [Exp] ";"
        | Block
        | "if" "(" Cond ")" Stmt ["else" Stmt]
        | "while" "(" Cond ")" Stmt
        | "break" ";"
        | "continue" ";"
        | "return" [Exp] ";"
        ;
BlockItem: Decl
        | Stmt
        ;
Block: "{" BlockItem "}"
        ;
FuncFParam: BType IDENT [ "[" "]" {"[" ConstExp "]"}];
FuncFParams: FuncFParam {"," FuncFParam}
        ;
FuncType: "void" | "int"
        ;
FuncDef: FuncType IDENT "(" [FuncFParams] ")" Block;
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
BType: "int"
        ;
ConstDecl: "const" BType ConstDef {"," ConstDef} ";"
        ;
Decl: ConstDecl
        |
        VarDecl
        ;
CompUnit: (Decl | FuncDef) [CompUnit]
        ;

%%

int main() {
    yyparse();
}