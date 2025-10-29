/*
Assignment:
    HW3 - Parser and Code Generator for PL/0

Author(s): Luiz Gustavo Santana Dias Gomes
Language: C (only)

To Compile:
    Scanner:
        gcc -O2 -std=c11 -o lex lex.c
    Parser/Code Generator:
        gcc -O2 -std=c11 -o parsercodegen parsercodegen.c
    To Execute (on Eustis):
        ./lex <input_file.txt>
        ./parsercodegen
    where:
        <input_file.txt> is the path to the PL/0 source program

Notes:
    - lex.c accepts ONE command-line argument (input PL/0 source file)
    - parsercodegen.c accepts NO command-line arguments
    - Input filename is hard-coded in parsercodegen.c
    - Implements recursive-descent parser for PL/0 grammar
    - Generates PM/0 assembly code (see Appendix A for ISA)
    - All development and testing performed on Eustis

    Class: COP3402 - System Software - Fall 2025

Instructor: Dr. Jie Lin
Due Date: Friday, October 31, 2025 at 11:59 PM ET
*/

// ! No AI usage; Sources = GOOGLE
// ! This is a literal translation I did to my Rust code
// ! (commented at the end of this file)

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_PCODE_SIZE 5000

/// @brief Port the MayFail type from Rust to C
typedef struct MayFail {
    int is_error;
    /// @brief Can be anything after all. A pointer that points to anything
    void *value;
    /// @brief If `is_error` is true, this is a pointer to a string
    const char *error_message;
} MayFail;

/// @brief I'd rather use my own struct but this was provided by the assignment
/// and I'm not sure if I can change it.
typedef struct symbol {
    int kind;      // const = 1, var = 2, proc = 3
    char name[12]; // name up to 11 chars
    int val;       // number (ASCII value)
    int level;     // L level
    int addr;      // M address
    int mark;      // to indicate unavailable or deleted
} symbol;

symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];

/// All `self` variables were moved to a static environment. Rust controls
/// the length of a Vec automatically, but C doesn't have it
static int SYMBOL_TABLE_LEN = 0;

/// Error codes defined in the assignment details
char *throw(int code) {
    switch (code) {
    case 1:
        return "Error: program must end with period";
    case 2:
        return "Error: const, var, and read keywords must be followed by identifier";
    case 3:
        return "Error: symbol name has already been declared";
    case 4:
        return "Error: constants must be assigned with =";
    case 5:
        return "Error: constants must be assigned an integer value";
    case 6:
        return "Error: constant and variable declarations must be followed by a semicolon";
    case 7:
        return "Error: undeclared identifier";
    case 8:
        return "Error: only variable values may be altered";
    case 9:
        return "Error: assignment statements must use :=";
    case 10:
        return "Error: begin must be followed by end";
    case 11:
        return "Error: if must be followed by then";
    case 12:
        return "Error: while must be followed by do";
    case 13:
        return "Error: condition must contain comparison operator";
    case 14:
        return "Error: right parenthesis must follow left parenthesis";
    case 15:
        return "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols";
    /// Custom error I added since I noticed that this "if-then" error was missing
    case 16:
        return "Error: if must have a condition and be followed by fi";
    default:
        return "Unknown error kind";
    }
}

/// @brief Symbol kind to be placed in the symbol table
typedef enum SymbolKind {
    SymbolKind_Const = 1,
    SymbolKind_Var,
    SymbolKind_Proc,
} SymbolKind;

/// @brief Operation codes for OPR instruction
typedef enum OprCode {
    OprCode_RTN,
    OprCode_ADD,
    OprCode_SUB,
    OprCode_MUL,
    OprCode_DIV,
    OprCode_EQL,
    OprCode_NEG,
    OprCode_LSS,
    OprCode_LEQ,
    OprCode_GTR,
    OprCode_GEQ,
    OprCode_EVEN,
} OprCode;

/// @brief Any instruction SYS
typedef enum Syscall {
    Syscall_Write = 1,
    Syscall_Read,
    Syscall_Halt,
} Syscall;

/// @brief Tokens defined the same way as in lex.c
typedef enum TokenType {
    TokenType_Skip = 1,
    TokenType_Ident,
    TokenType_Number,
    TokenType_Plus,
    TokenType_Minus,
    TokenType_Mult,
    TokenType_Slash,
    TokenType_Eq,
    TokenType_Neq,
    TokenType_Les,
    TokenType_Leq,
    TokenType_Gtr,
    TokenType_Geq,
    TokenType_Lparent,
    TokenType_Rparent,
    TokenType_Comma,
    TokenType_Semicolon,
    TokenType_Period,
    TokenType_Becomes,
    TokenType_Begin,
    TokenType_End,
    TokenType_If,
    TokenType_Fi,
    TokenType_Then,
    TokenType_While,
    TokenType_Do,
    TokenType_Call,
    TokenType_Const,
    TokenType_Var,
    TokenType_Proc,
    TokenType_Write,
    TokenType_Read,
    TokenType_Else,
    TokenType_Even,
} TokenType;

/// @brief Self explanatory
typedef enum Instruction {
    Instruction_LIT = 1,
    Instruction_OPR,
    Instruction_LOD,
    Instruction_STO,
    Instruction_CAL,
    Instruction_INC,
    Instruction_JMP,
    Instruction_JPC,
    Instruction_SYS,
} Instruction;

/// @brief Same struct as defined in HW1; Holds some OP code, L and M
typedef struct Register {
    Instruction OP;
    int L;
    int M;
} Register;

/// @brief Same definition used in Rust. `meta` is used only for the following types:
/// `TokenType::Number` or `TokenType::Ident` since they have their names after the token number
typedef struct Token {
    TokenType kind;
    char meta[12];
} Token;

/// @brief Emulating Rust behavior, but does not have `pcode` and `symbol_table` since they're
/// static, so they're accessible anywhere.
typedef struct TokenStream {
    int iteration;
    Token *tokens;
    int len_tokens;
} TokenStream;

/// @brief Previously defined in TokenStream struct (Rust)
static Register pcode[MAX_PCODE_SIZE];

static int PCODE_LEN = 0;

/// @brief Same as `self.iteration`
static int TS_ITERATION = 0;

Token ts_token(TokenStream *self);
TokenType ts_kind(TokenStream *self);
TokenType ts_next(TokenStream *self);
MayFail ts_get_symbol_index(TokenStream *self, int *out_index);
MayFail ts_program(TokenStream *self);
MayFail ts_block(TokenStream *self);
MayFail ts_const_declaration(TokenStream *self);
MayFail ts_var_declaration(TokenStream *self, int *count);
MayFail ts_statement(TokenStream *self);
MayFail ts_condition(TokenStream *self);
MayFail ts_expression(TokenStream *self);
MayFail ts_term(TokenStream *self);
MayFail ts_factor(TokenStream *self);
MayFail ts_get_token(TokenStream *self, int iteration);
int token_get_number(Token token);

/// @brief Receives a pointer to anything and returns a MayFail type.
/// @param value Intended to be an `int` or `String`
/// @return `struct MayFail`
MayFail Ok(void *value) {
    MayFail may_fail = {
        .value = value,
        .is_error = 0,
        .error_message = NULL};
    return may_fail;
}

/// @brief Receives any message and returns a MayFail type with Err variant
/// @param error_message Usually from function `throw`, but can be a literal string as well
/// @return `struct MayFail`
MayFail Err(char *error_message) {
    MayFail may_fail;
    may_fail.value = NULL;
    may_fail.is_error = 1;
    may_fail.error_message = error_message;
    return may_fail;
}

/// @brief Print the symbol table. `st` stands for Symbol Table
void st_print() {
    printf(
        "Symbol Table: \n\n%-4s | %-11s | %-5s | %-5s | %-7s | %-4s\n",
        "Kind",
        "Name",
        "Value",
        "Level",
        "Address",
        "Mark");
    for (int i = 0; i < 51; i++) {
        printf("-");
    }
    puts("");
    for (int i = 0; i < SYMBOL_TABLE_LEN; i++) {
        symbol sym = symbol_table[i];

        printf(
            "%4d | %11s | %5d | %5d | %7d | %4d\n",
            sym.kind,
            sym.name,
            sym.val,
            sym.level,
            sym.addr,
            sym.mark);
    }
}

/// @brief Checks if a symbol is in the symbol table
/// @param name Symbol name. May not repeat since variables and constants are in the same scope
/// @return `int` 1 for true, 0 for false
int st_has_symbol(char *name) {
    for (int i = 0; i < SYMBOL_TABLE_LEN; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

/// @brief Returns the symbol at the given index. If index does not exist, throws.
/// @param index Index where the wanted symbol is at
/// @return `struct MayFail` with `void*` being of type `symbol`
MayFail st_get_index(int index) {
    if (index < 0 || index >= SYMBOL_TABLE_LEN) {
        return Err("Index out of bounds");
    }
    return Ok(&(symbol_table[index]));
}

/// @brief Assigns to `out_index` the index of the symbol. Can fail.
/// @param name Name of the ident to be searched for
/// @param code Code to be called in function `throw` if an error occur.
/// @param out_index Variable where the index will be assigned, if it exists
/// @return `struct MayFail` with `void*` null
MayFail st_find_index_or_throw(const char *name, int code, int *out_index) {
    for (int i = 0; i < SYMBOL_TABLE_LEN; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            *out_index = i;
            return Ok(NULL);
        }
    }
    return Err(throw(code));
}

/// @brief Inserts a symbol to the symbol table. Can't fail.
/// @param symbol Symbol to be inserted
void st_push_symbol(symbol symbol) {
    symbol_table[SYMBOL_TABLE_LEN] = symbol;
    SYMBOL_TABLE_LEN++;
}

/// @brief Pushes a constant to the symbol table
/// @param name Constant ident name
/// @param value Its literal value
/// @return `struct MayFail`. Fails if the symbol already exists. `void*` is of type `int`
/// representing the index of the symbol that was just inserted
MayFail st_push_const(char *name, int value) {
    int out_index = 0;
    MayFail fail = st_find_index_or_throw(name, 3, &out_index);
    if (fail.is_error) {
        symbol symbol_value = {
            .kind = SymbolKind_Const,
            .val = value,
            .level = 0,
            .addr = 0,
            .mark = 0,
        };
        strncpy(symbol_value.name, name, sizeof symbol_value.name - 1);
        st_push_symbol(symbol_value);
        return Ok(&SYMBOL_TABLE_LEN);
    } else {
        return Err(throw(3));
    }
}

/// @brief Pushes a variable to the symbol table
/// @param name Variable ident name
/// @param addr Address of the variable
/// @return `struct MayFail`. Fails if the symbol already exists. `void*` is of type `int`
/// representing the index of the symbol that was just inserted
MayFail st_push_var(char *name, int addr) {
    int out_index = 0;
    MayFail fail = st_find_index_or_throw(name, 3, &out_index);
    if (fail.is_error) {
        symbol symbol_value = {
            .kind = SymbolKind_Var,
            .val = 0,
            .level = 0,
            .addr = addr,
            .mark = 0,
        };
        strncpy(symbol_value.name, name, sizeof symbol_value.name - 1);
        st_push_symbol(symbol_value);
        return Ok(&SYMBOL_TABLE_LEN);
    } else {
        return Err(throw(3));
    }
}

/// @brief Prints the assembly code
void pcode_print() {
    printf("Assembly Code:\n\n%4s%8s%8s%8s\n", "Line", "OP", "L", "M");
    for (int i = 0; i < PCODE_LEN; i++) {
        Register reg = pcode[i];

        char opcode[4] = {0};

        switch (reg.OP) {
        case Instruction_LIT: {
            strcpy(opcode, "LIT");
            break;
        }
        case Instruction_OPR: {
            strcpy(opcode, "OPR");
            break;
        }
        case Instruction_LOD: {
            strcpy(opcode, "LOD");
            break;
        }
        case Instruction_STO: {
            strcpy(opcode, "STO");
            break;
        }
        case Instruction_CAL: {
            strcpy(opcode, "CAL");
            break;
        }
        case Instruction_INC: {
            strcpy(opcode, "INC");
            break;
        }
        case Instruction_JMP: {
            strcpy(opcode, "JMP");
            break;
        }
        case Instruction_JPC: {
            strcpy(opcode, "JPC");
            break;
        }
        case Instruction_SYS: {
            strcpy(opcode, "SYS");
            break;
        }
        default: {
            strcpy(opcode, "ERR");
            break;
        }
        }

        printf("%3d %8s%8d%8d\n", i, opcode, reg.L, reg.M);
    }
}

/// @brief Multiplies by 3 the current `pcode` array length to get the `JPC` offset
/// @return The `JPC` offset
int ts_jpc_offset() {
    return PCODE_LEN * 3;
}

/// @brief Adds an instruction to the `pcode` array
/// @param self `TokenStream` adapt
/// @param instr Instruction code
/// @param M M value
void ts_emit(TokenStream *self, Instruction instr, int M) {
    Register reg = {
        .OP = instr,
        .L = 0,
        .M = M,
    };
    pcode[PCODE_LEN] = reg;
    PCODE_LEN += 1;
}

/// @brief Returns the provided `iteration` index in the token list. Fails if does not exist,
/// and panics if the index is out of bounds
/// @param iteration Index of the token to be searched for
/// @return `struct MayFail` with `void*` being of type `Token`
MayFail ts_get_token(TokenStream *self, int iteration) {
    if (iteration >= self->len_tokens) {
        printf("%s", "Failed assert that Token Stream iteration != Token len");
        exit(1);
    }
    return Ok(&(self->tokens[iteration]));
}

/// @brief Returns the current iteration's token. Panics if the current iteration is invalid.
/// @return `struct Token`
Token ts_token(TokenStream *self) {
    MayFail fail = ts_get_token(self, TS_ITERATION);
    if (fail.is_error) {
        printf("Fatal error at ts_token: %s", fail.error_message);
        exit(1);
    }
    return *(Token *)fail.value;
}

/// @brief Calls function `ts_token` and accesses the `kind` field
/// @return `enum TokenType`
TokenType ts_kind(TokenStream *self) {
    return ts_token(self).kind;
}

/// @brief Passes to the next token, and returns the next token type. Panics if index is invalid
/// @return `enum TokenType`
TokenType ts_next(TokenStream *self) {
    TS_ITERATION += 1;
    return ts_kind(self);
}

/// @brief Receives an `out_index` and assigns to it, if successful, the index of the symbol in Symbol Table
/// @param out_index Where the index will be assigned in case of success
/// @return `struct MayFail` with `void*` null
MayFail ts_get_symbol_index(TokenStream *self, int *out_index) {
    Token token = ts_token(self);
    char *meta = token.meta;
    MayFail index_fail = st_find_index_or_throw(meta, 7, out_index);
    if (index_fail.is_error) {
        return index_fail;
    }
    return Ok(NULL);
}

/// @brief Starts to parse the token stream
/// @return `struct MayFail` with `void*` null
MayFail ts_program(TokenStream *self) {
    TokenType last = self->tokens[self->len_tokens - 1].kind;
    if (last != TokenType_Period) {
        throw(1);
    } else {
        MayFail block_fail = ts_block(self);
        if (block_fail.is_error) {
            return block_fail;
        }
        return Ok(NULL);
    }
}

/// @brief Parses `block`
/// @return `struct MayFail` with `void*` null
MayFail ts_block(TokenStream *self) {
    MayFail const_declaration_fail = ts_const_declaration(self);

    if (const_declaration_fail.is_error) {
        return const_declaration_fail;
    }

    int number_of_vars = 0;

    MayFail var_declaration_fail = ts_var_declaration(self, &number_of_vars);

    if (var_declaration_fail.is_error) {
        return var_declaration_fail;
    }
    ts_emit(self, Instruction_INC, number_of_vars + 3);

    MayFail statement_fail = ts_statement(self);

    if (statement_fail.is_error) {
        return statement_fail;
    }

    ts_emit(self, Instruction_SYS, Syscall_Halt);
    return Ok(NULL);
}

MayFail ts_const_declaration(TokenStream *self) {
    if (ts_kind(self) == TokenType_Const) {
        do {
            TokenType token_d0 = ts_next(self);

            if (token_d0 != TokenType_Ident) {
                return Err(throw(2));
            }

            char *ident_name = ts_token(self).meta;

            if (st_has_symbol(ident_name)) {
                return Err(throw(3));
            }

            MayFail pushed_const = st_push_const(ident_name, 0);
            if (pushed_const.is_error) {
                return pushed_const;
            }

            int ident_offset_d0 = *(int *)pushed_const.value;
            TokenType token_d1 = ts_next(self);

            if (token_d1 != TokenType_Eq) {
                return Err(throw(4));
            }

            int token_d2 = ts_next(self);

            if (token_d2 != TokenType_Number) {
                return Err(throw(5));
            }

            int ident_value_d0 = token_get_number(ts_token(self));
            symbol_table[ident_offset_d0].val = ident_value_d0;
            ts_next(self);
        } while (ts_kind(self) == TokenType_Comma);

        if (ts_kind(self) != TokenType_Semicolon) {
            return Err(throw(6));
        }

        ts_next(self);
    }
    return Ok(NULL);
}

/// @brief Parses `var_declaration` and assigns to `count` the number of variables found
/// @param count Pointer to the number of variables to be assigned
/// @return `struct MayFail` with `void*` null
MayFail ts_var_declaration(TokenStream *self, int *count) {
    if (ts_kind(self) == TokenType_Var) {
        do {
            *count += 1;
            TokenType token = ts_next(self);

            if (token != TokenType_Ident) {
                return Err(throw(2));
            }

            char *ident_name = ts_token(self).meta;

            if (st_has_symbol(ident_name)) {
                printf("Symbol %s already exists", ident_name);
                return Err(throw(3));
            }

            MayFail pushed_var = st_push_var(ident_name, *count + 2);
            if (pushed_var.is_error) {
                return pushed_var;
            }

            ts_next(self);
        } while (ts_kind(self) == TokenType_Comma);

        if (ts_kind(self) != TokenType_Semicolon) {
            return Err(throw(6));
        }
    }

    ts_next(self);
    return Ok(NULL);
}

MayFail ts_statement(TokenStream *self) {
    switch (ts_kind(self)) {
    case TokenType_Ident: {
        int symbol_index_s0 = 0;
        MayFail get_symbol_fail = ts_get_symbol_index(self, &symbol_index_s0);
        if (get_symbol_fail.is_error) {
            return get_symbol_fail;
        }

        MayFail get_symbol_fail_2 = st_get_index(symbol_index_s0);
        if (get_symbol_fail_2.is_error) {
            return get_symbol_fail_2;
        }

        symbol retreived_symbol = *(symbol *)get_symbol_fail_2.value;

        if (retreived_symbol.kind != TokenType_Var) {
            return Err(throw(8));
        }

        TokenType token = ts_next(self);

        if (token != TokenType_Becomes) {
            return Err(throw(9));
        }

        ts_next(self);
        MayFail expression_fail = ts_expression(self);
        if (expression_fail.is_error) {
            return expression_fail;
        }

        ts_emit(self, Instruction_STO, retreived_symbol.addr);
        break;
    }
    case TokenType_Begin: {
        do {
            ts_next(self);

            if (ts_kind(self) == TokenType_End) {
                break;
            }

            MayFail statement_fail = ts_statement(self);
            if (statement_fail.is_error) {
                return statement_fail;
            }
        } while (ts_kind(self) == TokenType_Semicolon);

        if (ts_kind(self) != TokenType_End) {
            return Err(throw(10));
        }

        ts_next(self);
        break;
    }
    case TokenType_If: {
        ts_next(self);

        MayFail condition_fail_s0 = ts_condition(self);
        if (condition_fail_s0.is_error) {
            return condition_fail_s0;
        }

        int jpc_index_0 = PCODE_LEN;
        ts_emit(self, Instruction_JPC, 0);

        if (ts_kind(self) != TokenType_Then) {
            return Err(throw(11));
        }

        ts_next(self);
        MayFail statement_fail = ts_statement(self);
        if (statement_fail.is_error) {
            return statement_fail;
        }

        if (ts_kind(self) != TokenType_Fi) {
            return Err(throw(16));
        }

        ts_next(self);
        pcode[jpc_index_0].M = ts_jpc_offset();
        break;
    }
    case TokenType_While: {
        ts_next(self);

        int loop_index = PCODE_LEN;
        MayFail condition_fail_s1 = ts_condition(self);
        if (condition_fail_s1.is_error) {
            return condition_fail_s1;
        }

        if (ts_kind(self) != TokenType_Do) {
            return Err(throw(12));
        }

        ts_next(self);

        int jpc_index_1 = PCODE_LEN;
        ts_emit(self, Instruction_JPC, 0);
        ts_statement(self);
        ts_emit(self, Instruction_JMP, loop_index);

        pcode[jpc_index_1].M = ts_jpc_offset();
        break;
    }
    case TokenType_Read: {
        ts_next(self);

        if (ts_kind(self) != TokenType_Ident) {
            return Err(throw(2));
        }

        int symbol_index_s1 = 0;
        MayFail symbol_index_fail = ts_get_symbol_index(self, &symbol_index_s1);
        if (symbol_index_fail.is_error) {
            return symbol_index_fail;
        }

        MayFail get_Symbol_fail_3 = st_get_index(symbol_index_s1);
        if (get_Symbol_fail_3.is_error) {
            return get_Symbol_fail_3;
        }

        symbol retreived_symbol_2 = *(symbol *)get_Symbol_fail_3.value;

        if (retreived_symbol_2.kind != TokenType_Var) {
            return Err(throw(8));
        }

        ts_next(self);
        ts_emit(self, Instruction_SYS, Syscall_Read);
        ts_emit(self, Instruction_STO, retreived_symbol_2.addr);
        break;
    }
    case TokenType_Write: {
        ts_next(self);
        MayFail expression_fail_2 = ts_expression(self);
        if (expression_fail_2.is_error) {
            return expression_fail_2;
        }

        ts_emit(self, Instruction_SYS, Syscall_Write);
        break;
    }
    default: {
        TokenType this_token = ts_kind(self);
        printf("Found unexpected token: %d", this_token);
        exit(1);
    }
    }

    return Ok(NULL);
}

MayFail ts_condition(TokenStream *self) {
    MayFail expression_fail = ts_expression(self);
    if (expression_fail.is_error) {
        return expression_fail;
    }

    switch (ts_kind(self)) {
    case TokenType_Eq: {
        ts_next(self);
        MayFail expression_fail_c0 = ts_expression(self);
        if (expression_fail_c0.is_error) {
            return expression_fail_c0;
        }
        ts_emit(self, Instruction_OPR, OprCode_EQL);
        break;
    }
    case TokenType_Neq: {
        ts_next(self);
        MayFail expression_fail_c1 = ts_expression(self);
        if (expression_fail_c1.is_error) {
            return expression_fail_c1;
        }
        ts_emit(self, Instruction_OPR, OprCode_NEG);
        break;
    }
    case TokenType_Les: {
        ts_next(self);
        MayFail expression_fail_c2 = ts_expression(self);
        if (expression_fail_c2.is_error) {
            return expression_fail_c2;
        }
        ts_emit(self, Instruction_OPR, OprCode_LSS);
        break;
    }
    case TokenType_Leq: {
        ts_next(self);
        MayFail expression_fail_c3 = ts_expression(self);
        if (expression_fail_c3.is_error) {
            return expression_fail_c3;
        }
        ts_emit(self, Instruction_OPR, OprCode_LEQ);
        break;
    }
    case TokenType_Gtr: {
        ts_next(self);
        MayFail expression_fail_c4 = ts_expression(self);
        if (expression_fail_c4.is_error) {
            return expression_fail_c4;
        }
        ts_emit(self, Instruction_OPR, OprCode_GTR);
        break;
    }
    case TokenType_Geq: {
        ts_next(self);
        MayFail expression_fail_c5 = ts_expression(self);
        if (expression_fail_c5.is_error) {
            return expression_fail_c5;
        }
        ts_emit(self, Instruction_OPR, OprCode_GEQ);
        break;
    }
    default: {
        return Err(throw(15));
    }
    }
    return Ok(NULL);
}

MayFail ts_expression(TokenStream *self) {
    if (ts_kind(self) == TokenType_Minus) {
        ts_next(self);

        MayFail fail_term = ts_term(self);
        if (fail_term.is_error) {
            return fail_term;
        }

        ts_emit(self, Instruction_OPR, OprCode_NEG);

        while (ts_kind(self) == TokenType_Plus || ts_kind(self) == TokenType_Minus) {
            if (ts_kind(self) == TokenType_Plus) {
                ts_next(self);

                MayFail term_fail = ts_term(self);
                if (term_fail.is_error) {
                    return term_fail;
                }

                ts_emit(self, Instruction_OPR, OprCode_ADD);
            } else {
                ts_next(self);

                MayFail term_fail = ts_term(self);
                if (term_fail.is_error) {
                    return term_fail;
                }

                ts_emit(self, Instruction_OPR, OprCode_SUB);
            }
        }
    } else {
        if (ts_kind(self) == TokenType_Plus) {
            ts_next(self);
        }

        MayFail fail_term = ts_term(self);
        if (fail_term.is_error) {
            return fail_term;
        }

        while (ts_kind(self) == TokenType_Plus || ts_kind(self) == TokenType_Minus) {
            if (ts_kind(self) == TokenType_Plus) {
                ts_next(self);

                MayFail term_fail = ts_term(self);
                if (term_fail.is_error) {
                    return term_fail;
                }

                ts_emit(self, Instruction_OPR, OprCode_ADD);
            } else {
                ts_next(self);

                MayFail term_fail = ts_term(self);
                if (term_fail.is_error) {
                    return term_fail;
                }

                ts_emit(self, Instruction_OPR, OprCode_SUB);
            }
        }
    }

    return Ok(NULL);
}

MayFail ts_term(TokenStream *self) {
    MayFail factor_fail = ts_factor(self);
    if (factor_fail.is_error) {
        return factor_fail;
    }

    while (ts_kind(self) == TokenType_Mult || ts_kind(self) == TokenType_Slash) {
        if (ts_token(self).kind == TokenType_Mult) {
            ts_next(self);

            MayFail factor_fail = ts_factor(self);
            if (factor_fail.is_error) {
                return factor_fail;
            }

            ts_emit(self, Instruction_OPR, OprCode_MUL);
        } else {
            ts_next(self);

            MayFail factor_fail = ts_factor(self);
            if (factor_fail.is_error) {
                return factor_fail;
            }

            ts_emit(self, Instruction_OPR, OprCode_DIV);
        }
    }

    return Ok(NULL);
}

MayFail ts_factor(TokenStream *self) {
    switch (ts_kind(self)) {
    case TokenType_Ident: {
        int symbol_index = 0;
        MayFail symbol_index_fail = ts_get_symbol_index(self, &symbol_index);
        if (symbol_index_fail.is_error) {
            return symbol_index_fail;
        }

        symbol_table[symbol_index].mark = 1;
        MayFail symbol_fail = st_get_index(symbol_index);
        if (symbol_fail.is_error) {
            return symbol_fail;
        }

        symbol this_symbol = *(symbol *)symbol_fail.value;

        if (this_symbol.kind == SymbolKind_Const) {
            ts_emit(self, Instruction_LIT, this_symbol.val);
        } else {
            ts_emit(self, Instruction_LOD, this_symbol.addr);
        }
        ts_next(self);
        break;
    }
    case TokenType_Number: {
        int value = token_get_number(ts_token(self));
        ts_emit(self, Instruction_LIT, value);
        ts_next(self);
        break;
    }
    case TokenType_Lparent: {
        ts_next(self);
        ts_expression(self);
        if (ts_kind(self) != TokenType_Rparent) {
            return Err(throw(14));
        }
        ts_next(self);
        break;
    }
    default: {
        return Err(throw(15));
    }
    }

    return Ok(NULL);
}

/// @brief Returns -1 if token's meta is not a valid number, and the number otherwise
/// @return Parsing of token's meta to integer. Same as `token.meta.parse::<i32>().unwrap_or(-1)`
int token_get_number(Token token) {
    if (token.meta == NULL) {
        return -1;
    } else {
        return atoi(token.meta);
    }
}

/// @brief Checks if a string contains only whitespace kind. Builtin function in Rust.
/// @param s The string (intended to be a line of the file)
/// @return `int` 1 for true, 0 for false
int is_blank_or_whitespace_only(const char *s) {
    for (; *s; ++s) {
        if (!isspace((unsigned char)*s)) {
            return 0;
        }
    }
    return 1;
}

/// @brief Strips newlines from the end of a string. Like `.trim()`
/// @param s The string
void rstrip_newline(char *s) {
    int n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

/// @brief Converts a line to a Token struct.
Token token_from_str(const char *line) {
    Token token = {.kind = 0, .meta = ""};
    int kind;
    char meta[12];
    int n = sscanf(line, " %d %11s", &kind, meta);
    if (n >= 1)
        token.kind = kind;
    if (n >= 2) {
        strcpy(token.meta, meta);
    }
    return token;
}

int main() {
    FILE *file_ptr = fopen("tokens.txt", "r");

    Token *tokens = NULL;
    /// Capacity and length of token list initialized to 0
    int len = 0, cap = 0;

    /// @brief A line should never have more than ~20 characters but 1024 is just in case
    char line[1024];
    while (fgets(line, sizeof line, file_ptr)) {
        /// same as .trim()
        rstrip_newline(line);
        /// same as .filter(char::is_whitespace)
        if (line[0] == '\0' || is_blank_or_whitespace_only(line)) {
            continue;
        }

        /// same as .map(Token::from)
        Token this_token = token_from_str(line);

        /// Realloc if the buffer has run out of space
        if (len == cap) {
            /// If capacity is zero, multiply by 2 make no sense
            int new_cap = cap ? cap * 2 : 64;
            Token *tmp = (Token *)realloc(tokens, new_cap * sizeof *tokens);
            if (!tmp) {
                puts("Realloc error");
                free(tokens);
                fclose(file_ptr);
                return 1;
            }
            tokens = tmp;
            cap = new_cap;
        }
        tokens[len++] = this_token;
    }

    fclose(file_ptr);

    /// Same as creating the TokenStream type: TokenStream::from(tokens)
    TokenStream token_stream = {.tokens = tokens, .len_tokens = len};

    /// Emit the hardcoded instruction of JMP 0 3
    ts_emit(&token_stream, Instruction_JMP, 3);

    /// Start program execution
    MayFail program = ts_program(&token_stream);
    if (program.is_error) {
        /// Errors will be propagated as value in a chain, same as in Rust's `?` operator.
        /// If any error in any function that returns MayFail is truthy, this error message will
        /// fall in this line, and be printed to the console, and exit afterwards.
        printf("%s", program.error_message);
        return 1;
    }

    /// Prints the assembly instructions generated
    pcode_print();
    puts("");

    /// Prints the symbol table
    st_print();

    free(tokens);
    return 0;
}
