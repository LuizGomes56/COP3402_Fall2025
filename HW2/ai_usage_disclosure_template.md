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
3. I need to translate a Rust code to C; It uses regex crate, and in C I want to use the `<regex.h>` header in a LINUX system. Begin the translation, if a dynamic dispatch is found, you may hardcode it; try using similar function signatures, and when possible, use the same implementation as in the Rust standard library; Source: [I removed comments and minified using https://unminifyall.com/minify-rust/]
```rs
use std::fmt::Display;use regex::Regex;enum TokenType { Skip = 1,  Ident,  Number,  Plus,  Minus,  Mult,  Slash,  Eq,  Neq,  Les,  Leq,  Gtr,  Geq,  Lparent,  Rparent,  Comma,  Semicolon,  Period,  Becomes,  Begin,  End,  If,  Fi,  Then,  While,  Do,  Call,  Const,  Var,  Proc,  Write,  Read,  Else,  Even, }fn main() {   let general_regex = Regex::new(r"[A-Za-z][A-Za-z0-9]*|\d+|:=|<=|>=|<>|\+|-|\*|/|=|<|>|\(|\)|,|;|\.") .expect("Couldn't create general regex pattern");    let identifier_regex = Regex::new(r"[A-Za-z][A-Za-z0-9]*").expect("Couldn't create identifier regex pattern"); let file_path = std::env::args() .nth(1) .expect("Missing file path in first argument"); let content = std::fs::read_to_string(file_path) .expect("Couldn't read input file") .chars() .collect::<Vec<_>>() .iter()  .filter(|character| character.is_ascii()) .collect::<String>();       let comment_regex = Regex::new(r"(?s)/\*.*?\*/").expect("Couldn't create comment regex pattern"); let content_without_comments = comment_regex.replace_all(&content, ""); let tokens = general_regex .find_iter(&content_without_comments) .collect::<Vec<_>>(); println!( "Source Program:\n\n{}\n\nLexeme Table:\n\n{:<10}{:<10}", content, "lexeme", "token type" );   let mut result = Vec::<(TokenType, String)>::new(); for token in tokens.iter() { let token_str = token.as_str();   if token_str.is_empty() { continue; } else { result.push({   let base_token_type = match token_str { "+" => Some(TokenType::Plus), "-" => Some(TokenType::Minus), "*" => Some(TokenType::Mult), "/" => Some(TokenType::Slash), "=" => Some(TokenType::Eq), "<>" => Some(TokenType::Neq), "<" => Some(TokenType::Les), "<=" => Some(TokenType::Leq), ">" => Some(TokenType::Gtr), ">=" => Some(TokenType::Geq), "(" => Some(TokenType::Lparent), ")" => Some(TokenType::Rparent), "," => Some(TokenType::Comma), ";" => Some(TokenType::Semicolon), "." => Some(TokenType::Period), ":=" => Some(TokenType::Becomes), "begin" => Some(TokenType::Begin), "end" => Some(TokenType::End), "if" => Some(TokenType::If), "fi" => Some(TokenType::Fi), "then" => Some(TokenType::Then), "while" => Some(TokenType::While), "do" => Some(TokenType::Do), "call" => Some(TokenType::Call), "const" => Some(TokenType::Const), "var" => Some(TokenType::Var), "procedure" => Some(TokenType::Proc), "write" => Some(TokenType::Write), "read" => Some(TokenType::Read), "else" => Some(TokenType::Else), "even" => Some(TokenType::Even), _ => None, }; match base_token_type {  Some(token_type) => (token_type, token_str.to_string()),  None => {  if let Ok(number) = token_str.parse::<i32>() { ( if token_str.len() > 5 { TokenType::Skip } else { TokenType::Number }, number.to_string(), ) } else {    ( if identifier_regex.is_match(token_str) { TokenType::Ident } else { TokenType::Skip }, token_str.into(), ) } } } }); } } for (token_type_enum, token_data) in result.iter() {    let token_type: Box<dyn Display> = if *token_type_enum == TokenType::Skip && token_data.len() > 5 && token_data.parse::<i32>().is_ok() { Box::new("Number too long") } else {     Box::new(*token_type_enum as usize) }; println!("{:<10}{:<10}", token_data, token_type); } println!("\nToken List:\n"); for (token_type, token_data) in result.iter() { print!( "{} ", match token_type {   TokenType::Ident | TokenType::Number => { format!("{} {}", *token_type as usize, token_data) } _ => format!("{}", *token_type as usize), } ); } println!();}
```

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
3. The output was similar to the final code, I renamed and fixed some parts of it, added checks to ASCII, removed empty strings from the regex and more; -> File read to the end and regex + comment removal usage was entirely translated by ChatGPT, and I added my interpretation about it.
```c
#define _XOPEN_SOURCE 700
#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
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
    TokenType_Even
} TokenType;

static char *read_file_ascii_filtered(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("Couldn't open file");
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        fprintf(stderr, "ftell failed\n");
        exit(1);
    }
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((int)sz + 1);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    int rd = fread(buf, 1, (int)sz, f);
    fclose(f);
    buf[rd] = '\0';
    char *out = (char *)malloc(rd + 1);
    if (!out) {
        free(buf);
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    int w = 0;
    for (int i = 0; i < rd; i++) {
        unsigned char c = (unsigned char)buf[i];
        if (c <= 0x7F)
            out[w++] = (char)c;
    }
    out[w] = '\0';
    free(buf);
    return out;
}

static const char *remove_block_comments(const char *s) {
    int n = strlen(s);
    char *out = (char *)malloc(n + 1);
    if (!out) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    int i = 0, w = 0;
    while (i < n) {
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(s[i] == '*' && s[i + 1] == '/')) {
                i++;
            }
            if (i + 1 < n) {
                i += 2;
            }
        } else {
            out[w++] = s[i++];
        }
    }
    out[w] = '\0';
    return out;
}

static bool str_eq(char *a, char *b) { return strcmp(a, b) == 0; }

static bool match_token_type(char *lex, TokenType *out) {
    if (str_eq(lex, "+")) {
        *out = TokenType_Plus;
        return true;
    }
    if (str_eq(lex, "-")) {
        *out = TokenType_Minus;
        return true;
    }
    if (str_eq(lex, "*")) {
        *out = TokenType_Mult;
        return true;
    }
    if (str_eq(lex, "/")) {
        *out = TokenType_Slash;
        return true;
    }
    if (str_eq(lex, "=")) {
        *out = TokenType_Eq;
        return true;
    }
    if (str_eq(lex, "<>")) {
        *out = TokenType_Neq;
        return true;
    }
    if (str_eq(lex, "<")) {
        *out = TokenType_Les;
        return true;
    }
    if (str_eq(lex, "<=")) {
        *out = TokenType_Leq;
        return true;
    }
    if (str_eq(lex, ">")) {
        *out = TokenType_Gtr;
        return true;
    }
    if (str_eq(lex, ">=")) {
        *out = TokenType_Geq;
        return true;
    }
    if (str_eq(lex, "(")) {
        *out = TokenType_Lparent;
        return true;
    }
    if (str_eq(lex, ")")) {
        *out = TokenType_Rparent;
        return true;
    }
    if (str_eq(lex, ",")) {
        *out = TokenType_Comma;
        return true;
    }
    if (str_eq(lex, ";")) {
        *out = TokenType_Semicolon;
        return true;
    }
    if (str_eq(lex, ".")) {
        *out = TokenType_Period;
        return true;
    }
    if (str_eq(lex, ":=")) {
        *out = TokenType_Becomes;
        return true;
    }
    if (str_eq(lex, "begin")) {
        *out = TokenType_Begin;
        return true;
    }
    if (str_eq(lex, "end")) {
        *out = TokenType_End;
        return true;
    }
    if (str_eq(lex, "if")) {
        *out = TokenType_If;
        return true;
    }
    if (str_eq(lex, "fi")) {
        *out = TokenType_Fi;
        return true;
    }
    if (str_eq(lex, "then")) {
        *out = TokenType_Then;
        return true;
    }
    if (str_eq(lex, "while")) {
        *out = TokenType_While;
        return true;
    }
    if (str_eq(lex, "do")) {
        *out = TokenType_Do;
        return true;
    }
    if (str_eq(lex, "call")) {
        *out = TokenType_Call;
        return true;
    }
    if (str_eq(lex, "const")) {
        *out = TokenType_Const;
        return true;
    }
    if (str_eq(lex, "var")) {
        *out = TokenType_Var;
        return true;
    }
    if (str_eq(lex, "procedure")) {
        *out = TokenType_Proc;
        return true;
    }
    if (str_eq(lex, "write")) {
        *out = TokenType_Write;
        return true;
    }
    if (str_eq(lex, "read")) {
        *out = TokenType_Read;
        return true;
    }
    if (str_eq(lex, "else")) {
        *out = TokenType_Else;
        return true;
    }
    if (str_eq(lex, "even")) {
        *out = TokenType_Even;
        return true;
    }

    return false;
}

int main {
    ...
    char *content = read_file_ascii_filtered(argv[1]);
    char *content_wo_comments = remove_block_comments(content);
    char *GENERAL_PATTERN =
        "([A-Za-z][A-Za-z0-9]*)|([0-9]+)|(:=)|(<=)|(>=)|(<>)|(\\+)|(-)|(\\*)|(/)|(=)|(<)|(>)|(\\()|(\\))|(,)|(;)|(\\.)";
    char *IDENTIFIER_PATTERN = "^[A-Za-z][A-Za-z0-9]*$";
    regex_t general_regex, identifier_regex;
    regcomp(&general_regex, GENERAL_PATTERN, REG_EXTENDED);
    regcomp(&identifier_regex, IDENTIFIER_PATTERN, REG_EXTENDED);
    int cap = 512, len = 0;
    Token *result = (Token *)malloc(cap * sizeof(Token));
    if (!result) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    char *text = content_wo_comments;
    int text_len = strlen(text);
    int pos = 0;
    regmatch_t m[1];
    while (pos <= text_len) {
        regexec(&general_regex, text + pos, 1, m, 0);
        if (m[0].rm_so == -1) {
            break;
        }
        int so = (int)m[0].rm_so;
        int eo = (int)m[0].rm_eo;

        int tok_start = pos + so;
        int tok_len = eo - so;

        if (tok_len == 0) {
            pos += eo; 
            continue;
        }

        char *lex = (char *)malloc(tok_len + 1);
        memcpy(lex, text + tok_start, tok_len);
        lex[tok_len] = '\0';

        TokenType token_type;
        if (match_token_type(lex, &token_type)) {}
        else {
            if (is_all_digits(lex)) {
                if (strlen(lex) > 5)
                    token_type = TokenType_Skip;
                else
                    token_type = TokenType_Number;
            } else {
                regmatch_t mm[1];
                if (regexec(&identifier_regex, lex, 1, mm, 0) == 0) {
                    token_type = TokenType_Ident;
                } else {
                    token_type = TokenType_Skip;
                }
            }
        }
        if (len == cap) {
            cap *= 2;
            result = (Token *)realloc(result, cap * sizeof(Token));
            if (!result) {
                fprintf(stderr, "Couldn't grow the Token Vec\n");
                exit(1);
            }
        }
        result[len].type = token_type;
        result[len].lexeme = lex;
        len++;

        pos = tok_start + tok_len;
    }

    for (int i = 0; i < len; i++) {
        Token *token = &result[i];
        char *special = NULL;
        if (token->type == TokenType_Skip && strlen(token->lexeme) > 5 && is_all_digits(token->lexeme)) {
            special = "Number too long";
        }
        if (special) {
            printf("%-10s%-10s\n", token->lexeme, special);
        } else {
            if (strlen(token->lexeme) == 0) {
                continue;
            }

            printf("%-10s%-10d\n", token->lexeme, (int)token->type);
        }
    }

    for (int i = 0; i < len; i++) {
        Token *token = &result[i];
        switch (token->type) {
        case TokenType_Ident:
        case TokenType_Number:
            printf("%d %s ", (int)token->type, token->lexeme);
            break;
        default:
            if (strlen(token->lexeme) == 0) {
                continue;
            }
            printf("%d ", (int)token->type);
            break;
        }
    }
    printf("\n");

    for (int i = 0; i < len; i++) {
        free(result[i].lexeme);
    }
    free(result);
    regfree(&general_regex);
    regfree(&identifier_regex);
    free(content_wo_comments);
    free(content);

    return 0;
}
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
First I asked for a regex pattern that matches all symbols, and the regex provided in the assignment details: [a-zA-Z][a-z-A-Z0-9]*. Next prompt I asked for another regex, to match comments so I can remove them before tokenization. After these prompts, I took time to complete the Rust code, and finally started a new conversation to ask the AI to translate my code to C, and adapt the usage to the regex.h header, and make is as close as possible to my Rust implementation

### Learning & Reflection
AI helped me understand regex patterns and translate Rust code to C, which saved time for myself, given that C is much more verbose and its libraries are not intuitive, making it highly inneficient for this assignment. Also, my code in Rust is about 200 lines long, while the one in C is about 500, which was good to notice how modern languages are better to write than C, especially the Regex library, which in C is incomprehensive and has poor documentation, while the one in Rust is great and include many examples on how to use it (I have used regex in Rust several times before this assignment, but not in C, this assignment showed me how hard and inneficient it is to use it in C)

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
