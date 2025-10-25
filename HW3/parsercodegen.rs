#![allow(non_snake_case)]

/*
<program> ::= <block> "."
<block> ::= <const-declaration> <var-declaration> <statement>
<const-declaration> ::= [ "const" <ident> "=" <number>
{"," <ident> "=" <number>} ";"]
<var-declaration> ::= [ "var" <ident> {"," <ident>} ";"]
<statement> ::=
[
<ident> ":=" <expression>
| "begin" <statement> { ";" <statement> } "end"
| "if" <condition> "then" <statement> "fi"
| "while" <condition> "do" <statement>
| "read" <ident>
| "write" <expression>
| empty
]
<condition> ::= "even" <expression>
| <expression> <rel-op> <expression>
<expression> ::= <term> { ("+" | "-") <term> }
<term> ::= <factor> { ("*" | "/" ) <factor> }
<factor> ::=
<ident>
| <number>
| "(" <expression> ")"
<rel-op> ::= "=" | "<>" | "<" | "<=" | ">" | ">="
<number> ::= <digit> { <digit> }
<ident> ::= <letter> { <letter> | <digit> }
<digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
<letter> ::= "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"
*/

use std::fmt::Display;

fn throw(code: i32) -> &'static str {
    match code {
        1 => "Error: program must end with period",
        2 => "Error: const, var, and read keywords must be followed by identifier",
        3 => "Error: symbol name has already been declared",
        4 => "Error: constants must be assigned with =",
        5 => "Error: constants must be assigned an integer value",
        6 => "Error: constant and variable declarations must be followed by a semicolon",
        7 => "Error: undeclared identifier",
        8 => "Error: only variable values may be altered",
        9 => "Error: assignment statements must use :=",
        10 => "Error: begin must be followed by end",
        11 => "Error: if must be followed by then",
        12 => "Error: while must be followed by do",
        13 => "Error: condition must contain comparison operator",
        14 => "Error: right parenthesis must follow left parenthesis",
        15 => "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols",
        _ => unreachable!("Unknown error kind"),
    }
}

#[derive(Clone, Copy, PartialEq)]
pub enum TokenType {
    Skip = 1,
    Ident,
    Number,
    Plus,
    Minus,
    Mult,
    Slash,
    Eq,
    Neq,
    Les,
    Leq,
    Gtr,
    Geq,
    Lparent,
    Rparent,
    Comma,
    Semicolon,
    Period,
    Becomes,
    Begin,
    End,
    If,
    Fi,
    Then,
    While,
    Do,
    Call,
    Const,
    Var,
    Proc,
    Write,
    Read,
    Else,
    Even,
}

pub struct Register {
    /// The operation code specifying the instruction to execute
    /// (LIT, OPR, LOD, STO, CAL, INC, JMP, JPC, SYS).
    OP: Instruction,

    /// The lexicographical level for instructions that access variables in other activation records.
    L: usize,

    /// A parameter whose meaning depends on the opcode. It may be a literal value, an
    /// address in the text segment, an offset within an activation record or a sub-opcode for
    /// arithmetic and logical operations
    M: usize,
}

#[derive(Debug)]
pub enum Instruction {
    LIT,
    RTN,
    ADD,
    SUB,
    MUL,
    DIV,
    EQL,
    NEG,
    LSS,
    LEQ,
    GTR,
    GEQ,
    EVEN,
    LOD,
    STO,
    CAL,
    INC,
    JMP,
    JPC,
    SYS,
}

pub struct PCode(Vec<Register>);

impl PCode {
    pub fn new() -> Self {
        Self(vec![Register {
            OP: Instruction::JMP,
            L: 0,
            M: 3,
        }])
    }

    pub fn print(&self) {
        println!("{self}");
    }

    pub fn finish(self) -> MayFail {
        let mut contents = String::new();
        for register in self.0 {
            contents.push_str(&format!(
                "{OP} {L} {M}\n",
                OP = register.OP as usize,
                L = register.L,
                M = register.M
            ));
        }
        Ok(std::fs::write("elf.txt", contents)?)
    }
}

impl Display for PCode {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut contents = String::new();
        for (index, register) in self.0.iter().enumerate() {
            contents.push_str(&format!(
                "{index:>4}{OP:>5?}{L:>1}{M:>2}\n",
                OP = register.OP,
                L = register.L,
                M = register.M
            ));
        }
        write!(f, "{contents}")
    }
}

type MayFail<T = ()> = Result<T, Box<dyn std::error::Error>>;

pub struct SymbolTable {
    table: Vec<Symbol>,
    addr: i32,
}

impl SymbolTable {
    pub fn new() -> Self {
        Self {
            table: Vec::new(),
            addr: 3,
        }
    }

    fn has_symbol(&self, name: &str) -> bool {
        self.table.iter().any(|symbol| symbol.name == name)
    }

    fn get_symbol(&self, name: &str) -> MayFail<(usize, &Symbol)> {
        Ok(self
            .table
            .iter()
            .enumerate()
            .find(|(_, symbol)| symbol.name == name)
            .ok_or(throw(3))?)
    }

    fn push_symbol(&mut self, symbol: Symbol) {
        self.table.push(symbol);
        self.addr += 3;
    }

    pub fn push_const(&mut self, name: String, value: i32) -> MayFail<usize> {
        self.get_symbol(&name)?;
        let current_offset = self.table.len();
        self.push_symbol(Symbol {
            kind: SymbolKind::Const,
            name,
            value,
            level: 0,
            addr: 0,
            mark: false,
        });
        Ok(current_offset + 1)
    }

    pub fn push_var(&mut self, name: String) -> MayFail {
        self.get_symbol(&name)?;
        Ok(self.push_symbol(Symbol {
            kind: SymbolKind::Var,
            name,
            addr: self.addr,
            value: 0,
            level: 0,
            mark: false,
        }))
    }
}

pub enum SymbolKind {
    Const,
    Var,
    Proc,
}

pub struct Symbol {
    kind: SymbolKind,
    name: String,
    value: i32,
    level: i32,
    addr: i32,
    mark: bool,
}

#[derive(Clone, PartialEq)]
pub struct Token {
    kind: TokenType,
    meta: Option<String>,
}

impl Token {
    pub fn is_valid_number(&self) -> bool {
        self.kind == TokenType::Number
            && match self.meta {
                Some(ref s) => s.as_str() != "Number too long",
                _ => false,
            }
    }

    pub fn get_number(&self) -> MayFail<i32> {
        if !self.is_valid_number() {
            Err("Called get_number() in a Token that is not TokenType::Number".into())
        } else {
            Ok(self.meta.as_ref().unwrap().parse()?)
        }
    }
}

impl From<TokenType> for Token {
    fn from(value: TokenType) -> Self {
        Self {
            kind: value,
            meta: None,
        }
    }
}

impl Into<TokenType> for Token {
    fn into(self) -> TokenType {
        self.kind
    }
}

impl From<&str> for Token {
    fn from(value: &str) -> Self {
        let mut kind = TokenType::Skip;
        let mut meta = None;
        let parts = value.trim().split(" ");
        for (index, part) in parts.enumerate() {
            match index {
                0 => kind = unsafe { std::mem::transmute(part.parse::<u8>().unwrap()) },
                1 => meta = Some(part.to_string()),
                _ => unreachable!("More than two tokens found in a Token line"),
            }
        }
        Self { kind, meta }
    }
}

pub struct TokenStream {
    iteration: usize,
    tokens: Vec<Token>,
    symbol_table: SymbolTable,
}

impl std::ops::Deref for SymbolTable {
    type Target = Vec<Symbol>;

    fn deref(&self) -> &Self::Target {
        &self.table
    }
}

impl std::ops::DerefMut for SymbolTable {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.table
    }
}

impl TokenStream {
    fn get_token(&self, iteration: usize) -> MayFail<&Token> {
        assert!(iteration < self.tokens.len());
        self.tokens
            .get(iteration)
            .ok_or(format!("Index {iteration} does not exist in TokenStream").into())
    }

    fn get_curr_token(&self) -> &Token {
        self.get_token(self.iteration).unwrap()
    }

    fn program(&mut self) -> MayFail {
        match self.tokens.ends_with(&[TokenType::Period.into()]) {
            false => Err(throw(1).into()),
            true => self.block(),
        }
    }

    fn block(&mut self) -> MayFail {
        self.const_declaration()?;
        self.var_declaration()?;
        self.statement()
    }
    fn const_declaration(&mut self) -> MayFail {
        if self.get_curr_token().kind == TokenType::Const {
            while self.get_curr_token().kind == TokenType::Comma {
                let next_token = self.get_token(self.iteration + 1)?;

                if next_token != TokenType::Ident {
                    return Err(throw(2).into());
                }

                let ident_name = next_token
                    .meta
                    .as_ref()
                    .ok_or("Found a TokenType::Ident without metadata")?;

                if self.symbol_table.has_symbol(ident_name) {
                    return Err(throw(3).into());
                }

                let ident_offset = self.symbol_table.push_const(ident_name.clone(), 0)?;

                if self.get_token(self.iteration + 2)? != TokenType::Eq {
                    return Err(throw(4).into());
                }

                let ident_value = self.get_token(self.iteration + 3)?.clone();

                if &ident_value != TokenType::Number {
                    return Err(throw(5).into());
                }

                let symbol = self.symbol_table.get_mut(ident_offset).ok_or(format!(
                    "Attempted to get invalid index {ident_offset} from SymbolTable"
                ))?;

                symbol.value = ident_value.get_number()?;

                self.iteration += 4;
            }
            if self.get_curr_token() != TokenType::Semicolon {
                return Err(throw(6).into());
            }
            self.iteration += 1;
        }
        Ok(())
    }
    fn var_declaration(&mut self) -> MayFail {
        Ok(())
    }
    fn statement(&mut self) -> MayFail {
        Ok(())
    }
    fn condition(&mut self) -> MayFail {
        Ok(())
    }
    fn expression(&mut self) -> MayFail {
        Ok(())
    }
    fn term(&mut self) -> MayFail {
        Ok(())
    }
    fn factor(&mut self) -> MayFail {
        Ok(())
    }
    fn rel_op(&mut self) -> MayFail {
        Ok(())
    }
    fn number(&mut self) -> MayFail {
        Ok(())
    }
    fn ident(&mut self) -> MayFail {
        Ok(())
    }
    fn digit(&mut self) -> MayFail {
        Ok(())
    }
    fn letter(&mut self) -> MayFail {
        Ok(())
    }
}

impl From<Vec<Token>> for TokenStream {
    fn from(value: Vec<Token>) -> Self {
        Self {
            iteration: 0,
            tokens: value,
            symbol_table: SymbolTable::new(),
        }
    }
}

impl PartialEq<TokenType> for &Token {
    fn eq(&self, other: &TokenType) -> bool {
        self.kind == *other
    }
}

fn main() -> MayFail {
    let file = std::fs::read_to_string("tokens.txt")?;

    let tokens = TokenStream::from(
        file.lines()
            .into_iter()
            .map(Token::from)
            .collect::<Vec<_>>(),
    );

    let mut pcode = PCode::new();

    println!(
        "Assembly Code:\n\n{:>4}{:>5}{:>3}{:>3}",
        "Line", "OP", "L", "M"
    );

    pcode.finish()
}
