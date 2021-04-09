%{
    #include <iostream>
    using namespace std;
%}

%%

"+" { return PLUS; }
"-" { return MINUS; }
"*" { return MUL; }
"/" { return DIV; }
"%" { return MOD; } 
"==" { return EQ; }
"!=" { return NE; }
"if" { return IF; }
"while" { return WHILE; }
"break" { return BREAK; }
"continue" { return CONTINUE; }
"return" { return RETURN; }
"else" { return ELSE; }
"||" { return OR; }
"&&" { return AND; }
"!" { return NOT; }
"<" { return LT; }
">" { return GT; }
"<=" { return LE; }
">=" { return GE; }
"void" { return VOID; }
"int" { return INT; }
"const" { return CONST; }


%%