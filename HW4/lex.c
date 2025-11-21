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

// Regex type (EXTEND) - Not available on Windows (config attribute)
#define _XOPEN_SOURCE 700
#include <ctype.h>
// Not available on Windows, but for debugging purposes, this header can be
// downloaded through the following link
// #![https://codebrowser.dev/linux/include/regex.h.html]
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Modified version of:
// #[derive(PartialEq, Copy, Clone, Debug)]
// enum TokenType {
//     Skip = 1,
//     Ident,
//     Number,
//     Plus,
//     ...
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

// I used a tuple in Rust, but it does not exist in C, so this struct was created
// to address this issue
typedef struct {
    TokenType type;
    char *lexeme; // [Lexeme] [Token Type] is the way it is printed to the console
} Token;

// Read the whole file and filter non-ascii characters.
// ChatGPT 5 Thinking: Translate the following to C:
// let content = std::fs::read_to_string(file_path)
//     .expect("Couldn't read input file")
//     .chars()
//     .collect::<Vec<_>>()
//     .iter()
//     // á, ñ, ó, í, ì, ò, Ñ for example are not allowed
//     .filter(|character| character.is_ascii())
//     .collect::<String>();
// AKA: read file and convert to string;
// .expect() -> exit if not found and display the message
// .chars() + collect::<Vec<char>>() -> transform the string in a sequence of chars, dynamically allocated
// .iter() -> iterate through EACH character of the original string
// .is_ascii() -> seeing if that char is ASCII (In Rust char size = 4, not 1 like in C)
// so this is much easier to do (UTF-8 and Unicode support)
// .filter() + .is_ascii() -> non-ASCII characters are not added to the final result;
// .collect::<String>() -> add everything back into a single string
char *read_file_ascii_filtered(char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("Couldn't open file");
        exit(1);
    }
    // Chat GPT 5 Thinking;
    // [Relevant source](https://www.freetimelearning.com/c-language/c-language-ftell-fseek.php)
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        fprintf(stderr, "ftell failed\n");
        exit(1);
    }
    fseek(f, 0, SEEK_SET);

    // Allocate string where the results will be added
    char *buf = (char *)malloc((int)sz + 1);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    int rd = fread(buf, 1, (int)sz, f);
    fclose(f);
    // at last index, add null char to match a C-like string
    // AKA std::ffi::CString
    buf[rd] = '\0';

    // filter only ASCII, (In Rust there's the method .is_ascii() is used)
    char *out = (char *)malloc(rd + 1);
    // Malloc failing is nearly impossible but this is a standard in C
    // as well as in the CS1 FE
    if (!out) {
        free(buf);
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    int w = 0;
    // same as the .iter() call
    for (int i = 0; i < rd; i++) {
        // THIS IS THE EXACT SIGNATURE for the .is_ascii() method IN RUST,
        // so I just translated it directly
        // ```rs
        // pub const fn is_ascii(&self) -> bool {
        //     *self as u32 <= 0x7F
        // }
        // ```
        // char in Rust takes up 4 bytes, but in C it takes up 1 so in fact it just
        // has to be converted to unsigned and checked if <= 0x7F (127 in decimal)
        unsigned char c = (unsigned char)buf[i];
        if (c <= 0x7F)
            // convert back to char
            out[w++] = (char)c;
    }
    out[w] = '\0';
    // buffer has temporary value and should be dropped at the end
    free(buf);
    return out;
}

// Remove the /* comments */, but without using regex
// Equivalent:
// let comment_regex =
//     Regex::new(r"(?s)/\*.*?\*/").expect("Couldn't create comment regex pattern");
// let content_without_comments = comment_regex.replace_all(&content, "");
// .replace_all does what this function is doing
// ChatGPT 5 Thinking: Translate the rust code to C
char *remove_block_comments(char *s) {
    // s.len()
    int n = strlen(s);
    // let out = String::new();
    char *out = (char *)malloc(n + 1);
    // malloc useless fail check
    if (!out) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    // CHAT GPT 5 THINKING: Translated the Rust regex to an inline C code with no regex
    // In the script I told to do a literal translation but it did not do it for this one,
    // but worked anyway. Appearently this code finds the first '/' and check if the next
    // char is a '*', if it is, it starts to record indexes to remove these chars from the
    // original source, until a '*' followed by '/' is found.
    // ! -> I did not change anything from the generated code in this part, rather I just
    // interpreted it and checked if this behaves the same way as the code I provided in Rust
    int i = 0, w = 0;
    while (i < n) {
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            // consume "*/" until the end
            while (i + 1 < n && !(s[i] == '*' && s[i + 1] == '/')) {
                i++;
            }
            if (i + 1 < n) {
                i += 2; // skip "*/"
            }
        } else {
            out[w++] = s[i++];
        }
    }
    out[w] = '\0';
    return out;
}

// Strings and &str implement PartialEq in Rust, so this is just a helper that works like:
// if (String)A == (String)B { true } else { false }
bool str_eq(char *a, char *b) { return strcmp(a, b) == 0; }

// same as the big match arm used in Rust to check keywords and symbols. The
// _ => {} match is not covered here (Number, Ident, Skip) enumerations
// similar signature:
// match_token_type(lex: &mut str, out: &mut TokenType);
bool match_token_type(char *lex, TokenType *out) {
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

// returns true if the input string can be parsed to a number (aka all characters on it are numeric)
// Same as s.parse::<i32>().is_ok()
// parse returns a Result<T, E>, and is_ok drops whatever value came, and just says if it was successful or not
// this is what this function is essentially doing
// ChatGPT 5 THINKING: Translate this code to C: s.parse::<i32>().is_ok()
bool is_all_digits(char *s) {
    // Empty strings are not numeric
    if (*s == '\0') {
        return false;
    }
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        // function provided by ctype header
        if (!isdigit(*p)) {
            return false;
        }
    }
    return true;
}

// there were several tokens in the lexeme table that were printed as:
//     1
//    1  or similar (1 = Skip)
// so invisible characters were being printed and set as skip, but they should be
// removed instead of "skipped", so this function serve to not print a string if
// it is composed exclusively of invisible characters
bool has_visible_char(char *s) {
    // s can't be null since it will be dereferenced
    if (!s)
        return false;
    for (; *s; s++) {
        // if there's at least one visible character, return that it's true
        if (!isspace((unsigned char)*s)) {
            return true;
        }
    }
    // if there were only whitespaces or \t, \n, etc
    return false;
}

int main(int argc, char **argv) {
    // argv[0] is program execution path, and argv[1] must exist so the
    // input file path is passed as argumnet. argc < 2 mean that there are no
    // arguments
    if (argc < 2) {
        fprintf(stderr, "Missing file path in first argument\n");
        return 1;
    }

    // Read input file AND remove non-ASCII characters;
    // argv[1] is the path to input file
    char *content = read_file_ascii_filtered(argv[1]);

    // Remove all comments from input /* ... */ since they won't be parsed
    char *content_wo_comments = remove_block_comments(content);

    // ChatGPT 5 Thinking: Creating regex (Disclosed in AI Form)
    // identifier | number | := | <= | >= | <> | + | - | * | / | = | < | > | ( | ) | , | ; | .
    char *GENERAL_PATTERN =
        "([A-Za-z][A-Za-z0-9]*)|([0-9]+)|(:=)|(<=)|(>=)|(<>)|(\\+)|(-)|(\\*)|(/)|(=)|(<)|(>)|(\\()|(\\))|(,)|(;)|(\\.)";

    // ChatGPT 5 Thinking: Generate the identifier regex from the one used in the Rust program
    char *IDENTIFIER_PATTERN = "^[A-Za-z][A-Za-z0-9]*$";

    // [Source 1](https://www.geeksforgeeks.org/c/regular-expressions-in-c/)
    // [Source 2](https://thelinuxcode.com/regular-expression-c/)
    // [Source 3](https://www.geeksforgeeks.org/dsa/write-regular-expressions/)
    // [Source 4](Chat GPT 5 Thinking)
    regex_t general_regex, identifier_regex;

    // No error handling, these regexes compile
    regcomp(&general_regex, GENERAL_PATTERN, REG_EXTENDED);
    regcomp(&identifier_regex, IDENTIFIER_PATTERN, REG_EXTENDED);

    // Iterate through regex matches(same as find_iter)
    int cap = 512, len = 0;

    // Vec::<(TokenType, String)>::new()
    Token *result = (Token *)malloc(cap * sizeof(Token));
    if (!result) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }

    char *text = content_wo_comments;
    int text_len = strlen(text);
    int pos = 0;

    // The next code was translated from Rust to C by ChatGPT
    regmatch_t m[1];
    while (pos <= text_len) {
        // Chat GPT 5 Thinking: How to execute Regex in C
        regexec(&general_regex, text + pos, 1, m, 0);

        // Literal description of .rm_so is:
        // Byte offset from string's start to substring's start
        // -1 is used as error/no match
        // Chat GPT 5 Thinking: How to check if a regex match was found
        if (m[0].rm_so == -1) {
            break;
        }

        // rm_so = Byte offset from string's start to substring's start
        int so = (int)m[0].rm_so;
        // rm_eo = Byte offset from string's start to substring's end
        int eo = (int)m[0].rm_eo;

        // lexeme encontrado
        int tok_start = pos + so;
        int tok_len = eo - so;

        // if token_str.is_empty() {
        //     continue;
        // }
        if (tok_len == 0) {
            pos += eo; // If string is empty, ignore it
            continue;
        }

        char *lex = (char *)malloc(tok_len + 1);
        // Probably the same as cloning a String from one buffer to another (In Rust)
        memcpy(lex, text + tok_start, tok_len);
        lex[tok_len] = '\0';

        // clasiffy what kind of token it is
        TokenType token_type;
        if (match_token_type(lex, &token_type)) {
            // token_type was passed as mut ref, and if it returns true, its value
            // was already modified. Nothing to do here
        }
        // _ => None,
        // Check if it is identifier, number, or skip
        else {
            // if let Ok(number) = token_str.parse::<i32>()
            if (is_all_digits(lex)) {
                // if token_str.len() > 5 {
                //     TokenType::Skip
                // } else {
                //     TokenType::Number
                // },
                if (strlen(lex) > 5)
                    token_type = TokenType_Skip;
                else
                    token_type = TokenType_Number;
            } else {
                // Check if it is an identifier
                // if identifier_regex.is_match(token_str) {
                //     TokenType::Ident
                // } else {
                //     TokenType::Skip
                // }
                regmatch_t mm[1];
                // if the execution of the identifier regex was Ok, then this token
                // is a valid identifier. Otherwise skip it (invalid sequence)
                if (regexec(&identifier_regex, lex, 1, mm, 0) == 0) {
                    token_type = TokenType_Ident;
                } else {
                    token_type = TokenType_Skip;
                }
            }
        }

        // This is pretty much useless since 512 is a good boundary
        // Grows the vector if its capacity is reached
        // In Rust this is done automatically, so there's no equivalent translation
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

        pos = tok_start + tok_len; // go to next token
    }

    // println!(
    //     "Source Program:\n\n{}\n\nLexeme Table:\n\n{:<10}{:<10}",
    //     content, "lexeme", "token type"
    // );

    printf("Source Program:\n\n%s\n\n", content);
    printf("Lexeme Table:\n\n%-10s%-10s\n", "lexeme", "token type");

    for (int i = 0; i < len; i++) {
        Token *token = &result[i];
        // get over the Box<dyn Display> usage;
        // That's why in the script I said to ignore dynamic dispatch and hardcode it
        char *special = NULL;

        // same as:
        // if *token_type_enum == TokenType::Skip && token_data.len() > 5 && token_data.parse::<i32>().is_ok()
        // So it is SKIP, length > 5, and numeric
        if (token->type == TokenType_Skip && strlen(token->lexeme) > 5 && is_all_digits(token->lexeme)) {
            special = "Number too long";
        }
        if (special) {
            printf("%-10s%-10s\n", token->lexeme, special);
        } else {
            // check if token->lexeme is empty, if it is, do nothing
            // this is because this C regex was not perfectly translated from Rust and is capturing
            // characters that were not intended to be in the string
            if (!has_visible_char(token->lexeme)) {
                continue;
            }

            // println!("{:<10}{:<10}", token_data, token_type);
            printf("%-10s%-10d\n", token->lexeme, (int)token->type);
        }
    }

    printf("\nToken List:\n\n");
    for (int i = 0; i < len; i++) {
        Token *token = &result[i];
        switch (token->type) {
        // TokenType::Ident | TokenType::Number => ...
        // In these two cases we print the "data" from that token
        // ex: ident_num ident_name or number_num number_literal
        case TokenType_Ident:
        case TokenType_Number:
            printf("%d %s ", (int)token->type, token->lexeme);
            break;
        default:
            // avoid printing 1's that were not intended to be printed (Skip)
            if (!has_visible_char(token->lexeme)) {
                continue;
            }
            // Print just the numeric definition of that token
            // (In Rust this is the cast of the enum to usize)
            printf("%d ", (int)token->type);
            break;
        }
    }
    printf("\n");

    // Save token list to tokens.txt
    FILE *tokens = fopen("tokens.txt", "w");
    for (int i = 0; i < len; i++) {
        Token *token = &result[i];
        switch (token->type) {
        case TokenType_Ident:
        case TokenType_Number:
            fprintf(tokens, "%d %s\n", (int)token->type, token->lexeme);
            break;
        default:
            if (!has_visible_char(token->lexeme)) {
                continue;
            }
            fprintf(tokens, "%d\n", (int)token->type);
            break;
        }
    }
    fclose(tokens);

    // Cleanup
    for (int i = 0; i < len; i++) {
        free(result[i].lexeme);
    }
    free(result);
    // ChatGPT 5 Thinking: Free Regexes
    regfree(&general_regex);
    regfree(&identifier_regex);
    free(content_wo_comments);
    free(content);

    return 0;
}

// Rust code (Original)
// To run: cargo run <input.txt> (Add the proper Cargo.toml file to add the Regex crate)
// // ! I prefer doing the assignments on Rust, then translating to C
// // ! with help of AI. Note that regex is not part of the Rust's standard library
// // ! and for simplicity I've downloaded crate "regex"; This easier and better than
// // ! calling "regex.h" directly from C

// use std::fmt::Display;

// /// Cargo.toml
// /// ```toml
// /// [[bin]]
// /// name = "lex"
// /// path = "lex.rs"
// ///
// /// [dependencies]
// /// regex = "1"
// /// ```
// use regex::Regex;

// /// Adapted from the provided C enum
// #[derive(PartialEq, Copy, Clone, Debug)]
// enum TokenType {
//     Skip = 1,  // Skip / ignore token
//     Ident,     // Identifier [a-zA-Z][a-z-A-Z0-9]*
//     Number,    // Number [0-9]+
//     Plus,      // +
//     Minus,     // -
//     Mult,      // *
//     Slash,     // /
//     Eq,        // =
//     Neq,       // <>
//     Les,       // <
//     Leq,       // <=
//     Gtr,       // >
//     Geq,       // >=
//     Lparent,   // (
//     Rparent,   // )
//     Comma,     // ,
//     Semicolon, // ;
//     Period,    // .
//     Becomes,   // :=
//     Begin,     // begin
//     End,       // end
//     If,        // if
//     Fi,        // fi
//     Then,      // then
//     While,     // while
//     Do,        // do
//     Call,      // call
//     Const,     // const
//     Var,       // var
//     Proc,      // procedure
//     Write,     // write
//     Read,      // read
//     Else,      // else
//     Even,      // even
// }

// fn main() {
//     // Prompt:
//     /*
//     - ChatGPT 5 Thinking:
//     Using regex crate, in Rust, create the regex pattern that returns any string that
//     follows the regex [a-zA-Z][a-z-A-Z0-9]* and accept the following tokens separately:
//     + - * / = <> < <= > >= ( ) , ; . :=
//     */
//     let general_regex =
//         Regex::new(r"[A-Za-z][A-Za-z0-9]*|\d+|:=|<=|>=|<>|\+|-|\*|/|=|<|>|\(|\)|,|;|\.")
//             .expect("Couldn't create general regex pattern");
//     // I copied the [A-Za-z][A-Za-z0-9]* from the generated regex pattern
//     // it is the adapted regex pattern provided in the assignment
//     // (the provided one does not compile)
//     let identifier_regex =
//         Regex::new(r"[A-Za-z][A-Za-z0-9]*").expect("Couldn't create identifier regex pattern");

//     let file_path = std::env::args()
//         .nth(1)
//         .expect("Missing file path in first argument");
//     let content = std::fs::read_to_string(file_path)
//         .expect("Couldn't read input file")
//         .chars()
//         .collect::<Vec<_>>()
//         .iter()
//         // á, ñ, ó, í, ì, ò, Ñ for example are not allowed
//         .filter(|character| character.is_ascii())
//         .collect::<String>();

//     // before tokenizing, remove comments delimited in /* ... */
//     // Note that in the announcement, it was said that /* ... */ */
//     // should remove the first delimiter /* */ and tokenize * and / from the end
//     // since there will be no tokenization of it, then it does not need to go to the regex
//     // ChatGPT 5 Thinking:
//     // - Generate regex pattern that matches /* anything here, with no exceptions */
//     let comment_regex =
//         Regex::new(r"(?s)/\*.*?\*/").expect("Couldn't create comment regex pattern");
//     let content_without_comments = comment_regex.replace_all(&content, "");

//     let tokens = general_regex
//         .find_iter(&content_without_comments)
//         .collect::<Vec<_>>();

//     println!(
//         "Source Program:\n\n{}\n\nLexeme Table:\n\n{:<10}{:<10}",
//         content, "lexeme", "token type"
//     );

//     // Will hold the enum TokenType and some token_data in string format.
//     // This will be used to print the lexeme table and the token list
//     let mut result = Vec::<(TokenType, String)>::new();

//     for token in tokens.iter() {
//         let token_str = token.as_str();
//         // Tokens can be returned empty, so they don't need to be tokenized
//         // '\n', ' ', '\t', '\r' are examples of those that are returned empty
//         if token_str.is_empty() {
//             continue;
//         } else {
//             result.push({
//                 // Check if this is a token that can be easily determined
//                 // such as operands and keywords
//                 let base_token_type = match token_str {
//                     "+" => Some(TokenType::Plus),
//                     "-" => Some(TokenType::Minus),
//                     "*" => Some(TokenType::Mult),
//                     "/" => Some(TokenType::Slash),
//                     "=" => Some(TokenType::Eq),
//                     "<>" => Some(TokenType::Neq),
//                     "<" => Some(TokenType::Les),
//                     "<=" => Some(TokenType::Leq),
//                     ">" => Some(TokenType::Gtr),
//                     ">=" => Some(TokenType::Geq),
//                     "(" => Some(TokenType::Lparent),
//                     ")" => Some(TokenType::Rparent),
//                     "," => Some(TokenType::Comma),
//                     ";" => Some(TokenType::Semicolon),
//                     "." => Some(TokenType::Period),
//                     ":=" => Some(TokenType::Becomes),
//                     "begin" => Some(TokenType::Begin),
//                     "end" => Some(TokenType::End),
//                     "if" => Some(TokenType::If),
//                     "fi" => Some(TokenType::Fi),
//                     "then" => Some(TokenType::Then),
//                     "while" => Some(TokenType::While),
//                     "do" => Some(TokenType::Do),
//                     "call" => Some(TokenType::Call),
//                     "const" => Some(TokenType::Const),
//                     "var" => Some(TokenType::Var),
//                     "procedure" => Some(TokenType::Proc),
//                     "write" => Some(TokenType::Write),
//                     "read" => Some(TokenType::Read),
//                     "else" => Some(TokenType::Else),
//                     "even" => Some(TokenType::Even),
//                     _ => None,
//                 };
//                 match base_token_type {
//                     // If it is a hardcoded defined token, token_data is just it as String
//                     Some(token_type) => (token_type, token_str.to_string()),
//                     // If there are Number definitions or Identifiers, it is necessary to check
//                     None => {
//                         // If it can be converted to integer, it is a literal number
//                         if let Ok(number) = token_str.parse::<i32>() {
//                             (
//                                 if token_str.len() > 5 {
//                                     TokenType::Skip
//                                 } else {
//                                     TokenType::Number
//                                 },
//                                 number.to_string(),
//                             )
//                         } else {
//                             // Check if this is possibly an identifier
//                             // If it is made of just characters and numbers, it is an identifier
//                             // if there's an unknown character, them skip it
//                             (
//                                 if identifier_regex.is_match(token_str) {
//                                     TokenType::Ident
//                                 } else {
//                                     TokenType::Skip
//                                 },
//                                 token_str.into(),
//                             )
//                         }
//                     }
//                 }
//             });
//         }
//     }

//     for (token_type_enum, token_data) in result.iter() {
//         // Number too long error was placed as Skip; so it is necessary to verify if
//         // token_data is a number and fell there. If it did, then print in the token_type
//         // field that the number is too long, and the value itself
//         let token_type: Box<dyn Display> = if *token_type_enum == TokenType::Skip
//             && token_data.len() > 5
//             && token_data.parse::<i32>().is_ok()
//         {
//             Box::new("Number too long")
//         } else {
//             // token_type is copy, dereference it gives itself without move issues
//             // token_type is an enum valid with #[repr(inttype)]
//             // in this case: represented as a single byte (< 255 variants),
//             // so it can be converted directly to an integer by casting
//             Box::new(*token_type_enum as usize)
//         };

//         println!("{:<10}{:<10}", token_data, token_type);
//     }

//     println!("\nToken List:\n");

//     for (token_type, token_data) in result.iter() {
//         print!(
//             "{} ",
//             match token_type {
//                 // Literals and identifiers are printed with their token_data
//                 // any other ones are just the numeric representation of token_type
//                 TokenType::Ident | TokenType::Number => {
//                     format!("{} {}", *token_type as usize, token_data)
//                 }
//                 _ => format!("{}", *token_type as usize),
//             }
//         );
//     }
//     println!();
// }
