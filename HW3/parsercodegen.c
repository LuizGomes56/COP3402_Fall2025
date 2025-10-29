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
// ! This is a literal translation from my Rust source code.
// ! (commented at the end of this file)

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_PCODE_SIZE 5000

/// Creating a new helper macro to propagate errors through functions that may fail
/// Same as Rust's `?` operator. Propagates the error if MayFail failed.
#define try(expr)              \
    do {                       \
        MayFail __mf = (expr); \
        if (__mf.is_error)     \
            return __mf;       \
    } while (0)

/// macro_rules! try_cast { ($try:expr) => { $try? } }
/// Propagates the error. If it is successful, gets the value by casting the `value` field.
/// that the `MayFail` type has returned. First argument must be the type to cast to.
#define try_cast(T, expr) \
    ({ MayFail __mf = (expr);       \
    if (__mf.is_error) return __mf; \
    *(T *)__mf.value; })

/// @brief Port the MayFail type from Rust to C. Field `value` must be
/// casted to something else to be used. Example:
/// ```c
/// int out = 0;
/// MayFail success = ts_var_declaration(&token_stream, &out);
/// int returned_int = *(int *)success.value;
/// ```
/// I'm using the macro `try` and `try_cast` instead of manually doing it all the time.
/// Since C has no operators to propagate errors, this has to be manually done and pollutes the code.
/// However this is still the best way to guarantee that the program won't crash and avoid doing
/// weird comparisons such as `if (x == -1) { ... }` that says nothing about the error (error was not a value),
/// and has much less flexibility in its return type (must be fixed type instead of any such as in this implementation).
/// This is the same as `Result<T, E> where T: *const (), E: Box<dyn std::error::Error>`. T is a void pointer and
/// behave the same way in here -> can be unsafely casted to anything. Crashes if the cast was done to the wrong type.
typedef struct MayFail {
    int is_error;
    /// @brief Can be anything after all. A pointer that points to anything
    void *value;
    /// @brief If `is_error` is true, this is a pointer to a string
    const char *error_message;
} MayFail;

/// @brief I'd rather use my own struct but this was provided by the assignment
/// and I'm not sure if I can change it.
typedef struct Symbol {
    int kind;      // const = 1, var = 2, proc = 3
    char name[12]; // name up to 11 chars
    int val;       // number (ASCII value)
    int level;     // L level
    int addr;      // M address
    int mark;      // to indicate unavailable or deleted
} Symbol;

static Symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];

/// All `self` variables were moved to a static environment. Rust controls
/// the length of a Vec automatically, but C doesn't have it
static int SYMBOL_TABLE_LEN = 0;

/// Error codes defined in the assignment details
char *throw(int code) {
    switch (code) {
    /// Added error if a skip symbol was found
    case 0:
        return "Error: Scanning error detected by lexer (skipsym present)";
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
    /// Internal error
    case 17:
        return "Error: Assertion (iteration >= self->len_tokens) failed [IOB]";
    case 18:
        return "Error: Symbol table indexation failed: Index out of bounds";
    case 19:
        return "Error: Symbol is no longer usable (mark = 1)";
    default:
        return "Error: Unknown error code passed to 'throw' function";
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
    MayFail may_fail = {
        .value = NULL,
        .is_error = 1,
        .error_message = error_message,
    };
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
        Symbol sym = symbol_table[i];

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
        return Err(throw(18));
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
void st_push_symbol(Symbol symbol) {
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
        Symbol symbol_value = {
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
        Symbol symbol_value = {
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

/// @brief Generates the `elf.txt` file organized as `{OP} {L} {M}` per line
void pcode_elf() {
    FILE *elf = fopen("elf.txt", "w");
    for (int i = 0; i < PCODE_LEN; i++) {
        Register reg = pcode[i];
        fprintf(elf, "%d %d %d\n", reg.OP, reg.L, reg.M);
    }
    fclose(elf);
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
        return Err(throw(17));
    }
    return Ok(&(self->tokens[iteration]));
}

/// @brief Returns the current iteration's token. Panics if the current iteration is invalid.
/// @return `struct Token`
Token ts_token(TokenStream *self) {
    MayFail fail = ts_get_token(self, TS_ITERATION);
    if (fail.is_error) {
        printf("Fatal error at ts_token [unwrap]: %s", fail.error_message);
        exit(0);
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
    try(st_find_index_or_throw(meta, 7, out_index));
    return Ok(NULL);
}

/// @brief Starts to parse the token stream. Note that all following functions will have similar signature.
/// In Rust, the `&self` automatically has type `Self`, which refers to the type it is implementing the method to.
/// In this case it is struct `TokenStream`. A regular pointer is already mutable in C, and adding `const` modifier
/// to it won't make any difference since the first reference (base) is already mutable.
/// @return `struct MayFail` with `void*` null
MayFail ts_program(TokenStream *self) {
    /// If there's any skip symbol, there's no point on start parsing
    for (int i = 0; i < self->len_tokens; i++) {
        if (self->tokens[i].kind == TokenType_Skip) {
            free(self->tokens);
            /// Return skip symbol present error
            return Err(throw(0));
        }
    }

    TokenType last_token = self->tokens[self->len_tokens - 1].kind;
    if (last_token != TokenType_Period) {
        /// Program must end with period symbol
        return Err(throw(1));
    }

    try(ts_block(self));

    /// Update field `mark` for every variable in Symbol table
    for (int i = 0; i < SYMBOL_TABLE_LEN; i++) {
        Symbol *sym = &symbol_table[i];
        sym->mark = 1;
    }

    return Ok(NULL);
}

/// @brief Parses `block`
/// @return `struct MayFail` with `void*` null
MayFail ts_block(TokenStream *self) {
    try(ts_const_declaration(self));

    int number_of_vars = 0;

    try(ts_var_declaration(self, &number_of_vars));
    ts_emit(self, Instruction_INC, number_of_vars + 3);

    try(ts_statement(self));
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

            int ident_offset_d0 = try_cast(int, st_push_const(ident_name, 0));
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
                return Err(throw(3));
            }

            try(st_push_var(ident_name, *count + 2));

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
        try(ts_get_symbol_index(self, &symbol_index_s0));

        Symbol retreived_symbol = try_cast(Symbol, st_get_index(symbol_index_s0));

        if (retreived_symbol.kind != SymbolKind_Var) {
            return Err(throw(8));
        }

        if (retreived_symbol.mark != 0) {
            return Err(throw(19));
        }

        TokenType token = ts_next(self);

        if (token != TokenType_Becomes) {
            return Err(throw(9));
        }

        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_STO, retreived_symbol.addr);
        break;
    }
    case TokenType_Begin: {
        do {
            ts_next(self);

            if (ts_kind(self) == TokenType_End) {
                break;
            }

            try(ts_statement(self));
        } while (ts_kind(self) == TokenType_Semicolon);

        if (ts_kind(self) != TokenType_End) {
            return Err(throw(10));
        }

        ts_next(self);
        break;
    }
    case TokenType_If: {
        ts_next(self);

        try(ts_condition(self));
        int jpc_index_0 = PCODE_LEN;
        ts_emit(self, Instruction_JPC, 0);

        if (ts_kind(self) != TokenType_Then) {
            return Err(throw(11));
        }

        ts_next(self);
        try(ts_statement(self));

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
        try(ts_condition(self));

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
        try(ts_get_symbol_index(self, &symbol_index_s1));

        Symbol retreived_symbol_2 = try_cast(Symbol, st_get_index(symbol_index_s1));

        if (retreived_symbol_2.kind != SymbolKind_Var) {
            return Err(throw(8));
        }

        ts_next(self);
        ts_emit(self, Instruction_SYS, Syscall_Read);
        ts_emit(self, Instruction_STO, retreived_symbol_2.addr);
        break;
    }
    case TokenType_Write: {
        ts_next(self);
        try(ts_expression(self));

        ts_emit(self, Instruction_SYS, Syscall_Write);
        break;
    }
    default: {
        TokenType this_token = ts_kind(self);
        char message[256];
        snprintf(message, sizeof message, "Found unexpected token id: %d at iteration %d", this_token, self->iteration);
        return Err(message);
    }
    }

    return Ok(NULL);
}

MayFail ts_condition(TokenStream *self) {
    try(ts_expression(self));

    switch (ts_kind(self)) {
    case TokenType_Eq: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, OprCode_EQL);
        break;
    }
    case TokenType_Neq: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, OprCode_NEG);
        break;
    }
    case TokenType_Les: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, OprCode_LSS);
        break;
    }
    case TokenType_Leq: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, OprCode_LEQ);
        break;
    }
    case TokenType_Gtr: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, OprCode_GTR);
        break;
    }
    case TokenType_Geq: {
        ts_next(self);
        try(ts_expression(self));
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

        try(ts_term(self));
        ts_emit(self, Instruction_OPR, OprCode_NEG);

        while (ts_kind(self) == TokenType_Plus || ts_kind(self) == TokenType_Minus) {
            if (ts_kind(self) == TokenType_Plus) {
                ts_next(self);
                try(ts_term(self));
                ts_emit(self, Instruction_OPR, OprCode_ADD);
            } else {
                ts_next(self);
                try(ts_term(self));
                ts_emit(self, Instruction_OPR, OprCode_SUB);
            }
        }
    } else {
        if (ts_kind(self) == TokenType_Plus) {
            ts_next(self);
        }

        try(ts_term(self));

        while (ts_kind(self) == TokenType_Plus || ts_kind(self) == TokenType_Minus) {
            if (ts_kind(self) == TokenType_Plus) {
                ts_next(self);
                try(ts_term(self));
                ts_emit(self, Instruction_OPR, OprCode_ADD);
            } else {
                ts_next(self);
                try(ts_term(self));
                ts_emit(self, Instruction_OPR, OprCode_SUB);
            }
        }
    }

    return Ok(NULL);
}

MayFail ts_term(TokenStream *self) {
    try(ts_factor(self));

    while (ts_kind(self) == TokenType_Mult || ts_kind(self) == TokenType_Slash) {
        if (ts_kind(self) == TokenType_Mult) {
            ts_next(self);
            try(ts_factor(self));
            ts_emit(self, Instruction_OPR, OprCode_MUL);
        } else {
            ts_next(self);
            try(ts_factor(self));
            ts_emit(self, Instruction_OPR, OprCode_DIV);
        }
    }

    return Ok(NULL);
}

MayFail ts_factor(TokenStream *self) {
    switch (ts_kind(self)) {
    case TokenType_Ident: {
        int symbol_index = 0;
        try(ts_get_symbol_index(self, &symbol_index));

        if (symbol_table[symbol_index].mark != 0) {
            return Err(throw(19));
        }

        Symbol this_symbol = try_cast(Symbol, st_get_index(symbol_index));

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
        try(ts_expression(self));
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
    return atoi(token.meta);
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
                puts("Function std::realloc failed");
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

        /// Write the error message to `elf.txt` file
        FILE *elf = fopen("elf.txt", "w");
        fprintf(elf, "%s", program.error_message);
        fclose(elf);
        return 0;
    }

    /// Prints the assembly instructions generated
    pcode_print();
    puts("");

    /// Prints the symbol table
    st_print();

    /// Generates the `elf.txt` file
    pcode_elf();

    free(tokens);
    return 0;
}

/*
Rust file of origin. When the first code generation was successful, I translated it all to C,
and did not update it afterwards.

#![allow(non_snake_case, dead_code)]

use std::{
    fmt::Display,
    ops::{Deref, DerefMut},
};

macro_rules! throw {
    ($code:expr) => {
        MayFail::Err(throw($code).into())
    };
}

macro_rules! impl_usize_cast {
    ($($enum:ty),*) => {
        $(
            impl From<$enum> for usize {
                fn from(value: $enum) -> Self {
                    value as usize
                }
            }
        )*
    };
}

impl_usize_cast!(TokenType, OprCode, Syscall);

macro_rules! impl_derefs {
    ($(($source:ty, $target:ty)),*) => {
        $(
            impl Deref for $source {
                type Target = $target;

                fn deref(&self) -> &Self::Target {
                    &self.0
                }
            }

            impl DerefMut for $source {
                fn deref_mut(&mut self) -> &mut Self::Target {
                    &mut self.0
                }
            }
        )*
    };
}

impl_derefs![(SymbolTable, Vec<Symbol>), (PCode, Vec<Register>)];

fn throw(code: i32) -> &'static str {
    match code {
        1 => "Error: program must end with period",
        2 => "Error: const, var, and read keywords must be followed by identifier",
        3 => "Error: symbol name has already been declared",
        4 => "Error: constants must be assigned with =",
        5 => "Error: constants must be assigned an integer value",
        6 => "Error: constant and variable declarations must be followed by a semicolon",
        7 => "Error: undeclared identifier",
        8 => "Error: only variable values may be altered",
        9 => "Error: assignment statements must use :=",
        10 => "Error: begin must be followed by end",
        11 => "Error: if must be followed by then",
        12 => "Error: while must be followed by do",
        13 => "Error: condition must contain comparison operator",
        14 => "Error: right parenthesis must follow left parenthesis",
        15 => "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols",
        16 => "Error: if must have a condition and be followed by fi",
        _ => unreachable!("Unknown error kind"),
    }
}

enum OprCode {
    RTN,
    ADD,
    SUB,
    MUL,
    DIV,
    EQL,
    NEG,
    LSS,
    LEQ,
    GTR,
    GEQ,
    EVEN,
}

enum Syscall {
    Write = 1,
    Read,
    Halt,
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum TokenType {
    Skip = 1,
    Ident,
    Number,
    Plus,
    Minus,
    Mult,
    Slash,
    Eq,
    Neq,
    Les,
    Leq,
    Gtr,
    Geq,
    Lparent,
    Rparent,
    Comma,
    Semicolon,
    Period,
    Becomes,
    Begin,
    End,
    If,
    Fi,
    Then,
    While,
    Do,
    Call,
    Const,
    Var,
    Proc,
    Write,
    Read,
    Else,
    Even,
}

struct Register {
    OP: Instruction,
    L: usize,
    M: usize,
}

#[derive(Clone, Copy, Debug)]
enum Instruction {
    LIT = 1,
    OPR,
    LOD,
    STO,
    CAL,
    INC,
    JMP,
    JPC,
    SYS,
}

struct PCode(Vec<Register>);

impl PCode {
    fn new() -> Self {
        Self(vec![Register {
            OP: Instruction::JMP,
            L: 0,
            M: 3,
        }])
    }

    fn print(&self) {
        println!("{self}\n");
    }

    fn finish(self) -> MayFail {
        let mut contents = String::new();
        for register in self.iter() {
            contents.push_str(&format!(
                "{OP} {L} {M}\n",
                OP = register.OP as usize,
                L = register.L,
                M = register.M
            ));
        }
        Ok(std::fs::write("elf.txt", contents)?)
    }
}

impl Display for PCode {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut contents = format!(
            "Assembly Code:\n\n{:>4}{:>8}{:>8}{:>8}\n",
            "Line", "OP", "L", "M"
        );
        let result = self
            .iter()
            .enumerate()
            .map(|(index, register)| {
                let op = format!("{:?}", register.OP);

                format!(
                    "{index:>3} {OP:>8}{L:>8}{M:>8}",
                    OP = op,
                    L = register.L,
                    M = register.M
                )
            })
            .collect::<Vec<_>>()
            .join("\n");
        contents.push_str(&result);
        write!(f, "{contents}")
    }
}

type MayFail<T = ()> = Result<T, Box<dyn std::error::Error>>;

struct SymbolTable(Vec<Symbol>);

impl SymbolTable {
    fn new() -> Self {
        Self(Vec::new())
    }

    pub fn print(&self) {
        println!(
            "Symbol Table: \n\n{kind:<4} | {name:<11} | {value:<5} | {level:<5} | {addr:<7} | {mark:<4}",
            kind = "Kind",
            name = "Name",
            value = "Value",
            level = "Level",
            addr = "Address",
            mark = "Mark"
        );
        for _ in 0..51 {
            print!("-");
        }
        println!();
        for symbol in self.iter() {
            print!(
                "{kind:>4} | {name:>11} | {value:>5} | {level:>5} | {addr:>7} | {mark:>4}\n",
                kind = symbol.kind as usize,
                name = symbol.name,
                value = symbol.value,
                level = symbol.level,
                addr = symbol.addr,
                mark = symbol.mark as u8
            );
        }
    }

    fn has_symbol(&self, name: &str) -> bool {
        self.iter().any(|symbol| symbol.name == name)
    }

    fn get_index(&self, index: usize) -> MayFail<&Symbol> {
        self.get(index)
            .ok_or(format!("Index {index} does not exist in SymbolTable").into())
    }

    fn find_index_or_throw(&self, name: &str, code: i32) -> MayFail<usize> {
        Ok(self
            .iter()
            .enumerate()
            .find(|(_, symbol)| symbol.name == name)
            .map(|(index, _)| index)
            .ok_or(throw(code))?)
    }

    fn push_symbol(&mut self, symbol: Symbol) {
        self.push(symbol);
    }

    fn push_const(&mut self, name: String, value: usize) -> MayFail<usize> {
        self.find_index_or_throw(&name, 3)?;
        self.push_symbol(Symbol {
            kind: SymbolKind::Const,
            name,
            value,
            level: 0,
            addr: 0,
            mark: false,
        });
        Ok(self.len())
    }

    // In C translation, this function will never throw an error.
    fn push_var(&mut self, name: String, addr: usize) -> MayFail<usize> {
        match self.find_index_or_throw(&name, 3) {
            Err(_) => {
                self.push_symbol(Symbol {
                    kind: SymbolKind::Var,
                    name,
                    addr,
                    value: 0,
                    level: 0,
                    mark: false,
                });
                Ok(self.len())
            }
            Ok(_) => throw!(3),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum SymbolKind {
    Const = 1,
    Var,
    Proc,
}

struct Symbol {
    kind: SymbolKind,
    name: String,
    value: usize,
    level: usize,
    addr: usize,
    mark: bool,
}

#[derive(Clone, Debug, PartialEq)]
struct Token {
    kind: TokenType,
    meta: Option<String>,
}

impl Token {
    fn get_number(&self) -> MayFail<usize> {
        match self.meta {
            Some(ref meta) => Ok(meta.parse::<usize>()?),
            None => Err("Called get_number() in a Token that is not TokenType::Number".into()),
        }
    }

    fn get_meta(&self) -> MayFail<&String> {
        Ok(self.meta.as_ref().ok_or(format!(
            "Found a TokenType::{:?} without metadata",
            self.kind
        ))?)
    }
}

impl From<TokenType> for Token {
    fn from(value: TokenType) -> Self {
        Self {
            kind: value,
            meta: None,
        }
    }
}

impl Into<TokenType> for Token {
    fn into(self) -> TokenType {
        self.kind
    }
}

impl From<&str> for Token {
    fn from(value: &str) -> Self {
        let mut kind = TokenType::Skip;
        let mut meta = None;
        let parts = value.trim().split(" ");
        for (index, part) in parts.enumerate() {
            if part.len() > 11 {
                continue;
            }

            match index {
                0 => kind = unsafe { std::mem::transmute(part.parse::<u8>().unwrap()) },
                1 => meta = Some(part.to_string()),
                _ => unreachable!("More than two tokens found in a Token line"),
            }
        }
        Self { kind, meta }
    }
}

/// Every field in this struct will be transformed in a static mut.
struct TokenStream {
    iteration: usize,
    tokens: Vec<Token>,
    symbol_table: SymbolTable,
    pcode: PCode,
}

impl TokenStream {
    fn jpc_offset(&mut self) -> usize {
        self.pcode.len() * 3
    }

    fn emit(&mut self, instruction: Instruction, M: impl Into<usize>) {
        self.pcode.push(Register {
            OP: instruction,
            L: 0,
            M: M.into(),
        })
    }

    fn get_token(&self, iteration: usize) -> MayFail<&Token> {
        assert!(iteration < self.tokens.len());
        self.tokens
            .get(iteration)
            .ok_or(format!("Index {iteration} does not exist in TokenStream").into())
    }

    fn token(&self) -> &Token {
        if self.iteration == self.tokens.len() {
            panic!("TokenStream is out of tokens");
        }
        self.get_token(self.iteration).unwrap()
    }

    fn kind(&self) -> TokenType {
        self.token().kind
    }

    fn next(&mut self) -> TokenType {
        self.iteration += 1;
        self.kind()
    }

    fn get_symbol_index(&self) -> MayFail<usize> {
        self.symbol_table
            .find_index_or_throw(self.token().get_meta()?, 7)
    }

    fn get_symbol(&self, symbol_index: usize) -> MayFail<&Symbol> {
        self.symbol_table.get_index(symbol_index)
    }

    fn __dbg(&self) {
        println!("---------------------------------------------------------------------------");
        println!("[-2] token: {:?}", self.get_token(self.iteration - 2));
        println!("Previous token: {:?}", self.get_token(self.iteration - 1));
        println!("Current token:  {:?}", self.token());
        if self.iteration + 1 < self.tokens.len() {
            println!("Next token:     {:?}", self.get_token(self.iteration + 1));
        } else {
            println!("This is the last token");
        }
        println!("---------------------------------------------------------------------------");
    }

    fn program(&mut self) -> MayFail {
        match self.tokens.ends_with(&[TokenType::Period.into()]) {
            false => throw!(1),
            true => self.block(),
        }
    }

    fn block(&mut self) -> MayFail {
        self.const_declaration()?;
        let number_of_vars = self.var_declaration()?;
        self.emit(Instruction::INC, number_of_vars + 3);
        self.statement()?;
        self.emit(Instruction::SYS, Syscall::Halt);
        Ok(())
    }

    fn const_declaration(&mut self) -> MayFail {
        if self.token() == TokenType::Const {
            loop {
                let token = self.next();

                if token != TokenType::Ident {
                    return throw!(2);
                }

                let ident_name = self.token().get_meta()?;

                if self.symbol_table.has_symbol(ident_name) {
                    return throw!(3);
                }

                let ident_offset = self.symbol_table.push_const(ident_name.clone(), 0)?;
                let token = self.next();

                if token != TokenType::Eq {
                    return throw!(4);
                }

                let token = self.next();

                if token != TokenType::Number {
                    return throw!(5);
                }

                let ident_value = self.token().get_number()?;
                self.symbol_table[ident_offset].value = ident_value;

                self.next();

                if self.token() != TokenType::Comma {
                    break;
                }
            }

            if self.token() != TokenType::Semicolon {
                return throw!(6);
            }

            self.next();
        }
        Ok(())
    }

    fn var_declaration(&mut self) -> MayFail<usize> {
        let mut count = 0;

        if self.token() == TokenType::Var {
            loop {
                count += 1;

                let token = self.next();

                if token != TokenType::Ident {
                    return throw!(2);
                }

                let ident_name = self.token().get_meta()?;

                if self.symbol_table.has_symbol(ident_name) {
                    return throw!(3);
                }

                self.symbol_table.push_var(ident_name.clone(), count + 2)?;
                self.next();

                if self.token() != TokenType::Comma {
                    break;
                }
            }

            if self.token() != TokenType::Semicolon {
                return throw!(6);
            }
        }

        self.next();

        Ok(count)
    }

    fn statement(&mut self) -> MayFail {
        match self.kind() {
            TokenType::Ident => {
                let symbol_index = self.get_symbol_index()?;

                if self.get_symbol(symbol_index)?.kind != SymbolKind::Var {
                    return throw!(8);
                }

                let token = self.next();

                if token != TokenType::Becomes {
                    return throw!(9);
                }

                self.next();
                self.expression()?;
                self.emit(Instruction::STO, self.get_symbol(symbol_index)?.addr);
            }
            TokenType::Begin => {
                loop {
                    self.next();

                    if self.token() == TokenType::End {
                        break;
                    }

                    self.statement()?;

                    if self.token() != TokenType::Semicolon {
                        break;
                    }
                }

                if self.token() != TokenType::End {
                    return throw!(10);
                }

                self.next();
            }
            TokenType::If => {
                self.next();
                self.condition()?;

                let jpc_index = self.pcode.len();
                self.emit(Instruction::JPC, 0usize);

                if self.token() != TokenType::Then {
                    return throw!(11);
                }

                self.next();
                self.statement()?;

                if self.token() != TokenType::Fi {
                    return throw!(16);
                }

                self.next();

                self.pcode[jpc_index].M = self.jpc_offset();
            }
            TokenType::While => {
                self.next();

                let loop_index = self.pcode.len();
                self.condition()?;

                if self.token() != TokenType::Do {
                    return throw!(12);
                }

                self.next();

                let jpc_index = self.pcode.len();

                self.emit(Instruction::JPC, 0usize);
                self.statement()?;
                self.emit(Instruction::JMP, loop_index);

                self.pcode[jpc_index].M = self.jpc_offset();
            }
            TokenType::Read => {
                self.next();

                if self.token() != TokenType::Ident {
                    return throw!(2);
                }

                let symbol_index = self.get_symbol_index()?;

                if self.get_symbol(symbol_index)?.kind != SymbolKind::Var {
                    return throw!(8);
                }

                self.next();
                self.emit(Instruction::SYS, Syscall::Read);
                self.emit(Instruction::STO, self.get_symbol(symbol_index)?.addr);
            }
            TokenType::Write => {
                self.next();
                self.expression()?;
                self.emit(Instruction::SYS, Syscall::Write);
            }
            token => unreachable!("Statement received an unknown token type: TokenType::{token:?}"),
        };

        Ok(())
    }

    fn condition(&mut self) -> MayFail {
        self.expression()?;

        macro_rules! assert_kind {
            ($(($token:ident, $instr:ident)),*) => {
                match self.kind() {
                    $(
                        TokenType::$token => {
                            self.next();
                            self.expression()?;
                            self.emit(Instruction::OPR, OprCode::$instr);
                            Ok(())
                        }
                    )*
                    _ => throw!(15)
                }
            };
        }

        assert_kind![
            (Eq, EQL),
            (Neq, NEG),
            (Les, LSS),
            (Leq, LEQ),
            (Gtr, GTR),
            (Geq, GEQ)
        ]
    }

    fn expression(&mut self) -> MayFail {
        macro_rules! match_kind {
            () => {
                while matches!(self.kind(), TokenType::Minus | TokenType::Plus) {
                    match self.kind() {
                        TokenType::Minus => {
                            self.next();
                            self.term()?;
                            self.emit(Instruction::OPR, OprCode::SUB);
                        }
                        TokenType::Plus => {
                            self.next();
                            self.term()?;
                            self.emit(Instruction::OPR, OprCode::ADD);
                        }
                        _ => unreachable!(),
                    }
                }
            };
        }

        match self.kind() {
            TokenType::Minus => {
                self.next();
                self.term()?;
                self.emit(Instruction::OPR, OprCode::NEG);
                match_kind!();
            }
            kind => {
                if kind == TokenType::Plus {
                    self.next();
                }
                self.term()?;
                match_kind!();
            }
        }
        Ok(())
    }

    fn term(&mut self) -> MayFail {
        self.factor()?;

        macro_rules! match_kind {
            ($(($token:ident, $instr:ident)),*) => {
                while matches!(self.kind(), $(TokenType::$token)|*) {
                    match self.kind() {
                        $(
                            TokenType::$token => {
                                self.next();
                                self.factor()?;
                                self.emit(Instruction::OPR, OprCode::$instr);
                            }
                        )*
                        _ => unreachable!(),
                    }
                }
            };
        }

        Ok(match_kind![(Mult, MUL), (Slash, DIV)])
    }

    fn factor(&mut self) -> MayFail {
        match self.kind() {
            TokenType::Ident => {
                let symbol_index = self.get_symbol_index()?;

                self.symbol_table[symbol_index].mark = true;

                let symbol = self.get_symbol(symbol_index)?;

                if symbol.kind == SymbolKind::Const {
                    self.emit(Instruction::LIT, symbol.value);
                } else {
                    self.emit(Instruction::LOD, symbol.addr);
                }

                self.next();
            }
            TokenType::Number => {
                let value = self.token().get_number()?;
                self.emit(Instruction::LIT, value);
                self.next();
            }
            TokenType::Lparent => {
                self.next();
                self.expression()?;

                if self.token() != TokenType::Rparent {
                    return throw!(14);
                }

                self.next();
            }
            _ => return throw!(15),
        }

        Ok(())
    }
}

impl From<Vec<Token>> for TokenStream {
    fn from(value: Vec<Token>) -> Self {
        Self {
            iteration: 0,
            tokens: value,
            symbol_table: SymbolTable::new(),
            pcode: PCode::new(),
        }
    }
}

impl PartialEq<TokenType> for &Token {
    fn eq(&self, other: &TokenType) -> bool {
        self.kind == *other
    }
}

fn main() -> MayFail {
    let file = std::fs::read_to_string("tokens.txt")?;

    let tokens = file
        .lines()
        .into_iter()
        .filter(|line| !line.is_empty() && !line.chars().all(char::is_whitespace))
        .map(Token::from)
        .collect::<Vec<_>>();

    let mut token_stream = TokenStream::from(tokens);

    if let Err(e) = token_stream.program() {
        println!("{e:?}");
        return Ok(());
    }

    token_stream.pcode.print();
    token_stream.symbol_table.print();
    token_stream.pcode.finish()
}
*/