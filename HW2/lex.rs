// ! I prefer doing the assignments on Rust, then translating to C
// ! with help of AI. Note that regex is not part of the Rust's standard library
// ! and for simplicity I've downloaded crate "regex"; This easier and better than
// ! calling "regex.h" directly from C

use std::fmt::Display;

/// Cargo.toml
/// ```toml
/// [[bin]]
/// name = "lex"
/// path = "lex.rs"
///
/// [dependencies]
/// regex = "1"
/// ```
use regex::Regex;

/// Adapted from the provided C enum
#[derive(PartialEq, Copy, Clone, Debug)]
enum TokenType {
    Skip = 1,  // Skip / ignore token
    Ident,     // Identifier [a-zA-Z][a-z-A-Z0-9]*
    Number,    // Number [0-9]+
    Plus,      // +
    Minus,     // -
    Mult,      // *
    Slash,     // /
    Eq,        // =
    Neq,       // <>
    Les,       // <
    Leq,       // <=
    Gtr,       // >
    Geq,       // >=
    Lparent,   // (
    Rparent,   // )
    Comma,     // ,
    Semicolon, // ;
    Period,    // .
    Becomes,   // :=
    Begin,     // begin
    End,       // end
    If,        // if
    Fi,        // fi
    Then,      // then
    While,     // while
    Do,        // do
    Call,      // call
    Const,     // const
    Var,       // var
    Proc,      // procedure
    Write,     // write
    Read,      // read
    Else,      // else
    Even,      // even
}

fn main() {
    // Prompt:
    /*
    - ChatGPT 5 Thinking:
    Using regex crate, in Rust, create the regex pattern that returns any string that
    follows the regex [a-zA-Z][a-z-A-Z0-9]* and accept the following tokens separately:
    + - * / = <> < <= > >= ( ) , ; . :=
    */
    let general_regex =
        Regex::new(r"[A-Za-z][A-Za-z0-9]*|\d+|:=|<=|>=|<>|\+|-|\*|/|=|<|>|\(|\)|,|;|\.")
            .expect("Couldn't create general regex pattern");
    // I copied the [A-Za-z][A-Za-z0-9]* from the generated regex pattern
    // it is the adapted regex pattern provided in the assignment
    // (the provided one does not compile)
    let identifier_regex =
        Regex::new(r"[A-Za-z][A-Za-z0-9]*").expect("Couldn't create identifier regex pattern");

    let file_path = std::env::args()
        .nth(1)
        .expect("Missing file path in first argument");
    let content = std::fs::read_to_string(file_path)
        .expect("Couldn't read input file")
        .chars()
        .collect::<Vec<_>>()
        .iter()
        // á, ñ, ó, í, ì, ò, Ñ for example are not allowed
        .filter(|character| character.is_ascii())
        .collect::<String>();

    // before tokenizing, remove comments delimited in /* ... */
    // Note that in the announcement, it was said that /* ... */ */
    // should remove the first delimiter /* */ and tokenize * and / from the end
    // since there will be no tokenization of it, then it does not need to go to the regex
    // ChatGPT 5 Thinking:
    // - Generate regex pattern that matches /* anything here, with no exceptions */
    let comment_regex =
        Regex::new(r"(?s)/\*.*?\*/").expect("Couldn't create comment regex pattern");
    let content_without_comments = comment_regex.replace_all(&content, "");

    let tokens = general_regex
        .find_iter(&content_without_comments)
        .collect::<Vec<_>>();

    println!(
        "Source Program:\n\n{}\n\nLexeme Table:\n\n{:<10}{:<10}",
        content, "lexeme", "token type"
    );

    // Will hold the enum TokenType and some token_data in string format.
    // This will be used to print the lexeme table and the token list
    let mut result = Vec::<(TokenType, String)>::new();

    for token in tokens.iter() {
        let token_str = token.as_str();
        // Tokens can be returned empty, so they don't need to be tokenized
        // '\n', ' ', '\t', '\r' are examples of those that are returned empty
        if token_str.is_empty() {
            continue;
        } else {
            result.push({
                // Check if this is a token that can be easily determined
                // such as operands and keywords
                let base_token_type = match token_str {
                    "+" => Some(TokenType::Plus),
                    "-" => Some(TokenType::Minus),
                    "*" => Some(TokenType::Mult),
                    "/" => Some(TokenType::Slash),
                    "=" => Some(TokenType::Eq),
                    "<>" => Some(TokenType::Neq),
                    "<" => Some(TokenType::Les),
                    "<=" => Some(TokenType::Leq),
                    ">" => Some(TokenType::Gtr),
                    ">=" => Some(TokenType::Geq),
                    "(" => Some(TokenType::Lparent),
                    ")" => Some(TokenType::Rparent),
                    "," => Some(TokenType::Comma),
                    ";" => Some(TokenType::Semicolon),
                    "." => Some(TokenType::Period),
                    ":=" => Some(TokenType::Becomes),
                    "begin" => Some(TokenType::Begin),
                    "end" => Some(TokenType::End),
                    "if" => Some(TokenType::If),
                    "fi" => Some(TokenType::Fi),
                    "then" => Some(TokenType::Then),
                    "while" => Some(TokenType::While),
                    "do" => Some(TokenType::Do),
                    "call" => Some(TokenType::Call),
                    "const" => Some(TokenType::Const),
                    "var" => Some(TokenType::Var),
                    "procedure" => Some(TokenType::Proc),
                    "write" => Some(TokenType::Write),
                    "read" => Some(TokenType::Read),
                    "else" => Some(TokenType::Else),
                    "even" => Some(TokenType::Even),
                    _ => None,
                };
                match base_token_type {
                    // If it is a hardcoded defined token, token_data is just it as String
                    Some(token_type) => (token_type, token_str.to_string()),
                    // If there are Number definitions or Identifiers, it is necessary to check
                    None => {
                        // If it can be converted to integer, it is a literal number
                        if let Ok(number) = token_str.parse::<i32>() {
                            (
                                if token_str.len() > 5 {
                                    TokenType::Skip
                                } else {
                                    TokenType::Number
                                },
                                number.to_string(),
                            )
                        } else {
                            // Check if this is possibly an identifier
                            // If it is made of just characters and numbers, it is an identifier
                            // if there's an unknown character, them skip it
                            (
                                if identifier_regex.is_match(token_str) {
                                    TokenType::Ident
                                } else {
                                    TokenType::Skip
                                },
                                token_str.into(),
                            )
                        }
                    }
                }
            });
        }
    }

    for (token_type_enum, token_data) in result.iter() {
        // Number too long error was placed as Skip; so it is necessary to verify if
        // token_data is a number and fell there. If it did, then print in the token_type
        // field that the number is too long, and the value itself
        let token_type: Box<dyn Display> = if *token_type_enum == TokenType::Skip
            && token_data.len() > 5
            && token_data.parse::<i32>().is_ok()
        {
            Box::new("Number too long")
        } else {
            // token_type is copy, dereference it gives itself without move issues
            // token_type is an enum valid with #[repr(inttype)]
            // in this case: represented as a single byte (< 255 variants),
            // so it can be converted directly to an integer by casting
            Box::new(*token_type_enum as usize)
        };

        println!("{:<10}{:<10}", token_data, token_type);
    }

    println!("\nToken List:\n");

    for (token_type, token_data) in result.iter() {
        print!(
            "{} ",
            match token_type {
                // Literals and identifiers are printed with their token_data
                // any other ones are just the numeric representation of token_type
                TokenType::Ident | TokenType::Number => {
                    format!("{} {}", *token_type as usize, token_data)
                }
                _ => format!("{}", *token_type as usize),
            }
        );
    }
    println!();
}
