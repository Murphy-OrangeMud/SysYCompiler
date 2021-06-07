# <center> 编译实习报告 </center>
<center> 程芷怡 元培学院 1800017781 </center>

## 编译器概述

### 基本功能

编译器`compiler`，通过形如：
```compiler -S testcase.sy -o testcase.S```的命令行进行调用，将SysY语言程序testcase.sy编译为risc-v语言汇编程序testcase.S。

其中，SysY语言的文法定义如下：

```
CompUnit      ::= [CompUnit] (Decl | FuncDef);

Decl          ::= ConstDecl | VarDecl;
ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
BType         ::= "int";
ConstDef      ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal;
ConstInitVal  ::= ConstExp | "{" [ConstInitVal {"," ConstInitVal}] "}";
VarDecl       ::= BType VarDef {"," VarDef} ";";
VarDef        ::= IDENT {"[" ConstExp "]"}
                | IDENT {"[" ConstExp "]"} "=" InitVal;
InitVal       ::= Exp | "{" [InitVal {"," InitVal}] "}";

FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;
FuncType      ::= "void" | "int";
FuncFParams   ::= FuncFParam {"," FuncFParam};
FuncFParam    ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];

Block         ::= "{" {BlockItem} "}";
BlockItem     ::= Decl | Stmt;
Stmt          ::= LVal "=" Exp ";"
                | [Exp] ";"
                | Block
                | "if" "(" Cond ")" Stmt ["else" Stmt]
                | "while" "(" Cond ")" Stmt
                | "break" ";"
                | "continue" ";"
                | "return" [Exp] ";";

Exp           ::= AddExp;
Cond          ::= LOrExp;
LVal          ::= IDENT {"[" Exp "]"};
PrimaryExp    ::= "(" Exp ")" | LVal | Number;
Number        ::= INT_CONST;
UnaryExp      ::= PrimaryExp | IDENT "(" [FuncRParams] ")" | UnaryOp UnaryExp;
UnaryOp       ::= "+" | "-" | "!";
FuncRParams   ::= Exp {"," Exp};
MulExp        ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
AddExp        ::= MulExp | AddExp ("+" | "-") MulExp;
RelExp        ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
EqExp         ::= RelExp | EqExp ("==" | "!=") RelExp;
LAndExp       ::= EqExp | LAndExp "&&" EqExp;
LOrExp        ::= LAndExp | LOrExp "||" LAndExp;
ConstExp      ::= AddExp;
```

#### 基本思路

本编译器分为了三个部分进行实现，思路为SysY语言——>Eeyore语言——>Tigger语言——>RISC-V汇编语言。其中，SysY语言——>Eeyore语言阶段手动构造词法分析器和自顶向下的LL语法分析器构建抽象语法树，并对抽象语法树进行加工（常量折叠等操作）进行类型检查并通过语法制导翻译生成Eeyore语言。在Eeyore语言——>Tigger语言阶段手动构造词法分析器和自顶向下的LL语法分析器构建Eeyore语言的抽象语法树，并通过语法制导翻译生成Tigger代码。Tigger语言——>RISC-V汇编语言部分直接进行字符串处理然后查表转换的模式。

#### 目录结构

```
|-- .src
    |-- main.cpp
    |-- oj.hpp
    |-- utils
        |-- logger.hpp
        |-- logger.cpp
        |-- utils.hpp
    |-- define
        |-- ast.hpp
        |-- ir.hpp
        |-- irtok.hpp
        |-- token.hpp
        |-- irgen.cpp
        |-- gen.cpp
        |-- eval.cpp
    |-- front
        |-- lexer.hpp
        |-- lexer.cpp
        |-- parser.hpp
        |-- parser.cpp
    |-- mid
        |-- typeck.hpp
        |-- typeck.cpp
        |-- genIR.hpp
        |-- genIR.cpp
    |-- back
        |-- eeyore_lexer.hpp
        |-- eeyore_lexer.cpp
        |-- eeyore_parser.hpp
        |-- eeyore_parser.cpp
        |-- gen_riscv.hpp
        |-- gen_riscv.cpp
        |-- gen_tigger.hpp
        |-- gen_tigger.cpp
|-- CMakeLists.txt
|-- .gitignore
```

CMakeLists.txt文件是cmake构建工具的脚本，供编译使用。utils目录中包括utils.cpp，里面定义了一些数据结构。logger是日志类，供debug使用。define目录中定义了token，SysY语言的抽象语法树和中间语言表示的抽象语法树的结构，以及visitor模式的实现——包含了类型检查、代码生成和中间代码生成。front目录中包括了SysY语言的词法分析器和语法分析器。mid目录中包括了类型检查和中间代码生成部分。back目录中包括了eeyore语言到riscv语言的所有过程。

#### 具体代码实现和设计特点

为了方便提交代码到oj上和命名，代码中使用了三个命名空间，SysYToEeyore，EeyoreToTigger，TiggerToRiscV。并使用了oj.hpp文件进行宏定义管理。

同时，使用了`std::unique_ptr`进行语法树的管理，在编译期避免了指针使用的混乱。抽象语法树节点定义如下：

```
    using ASTPtr = std::unique_ptr<BaseAST>;
    using ASTPtrList = std::vector<ASTPtr>;

    class BaseAST {
    public:
        virtual ~BaseAST() = default;

        virtual ASTPtr Eval(TypeCheck &checker) = 0;

        virtual std::string GenerateIR(IRGenerator &gen, std::string &code) = 0;
    };
```

Eval函数和GenerateIR函数的一般形式如下：

```
    ASTPtr IfElseAST::Eval(TypeCheck &checker) {
        return checker.EvalIfElse(*this);
    }
```

```
    std::string IfElseAST::GenerateIR(IRGenerator &gen, std::string &code) {
        gen.GenIfElse(*this, code);
        return {};
    }
```

使用了多态和visitor模式。visitor模式是一种将算法与对象结构分离的软件设计模式。

这个模式的基本想法如下：首先我们拥有一个由许多对象构成的对象结构，这些对象的类都拥有一个accept方法用来接受visitor对象；visitor是一个接口，它拥有一个visit方法，这个方法对访问到的对象结构中不同类型的元素作出不同的反应；在对象结构的一次访问过程中，我们遍历整个对象结构，对每一个元素都实施accept方法，在每一个元素的accept方法中回调visitor的visit方法，从而使visitor得以处理对象结构的每一个元素。我们可以针对对象结构设计不同的实在的visitor类来完成不同的操作。

visitor模式使得我们可以在传统的单分派语言（如Smalltalk、Java和C++）中模拟双分派技术。对于支持多分派的语言（如CLOS），visitor模式已经内置于语言特性之中了，从而不再重要。此处使用visitor模式的visit就是`IRGenerator`和`Typechecker`中的一系列接口函数，而accept就是`GenerateIR`和`Eval`函数。这样使用将语法树定义和pass过程解耦，简单清晰。

**main函数（pass管理）**

```
int main(int argc, char *argv[]) {
#ifdef SYSY2EEYORE
    ...
#else
#ifdef EEYORE2TIGGER
    ...
#else
#ifdef TIGGER2RISCV
    ...
#else
#ifdef SYSY2RISCV
    if (argc < 5) {
        std::cerr << "Please enter filename\n";
        exit(-1);
    }
    if (!freopen(argv[2], "r", stdin)) { std::cerr << "open input file failed" << std::endl; exit(7); }
    if (!freopen(argv[4], "w", stdout)) { std::cerr << "open output file failed" << std::endl; exit(8); }

    SysYToEeyore::Parser parser = SysYToEeyore::Parser();
    SysYToEeyore::ASTPtr root = parser.ParseCompUnit();
    if (!root) {
        std::cerr << "Parse error, syntax error\n";
        exit(1);
    }
    SysYToEeyore::TypeCheck checker = SysYToEeyore::TypeCheck();
    SysYToEeyore::ASTPtr nRoot = root->Eval(checker);
    if (!nRoot) {
        std::cerr << "Type check error\n";
        exit(2);
    }
    std::map<std::string, Function> FuncTable = checker.FuncTable;
    std::map<int, std::map<std::string, Var>> BlockVars = checker.BlockVars;
    SysYToEeyore::IRGenerator generator = SysYToEeyore::IRGenerator(std::move(FuncTable), std::move(BlockVars));
    std::string out;
    nRoot->GenerateIR(generator, out);

    std::istringstream stream_stmt(out);
    EeyoreToTigger::Parser parser2 = EeyoreToTigger::Parser(stream_stmt);
    EeyoreToTigger::IRPtr root2 = parser2.ParseProgram();
    EeyoreToTigger::TiggerGenerator generator2 = EeyoreToTigger::TiggerGenerator();
    std::string out2;
    root2->Generate(generator2, out2);

    std::istringstream stream_stmt2(out2);
    TiggerToRiscV::RiscVGenerator generator3 = TiggerToRiscV::RiscVGenerator(stream_stmt2);
    std::string out3;
    generator3.Generate(out3);
    std::cout << out3 << std::endl;
#endif
#endif
#endif
#endif
    return 0;
}
```

使用了命名空间管理，并用`stringstream`进行流管理以避免修改过多代码。三个阶段的实现相对比较清晰。

**词法和语法分析**

这个部分由于gcc和助教first-step代码的启发（主要是学不会用flex和bison（划掉））决定使用手写递归式下降语法分析器。

token流定义：

```
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
}
```

词法分析器的主要接口：

```
    class Lexer {
    public:
        Lexer() = default;
        ~Lexer() = default;

        Token NextToken();
        Operator getOp() { return op; }
        long long getVal() { return value; }
        std::string getName() { return name; }
        Type getType() { return type; }

    private:
        Token parseInt();
        Token parseIDKeyword();
        Token parseComment();

        long long value{};
        std::string name;
        Operator op;
        Type type;
    };
```

`NextToken()`是暴露给语法分析器的接口，然后通过get系列函数拿到token值的具体内容。

语法分析器的主要接口：

```
    class Parser {
    private:
        Lexer lexer;
        Logger logger;
        Token current;
    public:
        Parser() {
            lexer = Lexer();
            logger = Logger();
        }

        void NextToken();

        ASTPtr ParseBinary(const std::function<ASTPtr()> &parser, std::initializer_list <Operator> ops);

        ASTPtr ParseRelExp();

        ASTPtr ParseEqExp();

        ASTPtr ParseAddExp();

        ASTPtr ParseMulExp();

        ASTPtr ParseLAndExp();

        ASTPtr ParseLOrExp();

        ASTPtr ParseUnaryExp();

        ASTPtr ParseIfElseStmt();

        ASTPtr ParseWhileStmt();

        ASTPtr ParseStmt();

        ASTPtr ParseBlock();

        ASTPtr ParseInitVal();

        ASTPtr ParseFuncDef();

        ASTPtr ParseVarDecl();

        ASTPtr ParseVarDef(bool isConst);

        ASTPtr ParseCompUnit();
    };
```

读入token流，在进行语法分析的同时粗略构造抽象语法树。

**类型检查**

```
    class TypeCheck {
    private:
        Logger logger;
        int currentBlock;
        std::string currentFunc;
    public:

        std::vector<int> parentBlock;

        std::map<std::string, Function> FuncTable;
        std::map<int, std::map<std::string, Var>> BlockVars;

        TypeCheck(const std::string &i = "") {
            logger = Logger();
            currentFunc = "";
            currentBlock = 0;

            FuncTable["getint"] = Function("getint", Type::INT, std::vector<Var>{});
            FuncTable["getch"] = Function("getch", Type::INT, std::vector<Var>{});
            FuncTable["getarray"] = Function("getarray", Type::INT,
                                             std::vector<Var>{Var("a", VarType::ARRAY, false, std::vector<int>{0})});
            FuncTable["putint"] = Function("putint", Type::VOID, std::vector<Var>{Var("a", VarType::VAR, false)});
            FuncTable["putch"] = Function("putch", Type::VOID, std::vector<Var>{Var("a", VarType::VAR, false)});
            FuncTable["putarray"] = Function("putarray", Type::VOID, std::vector<Var>{Var("a", VarType::VAR, false),
                                                                                      Var("b", VarType::ARRAY, false,
                                                                                          std::vector<int>{0})});
        }

        ~TypeCheck() = default;

        bool FillInValue(int *memory, InitValAST *init, std::vector<int> &dim, size_t i);

        std::unique_ptr<VarDeclAST> EvalVarDecl(VarDeclAST &varDecl);

        std::unique_ptr<ProcessedIdAST> EvalId(IdAST &id);

        std::unique_ptr<VarDefAST> EvalVarDef(VarDefAST &varDef);

        std::unique_ptr<FuncCallAST> EvalFuncCall(FuncCallAST &func);

        std::unique_ptr<BlockAST> EvalBlock(BlockAST &block);

        std::unique_ptr<IfElseAST> EvalIfElse(IfElseAST &stmt);

        std::unique_ptr<WhileAST> EvalWhile(WhileAST &stmt);

        std::unique_ptr<ControlAST> EvalControl(ControlAST &stmt);

        std::unique_ptr<AssignAST> EvalAssign(AssignAST &assign);

        ASTPtr EvalLVal(LValAST &lval);

        ASTPtr EvalAddExp(BinaryExpAST &exp);

        ASTPtr EvalMulExp(BinaryExpAST &exp);

        ASTPtr EvalUnaryExp(UnaryExpAST &exp);

        std::unique_ptr<FuncDefAST> EvalFuncDef(FuncDefAST &funcDef);

        ASTPtr EvalRelExp(BinaryExpAST &exp);

        ASTPtr EvalLAndExp(BinaryExpAST &exp);

        ASTPtr EvalLOrExp(BinaryExpAST &exp);

        std::unique_ptr<CompUnitAST> EvalCompUnit(CompUnitAST &unit);

        ASTPtr EvalEqExp(BinaryExpAST &exp);

        std::unique_ptr<StmtAST> EvalStmt(StmtAST &stmt);

        std::unique_ptr<InitValAST> EvalInitVal(InitValAST &init);

        std::unique_ptr<NumberAST> EvalNumber(NumberAST &num);
    };
```

在类型检查中构造了函数表，并以块（作用域）为单位构造符号表。

其中，`struct Function`的定义如下：

```
struct Function {
    std::string funcName;
    SysYToEeyore::Type funcType;
    std::vector<Var> argTable;

    Function() {}
    Function(std::string _n, SysYToEeyore::Type _t, std::vector<Var> _a) : funcName(std::move(_n)), funcType(_t),
                                                             argTable(std::move(_a)) {}
};
```

记录了函数名、函数返回值类型和参数表信息。

`struct Var`的定义如下：

```
struct Var {
    std::string name;
    SysYToEeyore::VarType type;
    bool isConst;
    std::vector<int> dims;
    int val;

    Var() {}
    Var(std::string _n, SysYToEeyore::VarType _t, bool _c, std::vector<int> _d = std::vector<int>{}, int _v = 0) : name(
            std::move(_n)), type(_t), isConst(_c), dims(std::move(_d)), val(_v) {}
};
```

记录了变量名，是否为常量，类型（数组还是变量），形状和值等类型。

**Eeyore代码生成**

Eeyore语言的文法定义如下：

```
Program         ::= {Declaration | Initialization | FunctionDef};

Declaration     ::= "var" [NUM] SYMBOL;
Initialization  ::= SYMBOL "=" NUM
                  | SYMBOL "[" NUM "]" "=" NUM;
FunctionDef     ::= FunctionHeader Statements FunctionEnd;
FunctionHeader  ::= FUNCTION "[" NUM "]";
Statements      ::= {Statement};
FunctionEnd     ::= "end" FUNCTION;

Statement       ::= Expression | Declaration;
Expression      ::= SYMBOL "=" RightValue BinOp RightValue
                  | SYMBOL "=" OP RightValue
                  | SYMBOL "=" RightValue
                  | SYMBOL "[" RightValue "]" "=" RightValue
                  | SYMBOL "=" SYMBOL "[" RightValue "]"
                  | "if" RightValue LOGICOP RightValue "goto" LABEL
                  | "goto" LABEL
                  | LABEL ":"
                  | "param" RightValue
                  | "call" FUNCTION
                  | SYMBOL "=" "call" FUNCTION
                  | "return" RightValue
                  | "return";
RightValue      ::= SYMBOL | NUM;
BinOp           ::= OP | LOGICOP;
```

这是一种三地址线性代码。将while、if等高级分支和循环结构变成了goto结构。同时摒弃了复杂的表达式，每个表达式之中最多只有三个量。函数调用变成了call语句。

```
    class IRGenerator {
    private:
        int t_num;
        int T_num;
        int l_num;
        int cur_break_l;
        int cur_continue_l;
        Logger logger;
        int currentDepth;
        int currentBlock;
        std::string currentFunc;

        std::map<int, std::map<std::string, GenVar>> BlockSymbolTable;
        std::vector<int> parentBlock;
        std::map<std::string, Function> FuncTable;
        std::vector<GenVar> ReverseSymbolTable;

    public:
        IRGenerator(std::map<std::string, Function> __FuncTable,
                    const std::map<int, std::map<std::string, Var>> &BlockVars, std::string i = "") : FuncTable(
                std::move(__FuncTable)) {
            for (auto &item1 : BlockVars) {
                for (auto &item2 : item1.second) {
                    if (item2.second.isConst && item2.second.type == VarType::VAR) continue;
                    BlockSymbolTable[item1.first][item2.first] = GenVar(item2.second.name, item2.second.type,
                                                                        item2.second.dims);
                }
            }

            t_num = 0;
            T_num = 0;
            l_num = 0;
            cur_break_l = -1;
            cur_continue_l = -1;
            currentDepth = 0;
            currentBlock = 0;
            currentFunc = "";

            logger = Logger();
        }

        void
        GenerateValue(const std::string &varName, int &idx, int indx, InitValAST *init, std::vector<int> dim, int i,
                      std::string &code);

        void GenVarDecl(VarDeclAST &varDecl, std::string &code);

        std::string GenId(ProcessedIdAST &id, std::string &code);

        std::string GenNumber(NumberAST &num, std::string &code);

        std::string GenVarDef(VarDefAST &varDef, std::string &code);

        std::string GenAssign(AssignAST &assign, std::string &code);

        std::string GenBinaryExp(BinaryExpAST &exp, std::string &code);

        std::string GenInitVal(InitValAST &init, std::string &code);

        static std::string op2char(Operator op);

        void GenBlock(BlockAST &block, std::string &code);

        std::string GenFuncCall(FuncCallAST &func, std::string &code);

        std::string GenLVal(LValAST &lval, std::string &code);

        std::string GenUnaryExp(UnaryExpAST &exp, std::string &code);

        std::string GenLAndExp(BinaryExpAST &exp, std::string &code);

        std::string GenLOrExp(BinaryExpAST &exp, std::string &code);

        void GenFuncDef(FuncDefAST &funcDef, std::string &code);

        void GenCompUnit(CompUnitAST &unit, std::string &code);

        void GenIfElse(IfElseAST &stmt, std::string &code);

        void GenWhile(WhileAST &stmt, std::string &code);

        void GenControl(ControlAST &stmt, std::string &code);

        void GenStmt(StmtAST &stmt, std::string &code);
    };
```

`t_num`和`T_num`分别表示下一个临时变量和原生变量的标号。`l_num`表示下一个label的标号。`cur_break_l`和`cur_continue_l`表示循环中break和continue代码需要跳到的对应label编号。`currentDepth`用于表示该block的嵌套深度，主要用于在代码前加上相应的tab，追求美观性和便于debug。`currentBlock`表明现在所处的块（作用域）编号，用于管理父子作用域。`currentFunc`表示当前所处的函数名。

`BlockSymbolTable`用于表示每个block、变量名和变量信息的映射关系。其中`struct GenVar`的定义如下，包含了变量名，变量类型（数组还是整型）和id（Eeyore语言中对应的变量名）信息。

```
struct GenVar {
    std::string name;
    SysYToEeyore::VarType argType;
    std::vector<int> dims;
    std::string id;

    GenVar() {}
    GenVar(std::string _n, SysYToEeyore::VarType _t, std::vector<int> _d = std::vector<int>{}, std::string _id = "")
            : name(std::move(_n)), argType(_t), dims(std::move(_d)), id(std::move(_id)) {}
};
```

在生成代码和之前的类型检查时，对于每个变量名递归查找`BlockSymbolTable`中的对应变量，找到最内部作用域的对应变量。如果找不到则报错。代码如下：

```
        std::map<std::string, GenVar>::iterator iter;
        int tmpCurrentBlock = currentBlock;
        while (tmpCurrentBlock != -1) {
            iter = BlockSymbolTable[tmpCurrentBlock].find(lval.getName());
            if (iter != BlockSymbolTable[tmpCurrentBlock].end()) {
                break;
            }
            tmpCurrentBlock = parentBlock[tmpCurrentBlock];
        }
        if (lval.getType() == VarType::VAR) {
            return iter->second.id;
        }
```

`parentBlock`记录父作用域用于递归查找。

`FuncTable`直接从类型检查复制过来，是函数的信息。

`ReverseSymbolTable`用于通过当前符号的id（原生变量）查找变量信息用于集中变量声明。用法示例：

```
        for (int i = T_tmp; i < T_num; i++) {
            if (ReverseSymbolTable[i].argType == VarType::VAR) {
                for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
                code += ("var T" + std::to_string(i) + "\n");
            } else {
                std::vector<int> dim = ReverseSymbolTable[i].dims;
                int size = 4;
                for (int d: dim) {
                    size *= d;
                }
                for (int j = 0; j < currentDepth + 1; j++) { code += "\t"; }
                code += ("var " + std::to_string(size) + " " + ReverseSymbolTable[i].id + "\n");
            }
        }
```

**Eeyore代码生成语法树**

同样经过了词法分析和语法分析部分，但和SysY语言相似，不再重复说明。

**活性分析、寄存器分配和代码优化**

这一部分由于时间和精力所限，在线性扫描算法碰到了编译上的困难时选择了略过，所有的变量均压栈处理。但是看了一下生成的Tigger代码，发现可优化的空间还有很多。如机械性加载变量会导致变量的重复加载和使用等。后续若时间精力充足，加上程序正确性有保证的前提下准备用图着色算法尝试。

**Tigger代码生成**

Tigger语言的文法定义如下：

```
Program         ::= {GlobalVarDecl | FunctionDef};

GlobalVarDecl   ::= VARIABLE "=" NUM
                  | VARIABLE "=" "malloc" NUM;
FunctionDef     ::= FunctionHeader Expressions FunctionEnd;
FunctionHeader  ::= FUNCTION "[" NUM "]" "[" NUM "]";
Expressions     ::= {Expression};
FunctionEnd     ::= "end" FUNCTION;

Expression      ::= Reg "=" Reg BinOp Reg
                  | Reg "=" Reg BinOp NUM
                  | Reg "=" OP Reg
                  | Reg "=" Reg
                  | Reg "=" NUM
                  | Reg "[" NUM "]" "=" Reg
                  | Reg "=" Reg "[" NUM "]"
                  | "if" Reg LOGICOP Reg "goto" LABEL
                  | "goto" LABEL
                  | LABEL ":"
                  | "call" FUNCTION
                  | "return"
                  | "store" Reg NUM
                  | "load" NUM Reg
                  | "load" VARIABLE Reg
                  | "loadaddr" NUM Reg
                  | "loadaddr" VARIABLE Reg;
BinOp           ::= OP | LOGICOP;
Reg             ::= "x0"
                  | "s0" | "s1" | "s2" | "s3" | "s4" | "s5" | "s6" | "s7" | "s8" | "s9" | "s10" | "s11"
                  | "t0" | "t1" | "t2" | "t3" | "t4" | "t5" | "t6"
                  | "a0" | "a1" | "a2" | "a3" | "a4" | "a5" | "a6" | "a7";
```

比起Eeyore语言，Tigger语言最大的特性就是加入了对运行时环境的考虑，包括栈的操作、寄存器的操作和栈、寄存器之间的交互等，用于为生成risc-v代码造势。以及将reg和num进一步区分开。

**Risc-V代码生成**

这一部分只需要线性机械翻译即可，但需要考虑到的东西是函数定义和返回时的栈操作，以及长立即数的处理。另外测试用例中不讲武德的`t0 [s0] `（即'['和符号分开、'!'和符号分开等）操作也给我带来了一些麻烦。

#### 测试用例

测试用例主要来自于open-test-cases的repo。

#### 碰到的主要困难和bug

遗憾的是截止到写报告的时候，编译器中的bug仍没有完全解决，从SysY到RiscV的得分仅仅为95/100。

目前碰到的主要bug有：

1.  词法分析中，需要忽略的空白字符不仅包括`\n`、`\t`、`\b`，可能还有`\r`和其他等。后来使用的策略包括将ascii字符小于等于32的空白或控制字符全部忽略。

2.  unary op中+号是不需要写的。

3.  数组初始化部分的定义不够详细和清晰导致之前钻牛角尖。

4.  Eeyore语言对于表达式的规定比SysY严格很多，因此很多cornercase没有考虑到（比如param不能压栈数组元素，除非先赋值给另一个变量）

5.  Tigger语言中`load`和`loadaddr`之前并没有掌握其精髓导致生成的代码加载数组的时候出现了内存错误等问题，以及`$REG[$REG] = $REG`语句没有搞清楚作用范围，导致访问栈上数组元素的时候一筹莫展。后来找到助教答疑才解决。

#### 主要收获

对于我来说，这次编译实习的最大收获应该是在写大型程序的软件工程方面和C++语言上的。之前笔者在课内外写过的规模最大的程序也没有超过1500行，而这个编译器估计代码在5000行左右。如何组织这么大的一个程序是笔者面对的非常重要的问题。在这个过程中，笔者探索了namespace的使用方法，并参考了助教在编译器大赛中的MimiC代码和供我们参考的first-step代码，学习了很多C++ 11以上的语言特性，比如各种智能指针、`dynamic_cast`关键字，了解了`std::function`、`std::initializer_list`等模板，并初次使用了宏展开。

如以下从first-step中借鉴的抽象化代码（而不是ParseAddExp、ParseMulExp、……）令笔者印象深刻，巧妙处理了多个操作数的表达式等。在MimiC中还包括了对于"="的处理，使用了栈而不是递归。可惜时间有限，没有精力进一步深入研究其中C++的一些特性，只做了大概了解。

```
    ASTPtr Parser::ParseBinary(const std::function<ASTPtr()> &parser, std::initializer_list<Operator> ops) {
        logger.SetFunc("ParseBinary");
        auto lhs = parser();
        logger.UnSetFunc("ParseBinary");
        if (!lhs) {
            logger.Error("Parse lhs failed");
            exit(101);
        }
        while (current == Token::OPERATOR && std::find(ops.begin(), ops.end(), lexer.getOp()) != ops.end()) {
            Operator tmp = lexer.getOp();
            NextToken(); // Consume OPERATOR
            auto rhs = parser();
            logger.UnSetFunc("ParseBinary");
            if (!rhs) {
                logger.Error("Parse rhs failed");
                exit(102);
            }
            lhs = std::make_unique<BinaryExpAST>(tmp, std::move(lhs), std::move(rhs));
        }
        return lhs;
    }
```

另外比较个人化的一点是，笔者以前对写lexer、parser心有神往（认为很酷）但心存畏惧。这次手写parser解决了心中的畏惧，并破除了程序语言和编译在很大程度上的神秘感，增强了对编译的理解。

