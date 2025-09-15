const NORW: usize = 15; // Reserved Words
const IMAX: i32 = i32::MAX; // Maximum integer value
const CMAX: usize = 11; // Maximum number of chars for idents
const STRMAX: usize = 256; // Maximum length of strings

enum TokenType {
    Const,
    Begin,
    End,
    If,
    Then,
    While,
    Number,
    Plus,
    Minus,
    Mult,
    Slash,
    Comma,
    Period,
    Proc,
    Become,
    Eq,
    Less,
    Leq,
    Gtr,
    Geq,
    Lparen,
    Rparen,
    Semicolon,
    Odd,
    Id,
    Nul,
    Do,
    Call,
    Var,
    Write,
}

enum Keywords {
    Null,
    Begin,
    Call,
    Const,
    Do,
    Else,
    End,
    If,
    Odd,
    Procedure,
    Read,
    Then,
    Var,
    While,
    Write,
}

fn main() {}
