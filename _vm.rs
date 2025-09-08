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
fn base(BP: usize, L: &mut usize) -> usize {
    unsafe {
        // Activation record base
        let mut activation_record_base = BP;
        while *L > 0 {
            // Follow static link
            activation_record_base = PAS[activation_record_base];
            *L -= 1;
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

        let last_instruction = STACK_SIZE - instructions.len() * 3;

        SP = last_instruction;
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

        let mut prev_SP_BP = None::<std::ops::RangeInclusive<usize>>;
        let mut may_exit = false;

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
                            prev_SP_BP = None;
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
                    PAS[SP] = PAS[base(BP, &mut IR.L) - IR.M];
                }
                // Store top of stack into offset o in the AR n static levels down
                4 => {
                    PAS[base(BP, &mut IR.L) - IR.M] = PAS[SP];
                    SP += 1;
                }
                // Call procedure at code address a; create activation record
                5 => {
                    prev_SP_BP = Some(SP..=BP);
                    PAS[SP - 1] = base(BP, &mut IR.L);
                    PAS[SP - 2] = BP;
                    PAS[SP - 3] = PC;
                    BP = SP - 1;
                    PC = PC_BASE - IR.M;
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
            // Remember that the stack grows downwards, so their order
            // of elements is also inverted

            if let Some(range_value) = prev_SP_BP.clone() {
                for i in range_value.rev() {
                    print!("{:<2} ", PAS[i]);
                }
                if SP - 1 != BP {
                    print!("| ");
                }
            }
            for i in (SP..=BP).rev() {
                print!("{:<2} ", PAS[i]);
            }
            println!();
            if may_exit {
                std::process::exit(0);
            }
        }
    }
}
