#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define STACK_SIZE 500
#define PC_BASE STACK_SIZE - 1

// Process Address Space
// .text = Addr 499..0; 3 words per instruction
// .data = below .text section
static int PAS[STACK_SIZE] = {0};

// Initialize PC to 499 (Page 3)
// Points to the next instruction in the text segment.
static int PC = PC_BASE;

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
    int SP = 0;

    // Points to the base of the current activation record on the stack
    int BP = 0;

    // Holds the OP, L, M fields of the instruction currently being executed
    InstRegister IR = {0, 0, 0};

    // LOOP the following code
    /*
    IR.OP = PAS[PC]
    IR.L = PAS[PC - 1]
    IR.M = PAS[PC - 2]
    PC -= 3
    */

    FILE *fileptr = fopen("input.txt", "r");
    if (!fileptr) {
        return 1;
    }

    InstRegister instructions[100];

    char buffer[500];
    // Read the file line by line

    int len_instructions = 0;
    while (fgets(buffer, sizeof(buffer), fileptr) != NULL) {
        char *token;
        token = strtok(buffer, " ");
        int j = 0;
        while (token != NULL) {
            if (j == 0) {
                instructions[len_instructions].OP = atoi(token);
            } else if (j == 1) {
                instructions[len_instructions].L = atoi(token);
            } else if (j == 2) {
                instructions[len_instructions].M = atoi(token);
            }
            token = strtok(NULL, " ");
            j++;
        }
        len_instructions++;
    }

    // Close the file
    fclose(fileptr);

    SP = STACK_SIZE - len_instructions * 3;
    BP = SP - 1;

    printf("%8s%8s%5s%5s%5s%5s%s\n", "", "L", "M", "PC", "BP", "SP", "stack");
    printf("%21s%5d%5d%5d\n", "Initial values:", PC, BP, SP);

    for (int i = 0; i < len_instructions; i++) {
        InstRegister instruction = instructions[i];
        PAS[PC] = instruction.OP;
        PAS[PC - 1] = instruction.L;
        PAS[PC - 2] = instruction.M;
        PC -= 3;
    }

    PC = PC_BASE;

    while ((PC - 2) < PC_BASE && (PC - 2) > 0 && PAS[PC - 2] != 0) {
        // Load everything to the IR variable
        IR.OP = PAS[PC];
        IR.L = PAS[PC - 1];
        IR.M = PAS[PC - 2];
        // Growing downwards
        PC -= 3;

        // IR.OP is always the first digit of a line in that input.txt file
        // Just map each one to an instruction. In C we're going to use lots of if's
        // or a switch statement
        // Everything below is from the homework 1 assignment details
        // We should inline every macro since they're not used anywhere else
        switch (IR.OP) {
        // Literal push
        case 1: {
            SP -= 1;
            PAS[SP] = IR.M;
            break;
        }
        case 2: {
            switch (IR.M) {
                // Return from subroutine and restore caller's AR
            case 0: {
                SP = BP + 1;
                BP = PAS[SP - 2];
                PC = PAS[SP - 3];
                break;
            }
            // Addition
            case 1: {
                PAS[SP + 1] += PAS[SP];
                SP += 1;
                break;
            }
            // Subtraction
            case 2: {
                PAS[SP + 1] -= PAS[SP];
                SP += 1;
                break;
            }
            // Multiplication
            case 3: {
                PAS[SP + 1] *= PAS[SP];
                SP += 1;
                break;
            }
            // Division
            case 4: {
                PAS[SP + 1] /= PAS[SP];
                SP += 1;
                break;
            }
            // Equality comparison
            case 5: {
                PAS[SP + 1] = PAS[SP + 1] == PAS[SP];
                SP += 1;
                break;
            }
            // Inequality comparison
            case 6: {
                PAS[SP + 1] = PAS[SP + 1] != PAS[SP];
                SP += 1;
                break;
            }
            // Less-than comparison
            case 7: {
                PAS[SP + 1] = PAS[SP + 1] < PAS[SP];
                SP += 1;
                break;
            }
            // Less-or-equal comparison
            case 8: {
                PAS[SP + 1] = PAS[SP + 1] <= PAS[SP];
                SP += 1;
                break;
            }
            // Greater-than comparison
            case 9: {
                PAS[SP + 1] = PAS[SP + 1] > PAS[SP];
                SP += 1;
                break;
            }
            // Greater-or-equal comparison
            case 10: {
                PAS[SP + 1] = PAS[SP + 1] >= PAS[SP];
                SP += 1;
                break;
            }
            }
            break;
        }
        // Load value to top of stack from offset a in the AR n
        // static levels down.
        case 3: {
            SP -= 1;
            PAS[SP] = PAS[base(BP, IR.L) - IR.M];
            break;
        }
        // Store top of stack into offset o in the AR n static levels down
        case 4: {
            PAS[base(BP, IR.L) - IR.M] = PAS[SP];
            SP += 1;
            break;
        }
        // Call procedure at code address a; create activation record
        case 5: {
            PAS[SP - 1] = base(BP, IR.L);
            PAS[SP - 2] = BP;
            PAS[SP - 3] = PC;
            BP = SP - 1;
            PC = PC_BASE - IR.M;
            break;
        }
        // Allocate n locals on the stack
        case 6: {
            SP -= IR.M;
            break;
        }
        // Unconditional jump to address a
        case 7: {
            PC = PC_BASE - IR.M;
            break;
        }
        // Conditional jump: if value at top of stack is 0, jump to a; pop the stack.
        case 8: {
            if (PAS[SP] == 0) {
                PC = PC_BASE - IR.M;
            }
            SP += 1;
            break;
        }
            // Compound instruction as well. M determines what action of SYS to take
        case 9:
            switch (IR.M) {
            // 1. Output integer value at top of stack; then pop.
            case 1: {
                SP += 1;
                break;
            }
            case 2: {
                int value;
                printf("%s", "Please Enter an Integer: ");
                scanf("%d", &value);
                PAS[SP - 1] = value;
                SP -= 1;
                break;
            }
            case 3: {
                exit(0);
                break;
            }
            }
        }

        char *op_name[3];
        switch (IR.OP) {
        case 1:
            *op_name = "LIT";
        case 2:
            switch (IR.M) {
            case 0:
                *op_name = "RTN";
            case 1:
                *op_name = "ADD";
            case 2:
                *op_name = "SUB";
            case 3:
                *op_name = "MUL";
            case 4:
                *op_name = "DIV";
            case 5:
                *op_name = "EQL";
            case 6:
                *op_name = "NEG";
            case 7:
                *op_name = "LSS";
            case 8:
                *op_name = "LEQ";
            case 9:
                *op_name = "GTR";
            case 10:
                *op_name = "GEQ";
            }
        case 3:
            *op_name = "LOD";
        case 4:
            *op_name = "STO";
        case 5:
            *op_name = "CAL";
        case 6:
            *op_name = "INC";
        case 7:
            *op_name = "JMP";
        case 8:
            *op_name = "JPC";
        case 9:
            *op_name = "SYS";
        }

        printf("%8s%8d%5d%5d%5d%5d", *op_name, IR.L, IR.M, PC, BP, SP);

        for (int i = BP - SP + 1; i > 0; i--) {
            printf("%2d", PAS[i]);
        }
        puts("");
    }
    return 0;
}