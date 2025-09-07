#![allow(
    non_snake_case,
    static_mut_refs,
    dead_code,
    unused_macros,
    named_arguments_used_positionally,
    unused_assignments
)]
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

// Because of this, that's not really a "no-std" program
// But in C we would have to import stdlib.h the same way
use std::io::{self, Write};

/// Process Address Space
/// .text = Addr 499..0; 3 words per instruction
/// .data = below .text section
static mut PAS: [i32; 500] = [0; 500];

/// Initialize PC to 499 (Page 3)
/// Points to the next instruction in the text segment.
static mut PC: i32 = 499;

/// Literal push
macro_rules! LIT {
    ($sp:expr, $n:expr) => {{
        $sp -= 1;
        PAS[$sp as usize] = $n;
    }};
}

/// Return from subroutine and restore caller's AR
macro_rules! RTN {
    ($sp:expr, $bp:expr) => {{
        $sp = $bp + 1;
        $bp = PAS[($sp - 2) as usize];
        PC = PAS[($sp - 3) as usize];
    }};
}

/// Addition
macro_rules! ADD {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] += PAS[$sp as usize];
        $sp += 1;
    }};
}

/// Subtraction
macro_rules! SUB {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] -= PAS[$sp as usize];
        $sp += 1;
    }};
}

/// Multiplication
macro_rules! MUL {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] *= PAS[$sp as usize];
        $sp += 1;
    }};
}

/// Division
macro_rules! DIV {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] /= PAS[$sp as usize];
        $sp += 1;
    }};
}

/// Equality comparison
macro_rules! EQL {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] = (PAS[($sp + 1) as usize] == PAS[$sp as usize]) as i32;
        $sp += 1;
    }};
}

/// Inequality comparison
macro_rules! NEQ {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] = (PAS[($sp + 1) as usize] != PAS[$sp as usize]) as i32;
        $sp += 1;
    }};
}

/// Less-than comparison
macro_rules! LSS {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] = (PAS[($sp + 1) as usize] < PAS[$sp as usize]) as i32;
        $sp += 1;
    }};
}

/// Less-or-equal comparison
macro_rules! LEQ {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] = (PAS[($sp + 1) as usize] <= PAS[$sp as usize]) as i32;
        $sp += 1;
    }};
}

/// Greater-than comparison
macro_rules! GTR {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] = (PAS[($sp + 1) as usize] > PAS[$sp as usize]) as i32;
        $sp += 1;
    }};
}

/// Greater-or-equal comparison
macro_rules! GEQ {
    ($sp:expr) => {{
        PAS[($sp + 1) as usize] = (PAS[($sp + 1) as usize] >= PAS[$sp as usize]) as i32;
        $sp += 1;
    }};
}

/// Load value to top of stack from offset a in the AR n
/// static levels down.
macro_rules! LOD {
    ($sp:expr, $bp:expr, $level:expr, $offset:expr) => {{
        $sp -= 1;
        PAS[$sp as usize] = PAS[(base($bp, $level) - $offset) as usize];
    }};
}

/// Store top of stack into offset o in the AR n static levels down
macro_rules! STO {
    ($sp:expr, $bp:expr, $level:expr, $offset:expr) => {{
        PAS[(base($bp, $level) - $offset) as usize] = PAS[$sp as usize];
        $sp += 1;
    }};
}

/// Call procedure at code address a; create activation record
macro_rules! CAL {
    ($sp:expr, $bp:expr, $address:expr, $offset:expr) => {{
        PAS[($sp - 1) as usize] = base($bp, $offset);
        PAS[($sp - 2) as usize] = $bp;
        PAS[($sp - 3) as usize] = PC;
        $bp = $sp - 1;
        PC = 499 - $address;
    }};
}

/// Allocate n locals on the stack
macro_rules! INC {
    ($sp:expr, $offset:expr) => {{
        $sp -= $offset;
    }};
}

/// Unconditional jump to address a
macro_rules! JMP {
    ($address:expr) => {{
        PC = 499 - $address;
    }};
}

/// Conditional jump: if value at top of stack is 0, jump to a; pop the stack.
macro_rules! JPC {
    ($sp:expr, $address:expr) => {{
        if (PAS[$sp as usize] == 0) {
            PC = 499 - $address;
        }
        $sp += 1;
    }};
}

/// 1. Output integer value at top of stack; then pop.
/// 2. Read an integer from stdin and push it
/// 3. Halt the program
macro_rules! SYS {
    (1 $sp:expr) => {{
        // println!("(1) {}", PAS[$sp as usize]);
        $sp += 1;
    }};
    (2 $sp:expr) => {{
        print!("Please enter an integer: ");
        io::stdout().flush().expect("Failed to flush stdout");
        let mut line = String::new();
        io::stdin()
            .read_line(&mut line)
            .expect("Failed to read line");
        let n = line.trim().parse::<i32>().expect("Invalid number entered");
        // push n onto the stack
        PAS[($sp - 1) as usize] = n;
        $sp -= 1;
    }};
    (3 $closure:expr) => {{
        $closure();
        std::process::exit(0);
    }};
}

/// Takes the number of the operation and translates to an operation name
macro_rules! OP_NAME {
    ($opcode:expr, $arg:expr) => {
        match $opcode {
            1 => "LIT",
            2 => match $arg {
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
                _ => "OPR",
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
    };
}

/// Translated funciton provided in the assignment details
fn base(BP: i32, ref mut L: i32) -> i32 {
    unsafe {
        // Activation record base
        let mut activation_record_base = BP;
        while *L > 0 {
            // Follow static link
            activation_record_base = PAS[activation_record_base as usize];
            *L -= 1;
        }
        return activation_record_base;
    }
}

struct InstRegister {
    /// The operation code specifying the instruction to execute
    /// (LIT, OPR, LOD, STO, CAL, INC, JMP, JPC, SYS).
    OP: i32,

    /// The lexicographical level for instructions that access variables in other activation records.
    L: i32,

    /// A parameter whose meaning depends on the opcode. It may be a literal value, an
    /// address in the text segment, an offset within an activation record or a sub-opcode for
    /// arithmetic and logical operations
    M: i32,
}

fn main() {
    // Static muts are not allowed, so an unsafe block is necessary
    unsafe {
        // Last M word (Lowest address used by code)
        // Points to the top of the stack. The stack grows downward (decrementing SP)
        // when values are pushed and upward when values are popped
        let mut SP: i32 = 0;

        // Points to the base of the current activation record on the stack
        let mut BP: i32 = 0 - 1;

        // Holds the OP, L, M fields of the instruction currently being executed
        // Start IR zeroed to avoid using too complext types like MaybeUninit<T> or raw pointers
        let mut IR = InstRegister { OP: 0, L: 0, M: 0 };

        // LOOP the following code (Provided by the assignment)
        /*
        IR.OP = PAS[PC]
        IR.L = PAS[PC - 1]
        IR.M = PAS[PC - 2]
        PC -= 3
        */

        // Read input.txt file
        let input = std::fs::read_to_string("input.txt").unwrap();
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

        let last_instruction = 500 - instructions.len() * 3;

        SP = last_instruction as i32;
        BP = SP - 1;

        // Each {:<10} adds a padding to the left of the template string
        // To align to the right, we can use {:>10}
        // First one is an empty string because there's nothing in there
        // in the output example
        println!(
            "{:<10}{:<10}{:<10}{:<10}{:<10}{:<10}{:<10}",
            "", "L", "M", "PC", "BP", "SP", "stack",
        );
        // Initial values takes up 20 characters in the output example
        println!("{:<20}{:<10}{:<10}{:<10}", "Initial values:", PC, BP, SP);

        // Load each instruction to the PAS variable
        // PC -= 3 here has nothing to do with the start of the program. I just
        // used it because it was initialized with the same value as the PAS length
        for instruction in instructions {
            PAS[PC as usize] = instruction.OP;
            PAS[(PC - 1) as usize] = instruction.L;
            PAS[(PC - 2) as usize] = instruction.M;
            PC -= 3;
        }

        // Get PC back to its original value before starting the true execution of the program
        PC = 499;

        // If index PC - 2 is not defined, program will crash on assignment IR.M
        // Due to an Index out of bounds error. PC and PC - 1 should be valid indexes as well
        while let Some(_) = PAS.get((PC - 2) as usize) {
            // Load everything to the IR variable
            IR.OP = PAS[PC as usize];
            IR.L = PAS[(PC - 1) as usize];
            IR.M = PAS[(PC - 2) as usize];
            // Growing downwards
            PC -= 3;

            // Copy these variables to avoid macro error for using
            // static mut reference
            // If we remove the closure below, we won't have to do that
            let _BP = BP;
            let _SP = SP;

            // Whoever call this function, will print the current stats in the console
            // Closures are functions, and we're allowed to use only one. We can keep it,
            // or hardcode in the macro SYS!(3) #match arm 3
            let trace = || {
                println!(
                    "{:<10}{:<10}{:<10}{:<10}{:<10}{:<10}{:<10}",
                    format!("{}", OP_NAME!(IR.OP, IR.M)),
                    IR.L,
                    IR.M,
                    PC,
                    _BP,
                    _SP,
                    {
                        let mut parts = String::new();
                        for i in (_SP..=_BP).rev() {
                            parts.push_str(&format!("{} ", PAS[i as usize]));
                        }
                        parts
                    }
                );
            };

            // IR.OP is always the first digit of a line in that input.txt file
            // Just map each one to an instruction. In C we're going to use lots of if's
            // or a switch statement
            // Everything I got from the homework 1 assignment details
            match IR.OP {
                1 => LIT!(SP, IR.M),
                // Compound instruction
                2 => match IR.M {
                    0 => RTN!(SP, BP),
                    1 => ADD!(BP),
                    2 => SUB!(SP),
                    3 => MUL!(SP),
                    4 => DIV!(SP),
                    5 => EQL!(SP),
                    6 => NEQ!(SP),
                    7 => LSS!(SP),
                    8 => LEQ!(SP),
                    9 => GTR!(SP),
                    10 => GEQ!(SP),
                    _ => unreachable!(),
                },
                3 => LOD!(SP, BP, IR.L, IR.M),
                4 => STO!(SP, BP, IR.L, IR.M),
                5 => CAL!(SP, BP, IR.M, IR.L),
                6 => INC!(SP, IR.M),
                7 => JMP!(IR.M),
                8 => JPC!(SP, IR.M),
                // Compound instruction as well. M determines what action of SYS to take
                9 => match IR.M {
                    1 => SYS!(1 SP),
                    2 => SYS!(2 SP),
                    3 => SYS!(3 trace),
                    _ => unreachable!(),
                },
                // If everything is working, this should be unreachable
                _ => {
                    println!("Unknown instruction: {}", IR.OP);
                    break;
                }
            };
            // Statistics are printed after the instruction call according to the example
            trace()
        }
    }
}
