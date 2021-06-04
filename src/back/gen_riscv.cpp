#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <set>
#include "gen_riscv.hpp"

namespace TiggerToRiscV {
    void RiscVGenerator::Generate() {
        std::string code;
        std::string currentFunc;
        while (true) {
            char stmt[1000];
            memset(stmt, 0, sizeof(stmt));
            std::cin.getline(stmt, 1005);
            std::string str_stmt;
            int init_idx = 0;
            for (; stmt[init_idx] <= 32; init_idx++);
            int len = strlen(stmt);
            for (; init_idx < len; init_idx++) {
                str_stmt += stmt[init_idx];
            }
            if (str_stmt.empty()) {
                break;
            }
            std::istringstream stream_stmt(str_stmt);
            std::string first;
            stream_stmt >> first;
            if (first[0] == 'f' && first[1] == '_') {
                // func header
                std::string func_name = first.substr(2, first.length() - 2);
                int int1, int2;
                int idx = 0;
                for (; !(stmt[idx] >= '0' && stmt[idx] <= '9'); idx++);
                for (; stmt[idx] >= '0' && stmt[idx] <= '9'; idx++) {
                    int1 = int1 * 10 + stmt[idx] - '0';
                }
                for (; !(stmt[idx] >= '0' && stmt[idx] <= '9'); idx++);
                for (; stmt[idx] >= '0' && stmt[idx] <= '9'; idx++) {
                    int2 = int2 * 10 + stmt[idx] - '0';
                }
                funcStack[func_name] = int2;
                code += ".text\n.align 2\n.global " + func_name + "\n.type " + func_name + ", @function\n" + func_name +
                        ":\naddi sp, sp, -" + std::to_string(16 * (int2 / 4 + 1)) + "\nsw ra, " +
                        std::to_string(16 * (int2 / 4 + 1)) + "-4(sp)\n";
                currentFunc = func_name;
            } else if (first == "end") {
                // func end
                std::string func_name;
                stream_stmt >> func_name;
                func_name = func_name.substr(2, func_name.length() - 2);
                code += ".size " + func_name + ", .-" + func_name + "\n";
                currentFunc = "";
            } else if (first[0] == 'v') {
                int isArray = first.find("malloc");
                if (isArray == std::string::npos) {
                    std::string var_name, init_val;
                    int idx = first.find("=");
                    if (idx != std::string::npos) {
                        var_name = first.substr(0, idx);
                        if (idx == first.length() - 1) {
                            stream_stmt >> init_val;
                        } else {
                            init_val = first.substr(idx + 1, first.length() - idx - 1);
                        }
                    } else {
                        var_name = first;
                        std::string eq;
                        stream_stmt >> eq;
                        stream_stmt >> init_val;
                    }
                    code += ".global " + var_name +
                            "\n.section .sdata\n.align 2\n.type " +
                            var_name + ", @object\n.size " +
                            var_name + ", 4\n" + var_name +
                            ":\n.word " + init_val + "\n";
                } else {
                    std::string var_name;
                    int idx = first.find("=");
                    std::string malloc_;
                    std::string size;
                    if (idx != std::string::npos) {
                        var_name = first.substr(0, idx);
                        if (idx == first.length() - 1) {
                            stream_stmt >> malloc_;
                        }
                        stream_stmt >> size;
                    }
                    code += ".comm " + var_name + ", " + size + ", 4\n";
                }
            } else if (first == "goto") {
                std::string label;
                stream_stmt >> label;
                code += "j ." + label + "\n";
            } else if (first[0] == 'l' && first[1] >= '0' && first[1] <= '9') {
                code += "." + first + "\n";
            } else if (first == "call") {
                std::string func_name;
                stream_stmt >> func_name;
                func_name = func_name.substr(2, func_name.length() - 2);
                code += "call " + func_name + "\n";
            } else if (first == "return") {
                int stk = funcStack[currentFunc];
                code += "lw ra, " + std::to_string(stk) + "-4(sp)\naddi sp, sp, " + std::to_string(stk) + "\nret\n";
            } else if (first == "store") {
                std::string reg;
                int int10;
                stream_stmt >> reg >> int10;
                if (int10 >= -512 && int10 <= 511) {
                    code += "sw " + reg + ", " + std::to_string(int10) + "*4(sp)\n";
                } else {
                    code += "li " + std::to_string(int10) + " s0\nsw " + reg + ", s0*4(sp)\n";
                }
            } else if (first == "load") {
                std::string var, reg;
                stream_stmt >> var >> reg;
                if (var[0] == 'v') {
                    code += "lui " + reg + ", %hi(" + var + ")\nlw " + reg + ", %lo(" + var + ")(" + reg + ")\n";
                } else {
                    int int10 = std::stoi(var);
                    if (int10 >= -512 && int10 <= 511) {
                        code += "lw " + reg + ", " + var + "*4(sp)\n";
                    } else {
                        code += "li " + var + " s0\nlw " + reg + ", s0*4(sp)\n";
                    }
                }
            } else if (first == "loadaddr") {
                std::string var, reg;
                stream_stmt >> var >> reg;
                if (var[0] == 'v') {
                    code += "la " + reg + ", " + var + "\n";
                } else {
                    int int10 = std::stoi(var);
                    if (int10 >= -512 && int10 <= 511) {
                        code += "addi " + reg + ", sp, " + var + "*4\n";
                    } else {
                        code += "li " + var + " s0\naddi " + reg + ", sp, s0*4\n";
                    }
                }
            } else if (first == "if") {
                // 假定有空格
                std::string reg1, reg2, cmp, gt, label;
                stream_stmt >> reg1 >> cmp >> reg2 >> gt >> label;
                if (cmp == "<") {
                    code += "blt " + reg1 + ", " + reg2 + ", ." + label + "\n";
                } else if (cmp == ">") {
                    code += "bgt " + reg1 + ", " + reg2 + ", ." + label + "\n";
                } else if (cmp == "<=") {
                    code += "ble " + reg1 + ", " + reg2 + ", ." + label + "\n";
                } else if (cmp == ">=") {
                    code += "bge " + reg1 + ", " + reg2 + ", ." + label + "\n";
                } else if (cmp == "!=") {
                    code += "bne " + reg1 + ", " + reg2 + ", ." + label + "\n";
                } else if (cmp == "==") {
                    code += "beq " + reg1 + ", " + reg2 + ", ." + label + "\n";
                }
            } else {
                std::string lhs;
                stream_stmt >> lhs;
                std::string token;
                std::string op, var1, var2;
                std::set<std::string> ops{"+", "-", "*", "/", "%", "<", ">", "<=", ">=", "&&", "||", "!=", "=="};
                while (stream_stmt >> token) {
                    if (token == "=") continue;
                    if (ops.find(token) != ops.end()) {
                        op = token;
                        continue;
                    }
                    if (var1.empty()) var1 = token;
                    if (var2.empty()) var2 = token;
                }
                if (op.empty()) {
                    if (var1[0] == '!') {
                        // reg1 = !reg2
                        var1 = var1.substr(1, var1.length() - 1);
                        code += "seqz " + lhs + ", " + var1 + "\n";
                    } else if (var1[0] == '-') {
                        if (var1[1] >= '0' && var1[1] <= '9') {
                            // reg = int
                            code += "li " + lhs + ", " + var1 + "\n";
                        } else {
                            // reg1 = -reg2
                            code += "neg " + lhs + ", " + var1 + "\n";
                        }
                    } else {
                        int llidx = lhs.find("[");
                        int rlidx = var1.find("[");
                        if (llidx == std::string::npos && rlidx == std::string::npos) {
                            code += "mv " + lhs + ", " + var1 + "\n";
                        } else if (llidx == std::string::npos) {
                            int rridx = var1.find("]");
                            int offset = std::stoi(var1.substr(rlidx + 1, rridx - rlidx - 1));
                            if (offset >= -2048 && offset <= 2047) {
                                code += "lw " + lhs + ", " + std::to_string(offset) + "(" +  var1 + ")\n";
                            } else {
                                code += "li s0, " + std::to_string(offset) + "\nlw " + lhs + ", s0(" + var1 + ")\n";
                            }
                        } else if (rlidx == std::string::npos) {
                            int lridx = lhs.find("]");
                            int offset = std::stoi(lhs.substr(llidx + 1, lridx - llidx - 1));
                            if (offset >= -2048 && offset <= 2047) {
                                code += "sw " + var1 + ", " + std::to_string(offset) + "(" + lhs + ")\n";
                            } else {
                                code += "li s0, " + std::to_string(offset) + "\nsw " + var1 + ", s0(" + lhs + ")\n";
                            }
                        }
                    }
                } else {
                    if (op == "+") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            int int12 = std::stoi(var2);
                            if (int12 >= -2048 && int12 <= 2047) {
                                code += "addi " + lhs + ", " + var1 + ", " + var2 + "\n";
                            } else {
                                code += "li s0, " + var2 + "\nadd " + lhs + ", " + var1 + ", s0\n";
                            }
                        } else {
                            code += "add " + lhs + ", " + var1 + ", " + var2 + "\n";
                        }
                    } else if (op == "-") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\nsub " + lhs + ", " + var1 + ", s0\n";
                        } else {
                            code += "sub " + lhs + ", " + var1 + ", " + var2 + "\n";
                        }
                    } else if (op == "*") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\nmul " + lhs + ", " + var1 + ", s0\n";
                        } else {
                            code += "mul " + lhs + ", " + var1 + ", " + var2 + "\n";
                        }
                    } else if (op == "/") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\ndiv " + lhs + ", " + var1 + ", s0\n";
                        } else {
                            code += "div " + lhs + ", " + var1 + ", " + var2 + "\n";
                        }
                    } else if (op == "%") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\nrem " + lhs + ", " + var1 + ", s0\n";
                        } else {
                            code += "rem " + lhs + ", " + var1 + ", " + var2 + "\n";
                        }
                    } else if (op == "<") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            int int12 = std::stoi(var2);
                            if (int12 >= -2048 && int12 <= 2047) {
                                code += "slti " + lhs + ", " + var1 + ", " + var2 + "\n";
                            } else {
                                code += "li s0, " + var2 + "\nslt " + lhs + ", " + var1 + ", s0\n";
                            }
                        } else {
                            code += "slt " + lhs + ", " + var1 + ", " + var2 + "\n";
                        }
                    } else if (op == ">") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\nsgt " + lhs + ", " + var1 + ", s0\n";
                        } else {
                            code += "sgt " + lhs + ", " + var1 + ", " + var2 + "\n";
                        }
                    } else if (op == "<=") {
                        if (!(var2[0] >= 'a'  && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\nsgt " +  lhs + ", "  + var1 + ", s0\nseqz " + lhs + ", " + lhs + "\n";
                        } else {
                            code += "sgt " +  lhs + ", "  + var1 + ", " + var2 + "\nseqz " + lhs + ", " + lhs + "\n";
                        }
                    } else if (op == ">=") {
                        if (!(var2[0] >= 'a'  && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\nslt " +  lhs + ", "  + var1 + ", s0\nseqz " + lhs + ", " + lhs + "\n";
                        } else {
                            code += "slt " +  lhs + ", "  + var1 + ", " + var2 + "\nseqz " + lhs + ", " + lhs + "\n";
                        }
                    } else if (op == "&&") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            int tmp = std::stoi(var2);
                            if (tmp == 0) {
                                code += "li t0, " + var2 + "\nsnez " + lhs + ", t0\nsnez s0, t0\nand " + lhs + ", " + lhs + ", s0\n";
                            }
                        } else {
                            code += "snez " + lhs + ", " + var1 + "\nsnez s0, " + var2 + "\nand " + lhs + ", " + lhs + ", s0\n";
                        }
                    } else if (op == "||") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "or " + lhs + ", " + var1 + ", t0\nsnez " + lhs + ", " + lhs + "\n";
                        } else {
                            code += "or " + lhs + ", " + var1 + ", " + var2 + "\nsnez " + lhs + ", " + lhs + "\n";
                        }
                    } else if (op == "!=") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\nxor " + lhs + ", " + var1 + ", " + var2 + "\nsnez " + lhs + ", " + lhs + "\n";
                        } else {
                            code += "xor " + lhs + ", " + var1 + ", " + var2 + "\nsnez " + lhs + ", " + lhs + "\n";
                        }
                    } else if (op == "==") {
                        if (!(var2[0] >= 'a' && var2[0] <= 'z')) {
                            code += "li s0, " + var2 + "\nxor " + lhs + ", " + var1 + ", " + var2 + "\nseqz " + lhs + ", " + lhs + "\n";
                        } else {
                            code += "xor " + lhs + ", " + var1 + ", " + var2 + "\nseqz " + lhs + ", " + lhs + "\n";
                        }
                    }
                }
            }
        }
    }
}