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
September 6, 2025

### Specific Parts of Assignment

- The first version of this assignment was written in Rust, then translated into C. Only two prompts were used, one related to how to get user input using Rust stdlib, and the other one regarding how to add a left padding, and align text to the left when printing to the console

- Supplementary source: `https://www.geeksforgeeks.org/c/c-program-to-read-contents-of-whole-file`

### Prompts Used
1. Corrija: `println!("{:<width$}L{:<width$} M{:<width$} PC{:<width$}BP{:<width$}SP{:<width$}stack", width = 10);`

2. `unsafe extern "C" fn scanf(fmt: *const i8) {}` Como pegar input do console em Rust?

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
