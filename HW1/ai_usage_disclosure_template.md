# AI Usage Disclosure Details

**Student Name:** Luiz Gustavo Santana Dias Gomes


**Student ID:** 5678035


**Assignment:** Homework 1

---

## Instructions

Complete this template with detailed information about your AI usage. Submit this file along with your signed PDF declaration form.

---

## AI Tool #1

### Tool Name
ChatGPT

### Version/Model
GPT-5

### Date(s) Used
September 6, 2025, and September 9, 2025

### Specific Parts of Assignment

- The first version of this assignment was written in Rust, then translated into C. Only two prompts were used, one related to how to get user input using Rust stdlib, and the other one regarding how to add a left padding, and align text to the left when printing to the console

- Supplementary source: `https://www.geeksforgeeks.org/c/c-program-to-read-contents-of-whole-file`

### Prompts Used
1. Corrija: `println!("{:<width$}L{:<width$} M{:<width$} PC{:<width$}BP{:<width$}SP{:<width$}stack", width = 10);`

2. `unsafe extern "C" fn scanf(fmt: *const i8) {}` Como pegar input do console em Rust?

3. (9/10/2025, second submission) Transforme o trecho a seguir em um código em C. É necessário verificar se o tamanho do array não será estourado;
Em C o tamanho do array `stack_frames` está na variável `len_frames`, e o array é diferente pois tuplas não são suportadas; Além disso, PAS é uma variável global com o mesmo layout em ambas as linguagens. Segue a implementação parcial em C e o que deve ser traduzido de Rust para C:

```c
typedef struct StackFrame {
    int sp;
    int bp;
} StackFrame;

StackFrame stack_frames[500] = {0};
```
Transforme em C:
```rs
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
```

### AI Output/Results

- Prompt #1 answer:
    ```rs
    println!("{:<10}{:<10}{:<10}{:<10}{:<10}{:<10}", "L", "M", "PC", "BP", "SP", "stack");
    ```

    - This translates to:
    ```c
    printf("%-10s ...", ...);
    ```

    - Also not relevant for the final submission

- Prompt #2 best answer: 
    ```rs
    use std::io::{self, Write};

    fn main() -> io::Result<()> {
        print!("Digite um número: ");
        io::stdout().flush()?; // garante que o prompt aparece

        let mut line = String::new();
        io::stdin().read_line(&mut line)?;
        let n: i32 = line.trim().parse().expect("número inválido");

        println!("Você digitou {n}");
        Ok(())
    }
    ```
    - This is an alternative form of using C's `scanf` function, without the usage of Foreign Function Interface (FFI). This had no impact on the final submission (C file).

- Prompt #3 (9/10/2025, second submission):
    ```c
    typedef struct StackFrame {
        int sp;
        int bp;
    } StackFrame;

    StackFrame stack_frames[500] = {0};

    for (int i = len_frames - 1; i >= 0; --i) {
        int sp = frames[i].sp, bp = frames[i].bp;
        if (sp <= bp) {
            for (int j = bp; j >= sp; --j) printf("%-2d ", PAS[j]);

            int has_next = 0;
            for (int k = i - 1; k >= 0; --k) {
                if (frames[k].sp <= frames[k].bp) { has_next = 1; break; }
            }
            if (has_next) printf("| ");
        }
    }
    ```
    - I changed little to nothing this output. This translation was very straightforward, except for the `.any()` method from Rust

### How Output was Verified/Edited

- Rust output was verified. Raw file content will be submitted.

### Multiple Iterations (if applicable)
- Not applicable

### Learning & Reflection

- I learned how to get user input using Rust (I hadn't done it before), and how to properly print values to the console, aligning to the left/right, and adding padding to it.

---

## AI Tool #2 (if applicable)
- Not applicable

---

## Overall Reflection

- It helped me in basic tasks, in which I knew how to do in C, but not yet in Rust.
---

## Notes

- Be as specific and detailed as possible in your responses
- Include exact prompts and AI outputs when possible
- Explain how you verified and modified AI-generated content
- Reflect on what you learned through the AI interaction

## Original source

- I created everything in Rust, then translated to C. This is attached as a comment in the bottom of the C file, where these prompt's usage can be seen implemented 