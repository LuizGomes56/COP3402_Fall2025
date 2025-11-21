/*
Assignment:
HW4 - Complete Parser and Code Generator for PL/0
(with Procedures, Call, and Else)
Author(s): Luiz Gustavo Santana Dias Gomes
Language: C (only)
To Compile:
Scanner:
gcc -O2 -std=c11 -o lex lex.c
Parser/Code Generator:
gcc -O2 -std=c11 -o parsercodegen_complete parsercodegen_complete.c
Virtual Machine:
gcc -O2 -std=c11 -o vm vm.c
To Execute (on Eustis):
./lex <input_file.txt>
./parsercodegen_complete
./vm elf.txt
where:
<input_file.txt> is the path to the PL/0 source program
Notes:
- lex.c accepts ONE command-line argument (input PL/0 source file)
- parsercodegen_complete.c accepts NO command-line arguments
- Input filename is hard-coded in parsercodegen_complete.c
- Implements recursive-descent parser for extended PL/0 grammar
- Supports procedures, call statements, and if-then-else
- Generates PM/0 assembly code (see Appendix A for ISA)
- VM must support EVEN instruction (OPR 0 11)
- All development and testing performed on Eustis
Class: COP3402 - System Software - Fall 2025
Instructor: Dr. Jie Lin
Due Date: Friday, November 21, 2025 at 11:59 PM ET
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_PCODE_SIZE 5000

// ! There are debug symbols commented over the code and they can be used to verify in which
// ! Iteration, Code Length, Symbol type, Token Type, line and column the error was thrown
// ! like a full Rust-backtrace. Rust implementation was omitted

// ? Functions that need an argument to return indexes are effectively useless. They could use
// ? A heap-alloc integer pointer to return the adderess -> `Box<i32>` inside of the Ok.value
// ? `void*` field, and never free it (as it is in the whole program, where most memory is never)
// ? freed (and it does not matter because almost everything lives on the stack in this parser)

/// Creating a new helper macro to propagate errors through functions that may fail
/// Same as Rust's `?` operator. Propagates the error if MayFail failed.
#define try(expr)              \
    do {                       \
        MayFail __mf = (expr); \
        if (__mf.is_error)     \
            return __mf;       \
    } while (0)

/// Propagates the error. If it is successful, gets the value by casting the `value` field.
/// that the `MayFail` type has returned. First argument must be the type to cast to.
/// This will succeed if and only if the returned type is not stack-allocated (such as an integer)
/// Integers are not guaranteed to remain in the same address after a function call, so it is
/// Undefined behavior or most of times will just return zero if T: int
#define try_cast(T, expr) \
    ({ MayFail __mf = (expr);       \
    if (__mf.is_error) return __mf; \
    *(T *)__mf.value; })

/// @brief Port the MayFail type from Rust to C. Field `value` must be
/// casted to something else to be used. Example:
/// ```c
/// MayFail symbol = get_some_symbol(self);
/// Symbol some_symbol = *(Symbol *)success.value;
/// ```
/// I'm using the macro `try` and `try_cast` instead of manually doing it all the time.
/// Since C has no operators to propagate errors, this has to be manually done and pollutes the code.
/// However this is still the best way to guarantee that the program won't crash and avoid doing
/// weird comparisons such as `if (x == -1) { ... }` that says nothing about the error (error was not a value),
/// and has much less flexibility in its return type (must be fixed type instead of any such as in this implementation).
/// This is the same as `Result<T, E> where T: *const (), E: &'static str`. T is a void pointer and
/// behave the same way in here -> can be unsafely casted to anything. Crashes if the cast was done to the wrong type.
typedef struct MayFail {
    int is_error;
    /// @brief Can be anything after all. A pointer that points to anything
    void *value;
    /// @brief If `is_error` is true, this is a pointer to a string
    char *error_message;
} MayFail;

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
    OprCode_NEQ,
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
    Token *tokens;
    int len_tokens;
} TokenStream;

/// @brief Previously defined in TokenStream struct (Rust)
static Register pcode[MAX_PCODE_SIZE];

/// @brief Current PCODE index
static int PCODE_LEN = 0;
/// @brief Lexographical level the program is currently at.
static int LEVEL = 0;

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
MayFail ts_procedure_declaration(TokenStream *self);
MayFail ts_statement(TokenStream *self);
MayFail ts_condition(TokenStream *self);
MayFail ts_expression(TokenStream *self);
MayFail ts_term(TokenStream *self);
MayFail ts_factor(TokenStream *self);
MayFail ts_get_token(TokenStream *self, int iteration);
MayFail st_find_symbol_eq(Symbol f_symbol);
int token_get_number(Token token);
void dbg_symbol(Symbol symbol);

/// Error codes defined in the assignment details
char *throw(int code) {
    // In the tokens.txt file is split in lines of tokens.
    // Uncommenting this will show in which one of them the error was thrown
    // printf("Error ocurred on token iteration %d\n", TS_ITERATION);
    switch (code) {
    /// Added error if a skip symbol was found
    case 0:
        return "Error: Scanning error detected by lexer (skipsym present)";
    case 1:
        return "Error: program must end with period";
    case 2:
        return "Error: const, var, read, procedure, and call keywords must be followed by identifier";
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
    /// Case "16" error was added in HW4 assignment and HW1 had a custom error type
    case 17:
        return "Error: Assertion (iteration >= self->len_tokens) failed [IOB]";
    case 18:
        return "Error: Symbol table indexation failed: Index out of bounds";
    case 19:
        return "Error: Symbol is no longer usable (mark = 1, maybe it is out of scope?)";
    case 20:
        return "Error: Unexpected token sequence after 'even'";
    case 21:
        return "Error: else must be followed by fi";
    case 22:
        return "Error: if statement must include else clause";
    case 23:
        return "Error: procedure declaration must be followed by a semicolon";
    case 24:
        return "Error: call statement may only target procedures";
    default:
        return "Error: Unknown error code passed to 'throw' function";
    }
}

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
        Symbol symbol = symbol_table[i];
        // dbg_symbol(symbol);
        if (strcmp(symbol.name, name) == 0 && symbol.mark == 0 && symbol.level == LEVEL) {
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

/// @brief Assigns to `out_index` the index of the symbol. Can fail, [Loop backwards]
/// @param name Name of the ident to be searched for
/// @param code Code to be called in function `throw` if an error occur.
/// @param out_index Variable where the index will be assigned, if it exists
/// @return `struct MayFail` with `void*` null
/// *out_index wouldn't be necessary if the integer was allocated on heap - I could have changed it
/// but at this point it appears in so many places that I'll like as it is.
MayFail st_find_index_or_throw(char *name, int code, int *out_index) {
    // printf("Looking for symbol [name]: %s\n", name);
    for (int i = SYMBOL_TABLE_LEN - 1; i >= 0; i--) {
        Symbol symbol = symbol_table[i];
        // dbg_symbol(symbol);
        if (strcmp(symbol.name, name) == 0 && symbol.mark == 0) {
            *out_index = i;
            return Ok(NULL);
        }
    }
    return Err(throw(code));
}

/// @brief Takes in a symbol and looks in the symbol table for some stored symbol that is identical to `f_symbol`
/// @param f_symbol The symbol to be fully compared (Same as Rust's `PartialEq` trait)
/// @return `Ok(int*)`, `Err(7)`
MayFail st_find_symbol_eq(Symbol f_symbol) {
    int *out_index = malloc(sizeof(int));
    // printf("Looking for symbol [name]: %s\n", name);
    for (int i = SYMBOL_TABLE_LEN - 1; i >= 0; i--) {
        Symbol symbol = symbol_table[i];
        // dbg_symbol(symbol);
        if (
            strcmp(symbol.name, f_symbol.name) == 0 && symbol.val == f_symbol.val && symbol.addr == f_symbol.addr && symbol.mark == f_symbol.mark && symbol.level == f_symbol.level && symbol.kind == f_symbol.kind) {
            *out_index = i;
            return Ok(out_index);
        }
    }
    return Err(throw(7));
}

/// @brief Inserts a symbol to the symbol table. Can't fail.
/// @param symbol Symbol to be inserted
void st_push_symbol(Symbol symbol) {
    symbol_table[SYMBOL_TABLE_LEN] = symbol;
    SYMBOL_TABLE_LEN++;
}

/// @brief Pushes a value to the symbol table, checking if it already exists
/// @param symbol Symbol definition
/// @return `struct MayFail`. Fails if the symbol already exists. `void*` is of type `int`
/// representing the index of the symbol that was just inserted
MayFail st_push_value(Symbol symbol) {
    // We can add symbols with the same name, as long as they are in different levels.
    // So a simple by-name lookup is not enough (function `st_find_index_or_throw`),
    // so this function does a full comparison to find the symbol. We can only add if it is
    // not found in the symbol table.
    MayFail fail = st_find_symbol_eq(symbol);
    if (fail.is_error) {
        st_push_symbol(symbol);
        return Ok(fail.value);
    } else {
        // dbg_symbol(symbol);
        // st_print();
        // printf("Symbol literal %s already exists\n", symbol.name);

        // If the literal symbol is already present in the symbol table, throw the
        // already declared error
        return Err(fail.error_message);
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

/// @brief Adds an instruction to the `pcode` array
/// @param self `TokenStream` adapt
/// @param instr Instruction code
/// @param M M value
void ts_emit(TokenStream *self, Instruction instr, int level, int M) {
    Register reg = {
        .OP = instr,
        .L = level,
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
        printf("Error: Called unwrap on Err(ts_get_token(self, TS_ITERATION)): %s", fail.error_message);
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
    // printf("Meta: %s \n", meta);
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

    int start_index = PCODE_LEN;
    ts_emit(self, Instruction_JMP, 0, 0);

    LEVEL = 0;

    int main_start = try_cast(int, ts_block(self));

    pcode[start_index].M = 3 * main_start;

    ts_emit(self, Instruction_SYS, 0, Syscall_Halt);

    return Ok(NULL);
}

/// @brief Parses `block`
/// @return `struct MayFail` with `void*` being the index of the `INC` instruction
MayFail ts_block(TokenStream *self) {
    try(ts_const_declaration(self));

    int number_of_vars = 0;

    try(ts_var_declaration(self, &number_of_vars));

    // ts_emit(self, Instruction_JMP, 0, 0);

    // printf("Start at index: %d\n", start);
    try(ts_procedure_declaration(self));

    int *inc_start = malloc(sizeof(int));
    *inc_start = PCODE_LEN;

    ts_emit(self, Instruction_INC, 0, number_of_vars + 3);

    // printf("Starting statement after proc decl, level: %d, LEN: %d\n", LEVEL, PCODE_LEN);
    // pcode_print();

    try(ts_statement(self));

    /// Update field `mark` for every variable in Symbol table
    for (int i = 0; i < SYMBOL_TABLE_LEN; i++) {
        if (symbol_table[i].level != LEVEL) {
            continue;
        }
        symbol_table[i].mark = 1;
    }

    return Ok(inc_start);
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

            Symbol symbol_value = {
                .name = {0},
                .kind = SymbolKind_Const,
                .val = 0,
                .level = LEVEL,
                .addr = 0,
                .mark = 0,
            };

            memcpy(symbol_value.name, ident_name, sizeof symbol_value.name);
            symbol_value.name[sizeof symbol_value.name - 1] = '\0';

            try(st_push_value(symbol_value));
            int ident_offset = SYMBOL_TABLE_LEN - 1;

            TokenType token_d1 = ts_next(self);

            if (token_d1 != TokenType_Eq) {
                return Err(throw(4));
            }

            int token_d2 = ts_next(self);

            if (token_d2 != TokenType_Number) {
                return Err(throw(5));
            }

            symbol_table[ident_offset].val = token_get_number(ts_token(self));

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

            Symbol symbol_value = {
                .name = {0},
                .kind = SymbolKind_Var,
                .val = 0,
                .level = LEVEL,
                .addr = *count + 2,
                .mark = 0,
            };

            memcpy(symbol_value.name, ident_name, sizeof symbol_value.name);
            symbol_value.name[sizeof symbol_value.name - 1] = '\0';

            try(st_push_value(symbol_value));

            ts_next(self);
        } while (ts_kind(self) == TokenType_Comma);

        if (ts_kind(self) != TokenType_Semicolon) {
            return Err(throw(6));
        }
        ts_next(self);
    }

    return Ok(NULL);
}

/// @brief Parses procedures and keeps their records of LEVEL (updating globally)
/// @param self Context
/// @return Nothing
MayFail ts_procedure_declaration(TokenStream *self) {
    while (ts_kind(self) == TokenType_Proc) {
        if (ts_next(self) != TokenType_Ident) {
            return Err(throw(2));
        }

        Token token = ts_token(self);

        if (ts_next(self) != TokenType_Semicolon) {
            return Err(throw(23));
        }

        int current_len = PCODE_LEN;
        ts_emit(self, Instruction_JMP, 0, 0);

        ts_next(self);
        Symbol symbol_value = {
            .name = {0},
            .kind = SymbolKind_Proc,
            .val = 0,
            .level = LEVEL,
            .addr = 3 * current_len,
            .mark = 0,
        };

        memcpy(symbol_value.name, token.meta, sizeof symbol_value.name);
        symbol_value.name[sizeof symbol_value.name - 1] = '\0';

        try(st_push_value(symbol_value));
        LEVEL++;

        int body_start = try_cast(int, ts_block(self));
        // puts("Entering block inside proc");
        // puts("Left block of proc");
        // printf("LEVEL: %d\n", LEVEL);

        ts_emit(self, Instruction_OPR, 0, OprCode_RTN);

        LEVEL--;
        // printf("[1] LEVEL: %d\n", LEVEL);

        pcode[current_len].M = 3 * body_start;

        if (ts_kind(self) != TokenType_Semicolon) {
            // Unespecified error type; Procedures should have a semicolon but nothing was
            // provided in the assignment about it
            return Err("Error: Expected semicolon after block of procedure declaration");
        }

        ts_next(self);
    }
    return Ok(NULL);
}

MayFail ts_statement(TokenStream *self) {
    Token t = ts_token(self);
    // printf("Kind: %d, Meta: %s, Level: %d\n", t.kind, t.meta, LEVEL);
    switch (ts_kind(self)) {
    case TokenType_Ident: {
        int symbol_index = 0;
        // st_print();
        try(ts_get_symbol_index(self, &symbol_index));

        Symbol retreived_symbol = try_cast(Symbol, st_get_index(symbol_index));

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
        ts_emit(self, Instruction_STO, LEVEL - retreived_symbol.level, retreived_symbol.addr);
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
        int jpc_index = PCODE_LEN;
        ts_emit(self, Instruction_JPC, 0, 0);

        if (ts_kind(self) != TokenType_Then) {
            return Err(throw(11));
        }

        ts_next(self);
        try(ts_statement(self));

        // printf("Token kind: %d\n", ts_kind(self));

        if (ts_kind(self) != TokenType_Else) {
            return Err(throw(22));
        }

        int jmp_index = PCODE_LEN;
        ts_emit(self, Instruction_JMP, 0, 0);

        pcode[jpc_index].M = 3 * PCODE_LEN;
        ts_next(self);

        try(ts_statement(self));

        if (ts_kind(self) != TokenType_Fi) {
            return Err(throw(21));
        }

        ts_next(self);
        pcode[jmp_index].M = 3 * PCODE_LEN;

        break;
    }
    case TokenType_While: {
        ts_next(self);

        int loop_index = 3 * PCODE_LEN;
        try(ts_condition(self));

        if (ts_kind(self) != TokenType_Do) {
            return Err(throw(12));
        }

        ts_next(self);

        int jpc_index = PCODE_LEN;

        ts_emit(self, Instruction_JPC, 0, 0);
        try(ts_statement(self));
        ts_emit(self, Instruction_JMP, 0, loop_index);

        pcode[jpc_index].M = 3 * PCODE_LEN;
        break;
    }
    case TokenType_Read: {
        ts_next(self);

        if (ts_kind(self) != TokenType_Ident) {
            return Err(throw(2));
        }

        int symbol_index = 0;
        try(ts_get_symbol_index(self, &symbol_index));

        Symbol retreived_symbol = try_cast(Symbol, st_get_index(symbol_index));

        if (retreived_symbol.kind != SymbolKind_Var) {
            return Err(throw(8));
        }

        ts_next(self);
        ts_emit(self, Instruction_SYS, 0, Syscall_Read);
        // Current level - symbol level to account for level changes. If current level is 1 and
        // the symbol is also in 1, the STO is L = 0 (current), so only the change is calculated
        ts_emit(self, Instruction_STO, LEVEL - retreived_symbol.level, retreived_symbol.addr);
        break;
    }
    case TokenType_Write: {
        ts_next(self);
        try(ts_expression(self));

        ts_emit(self, Instruction_SYS, 0, Syscall_Write);
        break;
    }
    case TokenType_Call: {
        if (ts_next(self) != TokenType_Ident) {
            return Err(throw(2));
        }
        int index = 0;
        try(ts_get_symbol_index(self, &index));
        Symbol symbol = try_cast(Symbol, st_get_index(index));
        if (symbol.kind != SymbolKind_Proc) {
            return Err(throw(24));
        }
        ts_emit(self, Instruction_CAL, LEVEL - symbol.level, symbol.addr);
        ts_next(self);
        break;
    }
    default: {
        TokenType this_token = ts_kind(self);
        char *message = malloc(256);
        snprintf(message, 256, "Error: Found unexpected token id: %d at iteration %d", this_token, TS_ITERATION);
        // Debug message to display to the console in case of Undefined Behavior. It is likely to happen
        // if some function has wrong return type (MayFail returning anything other than Ok<T> or Err<E>)
        // printf("Error: [default] %s\n", message);
        return Err(message);
    }
    }

    return Ok(NULL);
}

MayFail ts_condition(TokenStream *self) {
    if (ts_kind(self) == TokenType_Even) {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, 0, OprCode_EVEN);
        return Ok(NULL);
    }

    try(ts_expression(self));

    switch (ts_kind(self)) {
    case TokenType_Eq: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, 0, OprCode_EQL);
        break;
    }
    case TokenType_Neq: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, 0, OprCode_NEQ);
        break;
    }
    case TokenType_Les: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, 0, OprCode_LSS);
        break;
    }
    case TokenType_Leq: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, 0, OprCode_LEQ);
        break;
    }
    case TokenType_Gtr: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, 0, OprCode_GTR);
        break;
    }
    case TokenType_Geq: {
        ts_next(self);
        try(ts_expression(self));
        ts_emit(self, Instruction_OPR, 0, OprCode_GEQ);
        break;
    }
    default: {
        return Err(throw(13));
    }
    }
    return Ok(NULL);
}

MayFail ts_expression(TokenStream *self) {
    if (ts_kind(self) == TokenType_Minus) {
        ts_next(self);

        try(ts_term(self));
        ts_emit(self, Instruction_OPR, 0, OprCode_NEQ);

        while (ts_kind(self) == TokenType_Plus || ts_kind(self) == TokenType_Minus) {
            if (ts_kind(self) == TokenType_Plus) {
                ts_next(self);
                try(ts_term(self));
                ts_emit(self, Instruction_OPR, 0, OprCode_ADD);
            } else {
                ts_next(self);
                try(ts_term(self));
                ts_emit(self, Instruction_OPR, 0, OprCode_SUB);
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
                ts_emit(self, Instruction_OPR, 0, OprCode_ADD);
            } else {
                ts_next(self);
                try(ts_term(self));
                ts_emit(self, Instruction_OPR, 0, OprCode_SUB);
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
            ts_emit(self, Instruction_OPR, 0, OprCode_MUL);
        } else {
            ts_next(self);
            try(ts_factor(self));
            ts_emit(self, Instruction_OPR, 0, OprCode_DIV);
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
            ts_emit(self, Instruction_LIT, 0, this_symbol.val);
        } else {
            ts_emit(self, Instruction_LOD, LEVEL - this_symbol.level, this_symbol.addr);
        }
        ts_next(self);
        break;
    }
    case TokenType_Number: {
        int value = token_get_number(ts_token(self));
        ts_emit(self, Instruction_LIT, 0, value);
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

/// @brief If the Token has a metadata, returns it as an integer. Must be called if Token.kind is `TokenType::Number`
/// @return Parsing of token's meta to integer. Same as `token.meta.parse::<i32>().unwrap()`
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

    if (file_ptr == NULL) {
        printf("Error: Could not read tokens.txt file. Check its encoding or if it exists");
        return 0;
    }

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

/// @brief Prints a symbol as in Rust's `Debug` trait.
/// @param symbol
void dbg_symbol(Symbol symbol) {
    printf(
        "Symbol: { Kind: %d, Name: %s, Value: %d, Level: %d, Address: %d, Mark: %d }\n",
        symbol.kind,
        symbol.name,
        symbol.val,
        symbol.level,
        symbol.addr,
        symbol.mark);
}