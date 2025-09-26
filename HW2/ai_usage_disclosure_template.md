# AI Usage Disclosure Details

**Student Name:** Luiz Gustavo Santana Dias Gomes
**Student ID:** 5678035
**Assignment:** Lexer, HW2

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
September 25, 2025

### Specific Parts of Assignment
- Creation of the two base regexes to capture words, symbols, and comments.
- Translating Rust to C
- Usage of regex.h and correct function signatures from it

### Prompts Used
1. Generate regex pattern that matches /* anything here, with no exceptions */
2. Using regex crate, in Rust, create the regex pattern that returns any string that follows the regex [a-zA-Z][a-z-A-Z0-9]* and accept the following tokens separately: + - * / = <> < <= > >= ( ) , ; . :=

### AI Output/Results
1. 
```rs
let re =
    Regex::new(r"(?s)/\*.*?\*/").unwrap();
```
2. 
```rs
Regex::new(r"[A-Za-z][A-Za-z0-9]*|\d+|:=|<=|>=|<>|\+|-|\*|/|=|<|>|\(|\)|,|;|\.").unwrap()
```

### How Output was Verified/Edited
Output verification was done with the following code I produced:
```rs
general_regex
    .find_iter(&content)
    .collect::<Vec<_>>()
    .iter()
    .enumerate()
    .for_each(|(i, token)| println!("{i}: {}", token.as_str()));
```
It goes over all matches that such regex found, enumerates and prints to the console. From this output, I verified if all necessary tokens were included on it, and they were. However, many empty strings were captured by the regex, so I had to remove them.

### Multiple Iterations (if applicable)
First I asked for a regex pattern that matches all symbols, and the regex provided in the assignment details: [a-zA-Z][a-z-A-Z0-9]*. Next prompt I asked for another regex, to match comments so I can remove them before tokenization. After these prompts, I took time to complete the Rust code, and finally started a new conversation to ask the AI to translate my code to C, and adapt the usage to the regex.h header

### Learning & Reflection
AI helped me understand regex patterns and translate Rust code to C, which saved time for myself, given that C is much more verbose and its libraries are not intuitive, making it highly inneficient for this assignment

---

## Overall Reflection
AI was very useful in translating Rust code to C, I could visualize how part of the high level logic from Rust is translated to C, and how bad and weak is GCC, which compiles even when the arguments to a function are incorrect, making me lose a lot of time debugging code, especially because the only way to compile it is in Eustis since the regex.h header is not available on windows
- C is a bad language choice for this assignment. In general, manipulating strings in C is hard by iteself, especially when dealing with inputs that can contain Unicode/UTF-8 characters

---

## Notes

- Be as specific and detailed as possible in your responses
- Include exact prompts and AI outputs when possible
- Explain how you verified and modified AI-generated content
- Reflect on what you learned through the AI interaction
