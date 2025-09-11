#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #![author = Luiz Gustavo Santana Dias Gomes]
// Source code was written in Rust and translated.
// It is commented in the bottow of the document.

/*
Assignment:
    vm.c - Implement a P-machine virtual machine

Authors:
    Luiz Gustavo Santana Dias Gomes

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

/// Stack size was determined to be 500 in the assignment details
#define STACK_SIZE 500

/// PC of a downward moving stack is the length of Stack - 1
/// to match the last index in it
#define PC_BASE (STACK_SIZE - 1)

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

// argv[1] will be the path to input file
int main(int argc, char *argv[]) {
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

    // Open input file
    FILE *file_ptr = fopen(argv[1], "r");
    if (!file_ptr) {
        puts("Could not read input file. Maybe path is wrong?");
        return 1;
    }

    // Maximum of 500 instructions for that file
    InstRegister instructions[500];

    // Maximum of 500 characters for each line split
    char buffer[500];

    // Keep track of how many instructions there are in the file
    int len_instructions = 0;

    // Read the file line by line
    while (fgets(buffer, sizeof(buffer), file_ptr) != NULL) {
        char *token;
        // .split(" ") -> Get tokens separated by whitespace
        token = strtok(buffer, " ");

        // Track how many numbers were fetched so far
        int j = 0;
        while (token != NULL) {
            // atoi -> Convert string to integer
            // J == 0 -> OP
            if (j == 0) {
                instructions[len_instructions].OP = atoi(token);
            }
            // J == 1 -> L
            else if (j == 1) {
                instructions[len_instructions].L = atoi(token);
            }
            // J == 2 -> M
            // If j is more than that, there must be an error in input file
            else if (j == 2) {
                instructions[len_instructions].M = atoi(token);
            }
            // Get next token, also divided by whitespace
            token = strtok(NULL, " ");
            // increment j
            j++;
        }
        // Increment instructions
        len_instructions++;
    }

    // Close the file
    fclose(file_ptr);

    // Formula to get the last instruction
    // 3 words, X instructions; Growing downwards
    SP = STACK_SIZE - len_instructions * 3;

    // Assignment specifications
    BP = SP - 1;

    // Print table headers
    printf("%-8s%-8s%-5s%-5s%-5s%-5s%-s\n", "", "L", "M", "PC", "BP", "SP", "stack");
    printf("%-21s%-5d%-5d%-5d\n", "Initial values:", PC, BP, SP);

    // Load each instruction to the PAS variable
    // PC -= 3 here has nothing to do with the start of the program. I just
    // used it because it was initialized with the same value as the PAS length
    for (int i = 0; i < len_instructions; i++) {
        InstRegister instruction = instructions[i];
        PAS[PC] = instruction.OP;
        PAS[PC - 1] = instruction.L;
        PAS[PC - 2] = instruction.M;
        PC -= 3;
    }

    // Get PC back to its original value before starting the true execution of the program
    PC = PC_BASE;

    // Track when to exit the program. This was done because calling exit(0) directly
    // won't print its instruction details in the console
    // (Because it is done after the instruction has been executed)
    int may_exit = 0;

    // Know how many AR's other than the first one should be printed
    int AR = 0;

    // Safety check that PAS[PC - 2] is defined does not work in C
    // `while let Some(_) = PAS.get(PC - 2)`
    // PAS[INVALID_INDEX] returns a garbage value, not necessarily 0
    // Infinite loop, SYS (#3) must be called at some point
    while (1) {
        // Load everything to the IR variable
        IR.OP = PAS[PC];
        IR.L = PAS[PC - 1];
        IR.M = PAS[PC - 2];
        // Growing downwards
        PC -= 3;

        // Name of the operation that was just executed
        // Initialize empty to avoid Warnings
        char op_name[4] = {0};

        // IR.OP is always the first digit of a line in that input.txt file
        // Everything below is from the homework 1 assignment details
        switch (IR.OP) {
        // Literal push
        case 1: {
            strcpy(op_name, "LIT");
            SP -= 1;
            PAS[SP] = IR.M;
            break;
        }
        case 2: {
            switch (IR.M) {
                // Return from subroutine and restore caller's AR
            case 0: {
                strcpy(op_name, "RTN");
                SP = BP + 1;
                BP = PAS[SP - 2];
                PC = PAS[SP - 3];
                // Remove an AR because a function returned a value
                // So its stack should be "deleted"
                AR -= 1;
                break;
            }
            // Addition
            case 1: {
                strcpy(op_name, "ADD");
                PAS[SP + 1] += PAS[SP];
                SP += 1;
                break;
            }
            // Subtraction
            case 2: {
                strcpy(op_name, "SUB");
                PAS[SP + 1] -= PAS[SP];
                SP += 1;
                break;
            }
            // Multiplication
            case 3: {
                strcpy(op_name, "MUL");
                PAS[SP + 1] *= PAS[SP];
                SP += 1;
                break;
            }
            // Division
            case 4: {
                strcpy(op_name, "DIV");
                PAS[SP + 1] /= PAS[SP];
                SP += 1;
                break;
            }
            // Equality comparison
            case 5: {
                strcpy(op_name, "EQL");
                PAS[SP + 1] = PAS[SP + 1] == PAS[SP];
                SP += 1;
                break;
            }
            // Inequality comparison
            case 6: {
                strcpy(op_name, "NEQ");
                PAS[SP + 1] = PAS[SP + 1] != PAS[SP];
                SP += 1;
                break;
            }
            // Less-than comparison
            case 7: {
                strcpy(op_name, "LSS");
                PAS[SP + 1] = PAS[SP + 1] < PAS[SP];
                SP += 1;
                break;
            }
            // Less-or-equal comparison
            case 8: {
                strcpy(op_name, "LEQ");
                PAS[SP + 1] = PAS[SP + 1] <= PAS[SP];
                SP += 1;
                break;
            }
            // Greater-than comparison
            case 9: {
                strcpy(op_name, "GTR");
                PAS[SP + 1] = PAS[SP + 1] > PAS[SP];
                SP += 1;
                break;
            }
            // Greater-or-equal comparison
            case 10: {
                strcpy(op_name, "GEQ");
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
            strcpy(op_name, "LOD");
            SP -= 1;
            PAS[SP] = PAS[base(BP, IR.L) - IR.M];
            break;
        }
        // Store top of stack into offset o in the AR n static levels down
        case 4: {
            strcpy(op_name, "STO");
            PAS[base(BP, IR.L) - IR.M] = PAS[SP];
            SP += 1;
            break;
        }
        // Call procedure at code address a; create activation record
        case 5: {
            strcpy(op_name, "CAL");
            PAS[SP - 1] = base(BP, IR.L);
            PAS[SP - 2] = BP;
            PAS[SP - 3] = PC;
            BP = SP - 1;
            PC = PC_BASE - IR.M;
            // Calling a new function reserves space on the stack for it
            // So AR + 1 is to print this one to the console
            AR += 1;
            break;
        }
        // Allocate n locals on the stack
        case 6: {
            strcpy(op_name, "INC");
            SP -= IR.M;
            break;
        }
        // Unconditional jump to address a
        case 7: {
            strcpy(op_name, "JMP");
            PC = PC_BASE - IR.M;
            break;
        }
        // Conditional jump: if value at top of stack is 0, jump to a; pop the stack.
        case 8: {
            strcpy(op_name, "JPC");
            if (PAS[SP] == 0) {
                PC = PC_BASE - IR.M;
            }
            SP += 1;
            break;
        }
        // Compound instruction as well. M determines what action of SYS to take
        case 9: {
            strcpy(op_name, "SYS");
            switch (IR.M) {
            // 1. Output integer value at top of stack; then pop.
            case 1: {
                printf("%s%d\n", "Output result is: ", PAS[SP]);
                SP += 1;
                break;
            }
            // 2. Read an integer from stdin and push it
            case 2: {
                int value;
                printf("%s", "Please Enter an Integer: ");
                int _ignore = scanf("%d", &value);
                if (_ignore) {
                }
                PAS[SP - 1] = value;
                SP -= 1;
                break;
            }
            // 3. Halt the program
            case 3: {
                // Print details of this instruction before calling exit
                may_exit = 1;
                break;
            }
            }
        }
        }

        // Print the "Table" of each content
        printf("%-8s%-8d%-5d%-5d%-5d%-5d", op_name, IR.L, IR.M, PC, BP, SP);

        // Number of stacks to print is AR + 1
        // When AR = 0, we're printing only current stack for example
        int stacks_to_print = AR + 1;

        // In Rust I used usize tuples, but in C there's no such thing
        // so this type is to avoid adding 2 times the normal length of
        // stack frames and map each element like [SP_0, BP_0, SP_1, BP_1, ...]
        // I tried but it was being much harder
        typedef struct StackFrame {
            int sp;
            int bp;
        } StackFrame;

        // Hold stack frames until reach the main caller
        // SP and BP's will be stored here, then printed in reverse order to the console
        StackFrame stack_frames[500] = {0};

        // Temporary variable to hold current BP and SP
        // They will be inserted in the stack_frames array to record position of a stack
        int TBP = BP;
        int TSP = SP;

        int len_frames = 0;
        while (stacks_to_print > 0) {
            // Must be valid indexes
            stack_frames[len_frames].sp = TSP;
            stack_frames[len_frames].bp = TBP;
            // <Vec>.push() immediately increments index. If this is done at the end,
            // one "break" can be called and length will be incorrect
            len_frames++;

            // Check if this BP is plausible. It will be used to index in PAS, so if it is
            // zero, it is invalid, if it is less than zero, it can't be used to index an array
            // (That's why Rust only allow unsigned int to index an array)
            if (TBP < 1) {
                break;
            }

            // Get previous BP
            // Dynamic link (Caller's BP) is in PAS[BP - 1]
            // This can be inferred from RTN provided in the assignment
            // ---------------------
            // SP = BP + 1;
            // BP = PAS[SP - 2];
            // ---------------------
            // If SP is BP + 1, and the caller's BP is in PAS[SP - 2],
            // then, the caller's BP is in PAS[BP - 1] in fact
            int PBP = PAS[TBP - 1];
            if (PBP == 0) {
                // If it is zero, then there's no longer a dynamic link
                break;
            }

            // From the same RTN specifications provided in the assignment, we can infer
            // the previous SP (was BP + 1)
            int PSP = TBP + 1;

            // Update temporary BP and SP so we know what to print
            TBP = PBP;
            TSP = PSP;

            // Not really to print, here it is being used as a variable to add to the helper
            // array "stack_frames" the SP and BP's to be printed afterwards
            stacks_to_print -= 1;
        }

        // The assignment want to print the current stack the last, and
        // the main's stack frame the first, so order has to be reversed
        for (int i = len_frames - 1; i >= 0; --i) {
            // Get the SP and BP of each stack frame from the helper array
            int stack_sp = stack_frames[i].sp;
            int stack_bp = stack_frames[i].bp;

            // If SP < BP the print function will fail (Invalid range)
            if (stack_sp > stack_bp) {
                continue;
            }

            // Stack grow downwards, so the order of iteration is reversed, inclusive
            for (int j = stack_bp; j >= stack_sp; --j) {
                printf("%-2d ", PAS[j]);
            }

            // Will determine if the next iteration will print something
            // If it does, then add the bars to separate each stack frame
            // Otherwise, do nothing since this is the last one
            int may_add_bars = 0;

            // i is decreasing, so i - 1 represent the next iteration
            // check if the next iteration will in print something in the console
            // if it would, then may_add_bars has to be set true
            for (int k = i - 1; k >= 0; --k) {
                // It wouldn't print anything if SP > BP
                if (stack_frames[k].sp > stack_frames[k].bp) {
                    continue;
                }
                // If it got in here, then next iteration does exist
                may_add_bars = 1;
                break;
            }

            // Next iteration will print something, so add the bars to separate
            // the stack frames
            if (may_add_bars) {
                printf("%s", "| ");
            }
        }

        // Go to new line
        puts("");

        // Exit the program when SYS 3 is called
        if (may_exit) {
            exit(0);
        }
    }
    return 0;
}

/*
Source file (100% authoral version)
*/

/*
#![allow(non_snake_case, static_mut_refs, dead_code, unused_assignments)]

// Because of this, that's not really a "no-std" program
// But in C we would have to import stdlib.h the same way
use std::io::{self, Write};

/// Stack size was determined to be 500 in the assignment details
const STACK_SIZE: usize = 500;

/// PC of a downward moving stack is the length of Stack - 1
/// to match the last index in it
const PC_BASE: usize = STACK_SIZE - 1;

/// Process Address Space
/// .text = Addr 499..0; 3 words per instruction
/// .data = below .text section
static mut PAS: [usize; STACK_SIZE] = [0; STACK_SIZE];

/// Initialize PC to 499 (Page 3)
/// Points to the next instruction in the text segment.
static mut PC: usize = PC_BASE;

/// Translated funciton provided in the assignment details
unsafe fn base(BP: usize, mut L: usize) -> usize {
    unsafe {
        // Activation record base
        let mut activation_record_base = BP;
        while L > 0 {
            // Follow static link
            activation_record_base = PAS[activation_record_base];
            L -= 1;
        }
        return activation_record_base;
    }
}

struct InstRegister {
    /// The operation code specifying the instruction to execute
    /// (LIT, OPR, LOD, STO, CAL, INC, JMP, JPC, SYS).
    OP: usize,

    /// The lexicographical level for instructions that access variables in other activation records.
    L: usize,

    /// A parameter whose meaning depends on the opcode. It may be a literal value, an
    /// address in the text segment, an offset within an activation record or a sub-opcode for
    /// arithmetic and logical operations
    M: usize,
}

fn main() {
    // Static muts are not allowed, so an unsafe block is necessary
    unsafe {
        // Last M word (Lowest address used by code)
        // Points to the top of the stack. The stack grows downward (decrementing SP)
        // when values are pushed and upward when values are popped
        let mut SP = 0;

        // Points to the base of the current activation record on the stack
        let mut BP = 0;

        // Holds the OP, L, M fields of the instruction currently being executed
        // Start IR zeroed to avoid using too complext types like MaybeUninit<T> or raw pointers
        let mut IR = InstRegister { OP: 0, L: 0, M: 0 };

        // LOOP the following code (Provided by the assignment)
        // IR.OP = PAS[PC]
        // IR.L = PAS[PC - 1]
        // IR.M = PAS[PC - 2]
        // PC -= 3

        // Read input.txt file
        let input = std::fs::read_to_string("test_procedure_in.txt").unwrap();
        let raw_instructions = input.split("\n").collect::<Vec<_>>();
        let instructions = raw_instructions
            .iter()
            .filter_map(|line| {
                let operations = line.split(" ").collect::<Vec<_>>();
                (operations.len() == 3).then_some({
                    InstRegister {
                        OP: operations[0].trim().parse().unwrap(),
                        L: operations[1].trim().parse().unwrap(),
                        M: operations[2].trim().parse().unwrap(),
                    }
                })
            })
            .collect::<Vec<_>>();

        // SP will be the last instruction
        SP = STACK_SIZE - instructions.len() * 3;
        BP = SP - 1;

        // Each {:<8} adds a padding to the left of the template string
        // To align to the right, we can use {:>10}
        // First one is an empty string because there's nothing in there
        // in the output example
        println!(
            "{:<8}{:<8}{:<5}{:<5}{:<5}{:<5}{}",
            "", "L", "M", "PC", "BP", "SP", "stack",
        );
        // Initial values takes up 20 characters in the output example
        println!("{:<21}{:<5}{:<5}{:<5}", "Initial values:", PC, BP, SP);

        // Load each instruction to the PAS variable
        // PC -= 3 here has nothing to do with the start of the program. I just
        // used it because it was initialized with the same value as the PAS length
        for instruction in instructions {
            PAS[PC] = instruction.OP;
            PAS[PC - 1] = instruction.L;
            PAS[PC - 2] = instruction.M;
            PC -= 3;
        }

        // Get PC back to its original value before starting the true execution of the program
        PC = PC_BASE;

        // Track when to exit the program
        let mut may_exit = false;

        // Know how many AR's other than the first one should be printed
        let mut AR = 0usize;

        // If index PC - 2 is not defined, program will crash on assignment IR.M
        // Due to an Index out of bounds error. PC and PC - 1 should be valid indexes as well
        while let Some(_) = PAS.get(PC - 2) {
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
            match IR.OP {
                // Literal push
                1 => {
                    SP -= 1;
                    PAS[SP] = IR.M;
                }
                // Compound instruction
                2 => {
                    match IR.M {
                        // Return from subroutine and restore caller's AR
                        0 => {
                            SP = BP + 1;
                            BP = PAS[SP - 2];
                            PC = PAS[SP - 3];
                            // Remove an AR because a function returned a value
                            // So its stack should be "deleted"
                            AR -= 1;
                        }
                        // Addition
                        1 => {
                            PAS[SP + 1] += PAS[SP];
                            SP += 1;
                        }
                        // Subtraction
                        2 => {
                            PAS[SP + 1] -= PAS[SP];
                            SP += 1;
                        }
                        // Multiplication
                        3 => {
                            PAS[SP + 1] *= PAS[SP];
                            SP += 1;
                        }
                        // Division
                        4 => {
                            PAS[SP + 1] /= PAS[SP];
                            SP += 1;
                        }
                        // Equality comparison
                        5 => {
                            PAS[SP + 1] = (PAS[SP + 1] == PAS[SP]) as usize;
                            SP += 1;
                        }
                        // Inequality comparison
                        6 => {
                            PAS[SP + 1] = (PAS[SP + 1] != PAS[SP]) as usize;
                            SP += 1;
                        }
                        // Less-than comparison
                        7 => {
                            PAS[SP + 1] = (PAS[SP + 1] < PAS[SP]) as usize;
                            SP += 1;
                        }
                        // Less-or-equal comparison
                        8 => {
                            PAS[SP + 1] = (PAS[SP + 1] <= PAS[SP]) as usize;
                            SP += 1;
                        }
                        // Greater-than comparison
                        9 => {
                            PAS[SP + 1] = (PAS[SP + 1] > PAS[SP]) as usize;
                            SP += 1;
                        }
                        // Greater-or-equal comparison
                        10 => {
                            PAS[SP + 1] = (PAS[SP + 1] >= PAS[SP]) as usize;
                            SP += 1;
                        }
                        _ => unreachable!(),
                    };
                }
                // Load value to top of stack from offset a in the AR n
                // static levels down.
                3 => {
                    SP -= 1;
                    PAS[SP] = PAS[base(BP, IR.L) - IR.M];
                }
                // Store top of stack into offset o in the AR n static levels down
                4 => {
                    PAS[base(BP, IR.L) - IR.M] = PAS[SP];
                    SP += 1;
                }
                // Call procedure at code address a; create activation record
                5 => {
                    PAS[SP - 1] = base(BP, IR.L);
                    PAS[SP - 2] = BP;
                    PAS[SP - 3] = PC;
                    BP = SP - 1;
                    PC = PC_BASE - IR.M;
                    // Calling a new function reserves space on the stack for it
                    // So AR + 1 is to print this one to the console
                    AR += 1;
                }
                // Allocate n locals on the stack
                6 => {
                    SP -= IR.M;
                }
                // Unconditional jump to address a
                7 => {
                    PC = PC_BASE - IR.M;
                }
                // Conditional jump: if value at top of stack is 0, jump to a; pop the stack.
                8 => {
                    if PAS[SP] == 0 {
                        PC = PC_BASE - IR.M;
                    }
                    SP += 1;
                }
                // Compound instruction as well. M determines what action of SYS to take
                9 => match IR.M {
                    // 1. Output integer value at top of stack; then pop.
                    1 => {
                        println!("Output result is: {}", PAS[SP]);
                        SP += 1;
                    }
                    // 2. Read an integer from stdin and push it
                    2 => {
                        // The `scanf` function
                        print!("Please Enter an Integer: ");
                        io::stdout().flush().expect("Failed to flush stdout");
                        let mut line = String::new();
                        io::stdin()
                            .read_line(&mut line)
                            .expect("Failed to read line");
                        let value = line
                            .trim()
                            .parse::<usize>()
                            .expect("Invalid number entered");
                        // push the value entered onto the stack
                        PAS[SP - 1] = value;
                        SP -= 1;
                    }
                    // 3. Halt the program
                    3 => {
                        // Print details of this instruction before calling exit
                        may_exit = true;
                    }
                    _ => unreachable!(),
                },
                // If everything is working, this should be unreachable
                _ => unreachable!(),
            };
            print!(
                "{:<8}{:<8}{:<5}{:<5}{:<5}{:<5}",
                match IR.OP {
                    1 => "LIT",
                    2 => match IR.M {
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
                    },
                    3 => "LOD",
                    4 => "STO",
                    5 => "CAL",
                    6 => "INC",
                    7 => "JMP",
                    8 => "JPC",
                    9 => "SYS",
                    _ => unreachable!(),
                },
                IR.L,
                IR.M,
                PC,
                BP,
                SP,
            );

            // Hold current stack frames until reach the main caller
            let mut stack_frames: Vec<(usize, usize)> = Vec::new();

            // Temporary variable to hold current BP and SP
            let mut TBP = BP;
            let mut TSP = SP;

            // Number of stacks to print is AR + 1
            // When AR = 0, we're printing only current stack for example
            let mut stacks_to_print = AR + 1;

            while stacks_to_print > 0 {
                // Must be valid indexes
                stack_frames.push((TSP, TBP));

                // Check if dynamic link is valid
                if TBP < 1 {
                    break;
                }

                // Get previous BP
                // Dynamic link (Caller's BP) is in PAS[BP - 1]
                // This can be inferred from RTN
                // RTN:
                // SP = BP + 1;
                // BP = PAS[SP - 2];
                // If SP is BP + 1, and the caller's BP is in PAS[SP - 2],
                // then, the caller's BP is in PAS[BP - 1] in fact
                let PBP = PAS[TBP - 1];
                if PBP == 0 {
                    // If it is zero, then the root was reached (No Dynamic Link!)
                    break;
                }

                // From the same RTN specifications provided in the assignment, we can infer
                // the previous SP (was BP + 1)
                let PSP = TBP + 1;

                // Update temporary BP and SP so we know what to print
                TBP = PBP;
                TSP = PSP;

                stacks_to_print -= 1;
            }

            // The assignment want to print the current stack the last, and
            // the main's stack frame the first, so order has to be reversed
            stack_frames.reverse();

            for i in 0..stack_frames.len() {
                // Stack frames contain SP and BP in this order on them
                let (stack_sp, stack_bp) = stack_frames[i];
                // If SP < BP the print function will fail (Invalid range)
                if stack_sp > stack_bp {
                    continue;
                }
                // Stack grow downwards, so the order of iteration is reversed, inclusive
                for j in (stack_sp..=stack_bp).rev() {
                    print!("{:<2} ", PAS[j]);
                }
                // Check if the next element exist, and will print at least one element
                // I had to do this to avoid adding bars when nothing would be printed out
                let may_add_bars = stack_frames[i + 1..]
                    .iter()
                    .any(|(next_sp, next_bp)| next_sp <= next_bp);
                // Add the bars if we have more than one stack to print, so they're separated
                if may_add_bars {
                    print!("| ");
                }
            }

            println!();

            // Exit the program when SYS 3 is called
            if may_exit {
                std::process::exit(0);
            }
        }
    }
}
*/