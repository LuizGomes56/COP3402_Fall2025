#include <stdio.h>

/*
Assignment:
    vm.c - Implement a P-machine virtual machine

Authors:
    Luiz Gustavo Santana Dias Gomes, Daniel Scariti

Language: C (only)

To Compile:
    gcc -O2 -Wall -std=c11 -o vm vm.c

To Execute (on Eustis):
    ./vm input.txt

Where:
    input.txt is the name of the file containing PM/0 instructions;
    each line has three integers (OP L M)

Notes:
    - Implements the PM/0 virtual machine described in the homework instructions.
    - No dynamic memory allocation or pointer arithmetic.
    - Does not implement any VM instruction using a separate function.
    - Runs on Eustis.

Class:
    COP 3402 - Systems Software - Fall 2025

Instructor:
    Dr. Jie Lin

Due Date:
    Friday, September 12th, 2025
*/

// Process Address Space
// .text = Addr 499..0; 3 words per instruction
// .data = below .text section
static int PAS[500] = {0};

// Initialize PC to 499 (Page 3)
// Points to the next instruction in the text segment.
static int PC = 499;

#define LAST_INSTRUCTION 499

/*
```rs
macro_rules! LIT {
    ($sp:expr, $n:expr) => {
        $sp -= 1;
        PAS[$sp] = $n;
    }
}
```
*/
// Literal push
#define LIT(sp, n)   \
    do {             \
        sp--;        \
        PAS[sp] = n; \
    } while (0)
#define __LIT 1

#define __OPR 2

/*
```rs
macro_rules! RTN {
    ($sp:expr, $bp:expr) => {
        $sp = $bp + 1;
        $bp = PAS[$sp - 2];
        PC = PAS[$sp - 3];
    }
}
```
*/
// Return from subroutine and restore caller's AR
#define RTN(sp, bp)       \
    do {                  \
        sp = bp + 1;      \
        bp = PAS[sp - 2]; \
        PC = PAS[sp - 3]; \
    } while (0)
#define __RTN 0

/*
```rs
macro_rules! ADD {
    ($sp:expr) => {
        PAS[$sp + 1] += PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Addition
#define ADD(sp)                 \
    do {                        \
        PAS[sp + 1] += PAS[sp]; \
        sp += 1;                \
    } while (0)
#define __ADD 1

/*
```rs
macro_rules! SUB {
    ($sp:expr) => {
        PAS[$sp + 1] -= PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Subtraction
#define SUB(sp)                 \
    do {                        \
        PAS[sp + 1] -= PAS[sp]; \
        sp += 1;                \
    } while (0)
#define __SUB 2

/*
```rs
macro_rules! MUL {
    ($sp:expr) => {
        PAS[$sp + 1] *= PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Multiplication
#define MUL(sp)                 \
    do {                        \
        PAS[sp + 1] *= PAS[sp]; \
        sp += 1;                \
    } while (0)
#define __MUL 3

/*
```rs
macro_rules! DIV {
    ($sp:expr) => {
        PAS[$sp + 1] /= PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Division
#define DIV(sp)                 \
    do {                        \
        PAS[sp + 1] /= PAS[sp]; \
        sp += 1;                \
    } while (0)
#define __DIV 4

/*
```rs
macro_rules! EQL {
    ($sp:expr) => {
        PAS[$sp + 1] = PAS[$sp + 1] == PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Equality comparison
#define EQL(sp)                               \
    do {                                      \
        PAS[sp + 1] = PAS[sp + 1] == PAS[sp]; \
        sp += 1;                              \
    } while (0)
#define __EQL 5

/*
```rs
macro_rules! NEQ {
    ($sp:expr) => {
        PAS[$sp + 1] = PAS[$sp + 1] != PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Inequality comparison
#define NEQ(sp)                               \
    do {                                      \
        PAS[sp + 1] = PAS[sp + 1] != PAS[sp]; \
        sp += 1;                              \
    } while (0)
#define __NEQ 6

/*
```rs
macro_rules! LSS {
    ($sp:expr) => {
        PAS[$sp + 1] = PAS[$sp + 1] < PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Less-than comparison
#define LSS(sp)                              \
    do {                                     \
        PAS[sp + 1] = PAS[sp + 1] < PAS[sp]; \
        sp += 1;                             \
    } while (0)
#define __LSS 7

/*
```rs
macro_rules! LEQ {
    ($sp:expr) => {
        PAS[$sp + 1] = PAS[$sp + 1] <= PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Less-or-equal comparison
#define LEQ(sp)                               \
    do {                                      \
        PAS[sp + 1] = PAS[sp + 1] <= PAS[sp]; \
        sp += 1;                              \
    } while (0)
#define __LEQ 8

/*
```rs
macro_rules! GTR {
    ($sp:expr) => {
        PAS[$sp + 1] = PAS[$sp + 1] > PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Greater-than comparison
#define GTR(sp)                              \
    do {                                     \
        PAS[sp + 1] = PAS[sp + 1] > PAS[sp]; \
        sp += 1;                             \
    } while (0)
#define __GTR 9

/*
```rs
macro_rules! GEQ {
    ($sp:expr) => {
        PAS[$sp + 1] = PAS[$sp + 1] >= PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Greater-or-equal comparison
#define GEQ(sp)                               \
    do {                                      \
        PAS[sp + 1] = PAS[sp + 1] >= PAS[sp]; \
        sp += 1;                              \
    } while (0)
#define __GEQ 10

/*
```rs
macro_rules! LOD {
    ($sp:expr, $bp:expr, $level:expr, $offset:expr) => {
        $sp -= 1;
        PAS[$sp] = PAS[base($bp, $level) - $offset];
    }
}
```
*/
// Load value to top of stack from offset a in the AR n
// static levels down.
#define LOD(sp, bp, level, offset)               \
    do {                                         \
        sp--;                                    \
        PAS[sp] = PAS[base(bp, level) - offset]; \
    } while (0)
#define __LOD 3

/*
```rs
macro_rules! STO {
    ($sp:expr, $bp:expr, $level:expr, $offset:expr) => {
        PAS[base($bp, $level) - $offset] = PAS[$sp];
        $sp += 1;
    }
}
```
*/
// Store top of stack into offset o in the AR n static levels down
#define STO(sp, bp, level, offset)               \
    do {                                         \
        PAS[base(bp, level) - offset] = PAS[sp]; \
        sp += 1;                                 \
    } while (0)
#define __STO 4

/*
```rs
macro_rules! CAL {
    ($sp:expr, $bp:expr, $address:expr, $offset:expr) => {
        PAS[$sp - 1] = base($bp, $offset);
        PAS[$sp - 2] = $bp;
        PAS[$sp - 3] = PC;
        $bp = $sp - 1;
        PC = $address;
    }
}
```
*/
// Call procedure at code address a; create activation record
#define CAL(sp, bp, address, offset)    \
    do {                                \
        PAS[sp - 1] = base(bp, offset); \
        PAS[sp - 2] = bp;               \
        PAS[sp - 3] = PC;               \
        bp = sp - 1;                    \
        PC = address;                   \
    } while (0)
#define __CAL 5

/*
```rs
macro_rules! INC {
    ($sp:expr, $offset:expr) => {
        $sp -= $offset;
    }
}
```
*/
// Allocate n locals on the stack
#define INC(sp, n) sp -= n
#define __INC 6

/*
```rs
macro_rules! JMP {
    ($address:expr) => {
        PC = $address;
    }
}
```
*/
// Unconditional jump to address a
#define JMP(address) PC = address
#define __JMP 7

/*
```rs
macro_rules! JPC {
    ($sp:expr, $address:expr) => {
        if (PAS[$sp] == 0) {
            PC = $address;
        }
        $sp += 1;
    }
}
```
*/
// Conditional jump: if value at top of stack is 0, jump to a; pop the stack.
#define JPC(sp, address)    \
    do {                    \
        if (PAS[sp] == 0) { \
            PC = address;   \
        }                   \
        sp += 1;            \
    } while (0)
#define __JPC 8

/*
```rs
macro_rules! SYS {
    (1 $sp:expr) => {
        printf("%d\n", PAS[$sp]);
        $sp += 1;
    };
    (2 $sp:expr) => {
        scanf("%d", &PAS[$sp]);
        $sp += 1;
    };
    (3) => {
        exit(0);
    };
}
```
*/
// 1. Output integer value at top of stack; then pop.
// 2. Read an integer from stdin and push it
// 3. Halt the program
#define SYS_1(sp)                  \
    do {                           \
        printf("%d\n", PAS[(sp)]); \
        (sp) += 1;                 \
    } while (0)
#define SYS_2(sp)                             \
    do {                                      \
        if (scanf("%d", &PAS[(sp)]) != 1) {   \
            fprintf(stderr, "Invalid input"); \
            exit(1);                          \
        }                                     \
        (sp) += 1;                            \
    } while (0)
#define SYS_3()  \
    do {         \
        exit(0); \
    } while (0)
#define __SYS 9

/*
```rs
macro_rules! OP_NAME {
    ($opcode:expr, $arg:expr) => {
        match $opcode {
            1 => "LIT",
            2 => {
                match $arg {
                    0 => "RTN",
                    1 => "ADD",
                    2 => "SUB",
                    3 => "MUL",
                    4 => "DIV",
                    5 => "EQL",
                    6 => "NEG",
                    7 => "LSS",
                    8 => "LEQ",
                    9 => "GTR",
                    10 => "GEQ",
                    _ => unreachable!(),
                }
            },
            3 => "LOD",
            4 => "STO",
            5 => "CAL",
            6 => "INC",
            7 => "JMP",
            8 => "JPC",
            9 => "SYS",
            _ => unreachable!(),
        }
    }
}
```
*/
// Takes the number of the operation and translates to an operation name
#define OP_NAME(opcode, arg)             \
    do {                                 \
        switch ((opcode)) {              \
        case 1:                          \
            printf("LIT");               \
            break;                       \
        case 2:                          \
            switch ((arg)) {             \
            case 0:                      \
                printf("RTN");           \
                break;                   \
            case 1:                      \
                printf("ADD");           \
                break;                   \
            case 2:                      \
                printf("SUB");           \
                break;                   \
            case 3:                      \
                printf("MUL");           \
                break;                   \
            case 4:                      \
                printf("DIV");           \
                break;                   \
            case 5:                      \
                printf("EQL");           \
                break;                   \
            case 6:                      \
                printf("NEG");           \
                break;                   \
            case 7:                      \
                printf("LSS");           \
                break;                   \
            case 8:                      \
                printf("LEQ");           \
                break;                   \
            case 9:                      \
                printf("GTR");           \
                break;                   \
            case 10:                     \
                printf("GEQ");           \
                break;                   \
            default:                     \
                printf("ALU?%d", (arg)); \
                break;                   \
            }                            \
            break;                       \
        case 3:                          \
            printf("LOD");               \
            break;                       \
        case 4:                          \
            printf("STO");               \
            break;                       \
        case 5:                          \
            printf("CAL");               \
            break;                       \
        case 6:                          \
            printf("INC");               \
            break;                       \
        case 7:                          \
            printf("JMP");               \
            break;                       \
        case 8:                          \
            printf("JPC");               \
            break;                       \
        case 9:                          \
            printf("SYS");               \
            break;                       \
        default:                         \
            printf("?%d", (opcode));     \
            break;                       \
        }                                \
    } while (0)

int base(int BP, int L) {
    // Activation record base
    int activation_record_base = BP;
    while (L > 0) {
        // Follow static link
        activation_record_base = PAS[activation_record_base];
        L--;
    }
    return activation_record_base;
}

void print_stack_trace(int BP) {
    int i = LAST_INSTRUCTION;
    printf("Stack Trace: [");
    while (i > BP) {
        printf("%d, ", PAS[i]);
        i--;
    }
    printf("]\n");
}

typedef struct InstRegister {
    // The operation code specifying the instruction to execute
    // (LIT, OPR, LOD, STO, CAL, INC, JMP, JPC, SYS).
    int OP;

    // The lexicographical level for instructions that access variables in other activation records.
    int L;

    // A parameter whose meaning depends on the opcode. It may be a literal value, an
    // address in the text segment, an offset within an activation record or a sub-opcode for
    // arithmetic and logical operations
    int M;
} InstRegister;

int main() {
    // Last M word (Lowest address used by code)
    // Points to the top of the stack. The stack grows downward (decrementing SP)
    // when values are pushed and upward when values are popped
    int SP = LAST_INSTRUCTION;

    // Points to the base of the current activation record on the stack
    int BP = LAST_INSTRUCTION - 1;

    // Holds the OP, L, M fields of the instruction currently being executed
    InstRegister IR = {0, 0, 0};

    // LOOP the following code
    /*
    IR.OP = PAS[PC]
    IR.L = PAS[PC - 1]
    IR.M = PAS[PC - 2]
    PC -= 3
    */

    // Read input.txt file
    /*
    ```rs
    let input = std::fs::read_to_string("input.txt");
    let raw_instructions = input.split("\n").collect::<Vec<_>>();
    struct InstRegister {
        OP: u8,
        L: u8,
        M: u8,
    }
    let instructions: Vec<InstRegister> = raw_instructions
        .iter()
        .map(|line| {
            let operations = line.split(" ").collect::<Vec<_>>();
            InstRegister {
                OP: operations[0].parse().unwrap(),
                L:  operations[1].parse().unwrap(),
                M:  operations[2].parse().unwrap(),
            }
        })
        .collect();
    ```
    */
    return 0;
}