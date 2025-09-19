// const NORW: usize = 15; // Reserved Words
// const IMAX: i32 = i32::MAX; // Maximum integer value
// const CMAX: usize = 11; // Maximum number of chars for idents
// const STRMAX: usize = 256; // Maximum length of strings

// enum TokenType {
//     Const,
//     Begin,
//     End,
//     If,
//     Then,
//     While,
//     Number,
//     Plus,
//     Minus,
//     Mult,
//     Slash,
//     Comma,
//     Period,
//     Proc,
//     Become,
//     Eq,
//     Less,
//     Leq,
//     Gtr,
//     Geq,
//     Lparen,
//     Rparen,
//     Semicolon,
//     Odd,
//     Id,
//     Nul,
//     Do,
//     Call,
//     Var,
//     Write,
// }

// enum Keywords {
//     Null,
//     Begin,
//     Call,
//     Const,
//     Do,
//     Else,
//     End,
//     If,
//     Odd,
//     Procedure,
//     Read,
//     Then,
//     Var,
//     While,
//     Write,
// }

fn main() {}

enum RWords {
    Begin = 20,
    End,
    If,
    Fi,
    Then,
    While,
    Do,
    Call,
    Const,
    Var,
    Procedure,
    Write,
    Read,
    Else,
    Even,
}

enum __Token {
    Identifier = 2, // [a-zA-Z][a-z-A-Z0-9]*
    Number = 3,     // [0-9]+
}

enum IgnoredElements {
    Comment, // /* */
}

enum Characters {
    Space,
    Tab,
    NewLine,
    Carriage,
}

// No more than 11 characters;
// Numbers can't have more than 5 digits

/* input
var x, y;
begin
y := 3;
x := y + 56;
end;
*/

/*
Lexeme Table:
lexeme  token type
var     29
x       2
,       16
...
*/

// Token List:
// 29 2 x 16 2 y 16 20 2 y 19 3 3 16 2 x 19 2 y 4 3 56 17 21 18
// a+b;begin;a,c,c,;

/*
begin
    x := 123456;
end.
*/

// 123456   {Number too long;} -> Error = 1
