#![allow(non_snake_case)]

use std::{
    fmt::Display,
    ops::{Deref, DerefMut},
};

macro_rules! throw {
    ($code:expr) => {{
        // panic!("{}", throw($code));
        MayFail::Err(throw($code).into())
    }};
}

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

#[derive(Clone, Copy, Debug, PartialEq)]
enum TokenType {
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
    Odd,
    Mod,
}

struct Register {
    OP: Instruction,
    L: usize,
    M: usize,
}

#[derive(Debug)]
enum Instruction {
    LIT,
    RTN,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    EQL,
    NEG,
    LSS,
    LEQ,
    GTR,
    GEQ,
    EVEN,
    ODD,
    LOD,
    STO,
    CAL,
    INC,
    JMP,
    JPC,
    SYS,
    READ,
    WRITE,
}

struct PCode(Vec<Register>);

impl Deref for PCode {
    type Target = Vec<Register>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for PCode {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl PCode {
    fn new() -> Self {
        Self(vec![Register {
            OP: Instruction::JMP,
            L: 0,
            M: 3,
        }])
    }

    fn print(&self) {
        println!("{self}");
    }

    fn finish(self) -> MayFail {
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

struct SymbolTable {
    table: Vec<Symbol>,
    addr: usize,
}

impl SymbolTable {
    fn new() -> Self {
        Self {
            table: Vec::new(),
            addr: 3,
        }
    }

    pub fn print(&self) {
        println!(
            "Symbol Table: \n\n{kind:<6}|{name:<14}|{value:<6}|{level:<6}|{addr:<8}|{mark:<5}\n",
            kind = "Kind",
            name = "Name",
            value = "Value",
            level = "Level",
            addr = "Address",
            mark = "Mark"
        );
        for _ in 0..51 {
            print!("-");
        }
        for symbol in &self.table {
            println!(
                "{kind:<6}|{name:<14}|{value:<6}|{level:<6}|{addr:<8}|{mark:<5}\n",
                kind = symbol.kind as usize,
                name = symbol.name,
                value = symbol.value,
                level = symbol.level,
                addr = symbol.addr,
                mark = symbol.mark
            );
        }
    }

    fn has_symbol(&self, name: &str) -> bool {
        self.table.iter().any(|symbol| symbol.name == name)
    }

    fn get_index(&self, index: usize) -> MayFail<&Symbol> {
        self.table
            .get(index)
            .ok_or(format!("Index {index} does not exist in SymbolTable").into())
    }

    fn find_index_or_throw(&self, name: &str, code: i32) -> MayFail<usize> {
        Ok(self
            .table
            .iter()
            .enumerate()
            .find(|(_, symbol)| symbol.name == name)
            .map(|(index, _)| index)
            .ok_or(throw(code))?)
    }

    fn push_symbol(&mut self, symbol: Symbol) {
        self.table.push(symbol);
        self.addr += 3;
    }

    fn push_const(&mut self, name: String, value: usize) -> MayFail<usize> {
        self.find_index_or_throw(&name, 3)?;
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

    // In C translation, this function will never throw an error.
    fn push_var(&mut self, name: String) -> MayFail<usize> {
        match self.find_index_or_throw(&name, 3) {
            Err(_) => {
                let current_offset = self.table.len();
                self.push_symbol(Symbol {
                    kind: SymbolKind::Var,
                    name,
                    addr: self.addr,
                    value: 0,
                    level: 0,
                    mark: false,
                });
                self.addr += 2;
                Ok(current_offset + 1)
            }
            Ok(_) => throw!(3),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum SymbolKind {
    Const,
    Var,
    Proc,
}

struct Symbol {
    kind: SymbolKind,
    name: String,
    value: usize,
    level: usize,
    addr: usize,
    mark: bool,
}

#[derive(Clone, Debug, PartialEq)]
struct Token {
    kind: TokenType,
    meta: Option<String>,
}

impl Token {
    fn is_valid_number(&self) -> bool {
        self.kind == TokenType::Number
            && match self.meta {
                Some(ref s) => s.as_str() != "Number too long",
                _ => false,
            }
    }

    fn get_number(&self) -> MayFail<usize> {
        if !self.is_valid_number() {
            Err("Called get_number() in a Token that is not TokenType::Number".into())
        } else {
            Ok(self.meta.as_ref().unwrap().parse()?)
        }
    }

    fn get_meta(&self) -> MayFail<&String> {
        Ok(self.meta.as_ref().ok_or(format!(
            "Found a TokenType::{:?} without metadata",
            self.kind
        ))?)
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
            if part.len() > 11 {
                continue;
            }

            match index {
                0 => kind = unsafe { std::mem::transmute(part.parse::<u8>().unwrap()) },
                1 => meta = Some(part.to_string()),
                _ => unreachable!("More than two tokens found in a Token line"),
            }
        }
        Self { kind, meta }
    }
}

/// Every field in this struct will be transformed in a static mut.
struct TokenStream {
    iteration: usize,
    tokens: Vec<Token>,
    symbol_table: SymbolTable,
    pcode: PCode,
}

impl Deref for SymbolTable {
    type Target = Vec<Symbol>;

    fn deref(&self) -> &Self::Target {
        &self.table
    }
}

impl DerefMut for SymbolTable {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.table
    }
}

impl TokenStream {
    fn emit(&mut self, instruction: Instruction, M: usize) {
        self.pcode.push(Register {
            OP: instruction,
            L: 0,
            M,
        })
    }

    fn get_token(&self, iteration: usize) -> MayFail<&Token> {
        assert!(iteration < self.tokens.len());
        self.tokens
            .get(iteration)
            .ok_or(format!("Index {iteration} does not exist in TokenStream").into())
    }

    fn token(&self) -> &Token {
        if self.iteration == self.tokens.len() {
            panic!("TokenStream is out of tokens");
        }
        self.get_token(self.iteration).unwrap()
    }

    fn kind(&self) -> TokenType {
        self.token().kind
    }

    fn program(&mut self) -> MayFail {
        match self.tokens.ends_with(&[TokenType::Period.into()]) {
            false => throw!(1),
            true => self.block(),
        }
    }

    fn next(&mut self) -> TokenType {
        self.iteration += 1;
        self.kind()
    }

    fn block(&mut self) -> MayFail {
        self.const_declaration()?;
        let number_of_vars = self.var_declaration()?;
        self.emit(Instruction::INC, number_of_vars + 3);
        self.statement()
    }

    fn const_declaration(&mut self) -> MayFail {
        if self.token() == TokenType::Const {
            loop {
                let token = self.next();

                if token != TokenType::Ident {
                    return throw!(2);
                }

                let ident_name = self.token().get_meta()?;

                if self.symbol_table.has_symbol(ident_name) {
                    return throw!(3);
                }

                let ident_offset = self.symbol_table.push_const(ident_name.clone(), 0)?;
                let token = self.next();

                if token != TokenType::Eq {
                    return throw!(4);
                }

                let token = self.next();

                if token != TokenType::Number {
                    return throw!(5);
                }

                let ident_value = self.token().get_number()?;
                self.symbol_table[ident_offset].value = ident_value;

                self.next();

                if self.token() != TokenType::Comma {
                    break;
                }
            }

            if self.token() != TokenType::Semicolon {
                return throw!(6);
            }

            self.next();
        }
        Ok(())
    }

    fn var_declaration(&mut self) -> MayFail<usize> {
        let mut count = 0;

        if self.token() == TokenType::Var {
            loop {
                count += 1;

                let token = self.next();

                if token != TokenType::Ident {
                    return throw!(2);
                }

                let ident_name = self.token().get_meta()?;

                if self.symbol_table.has_symbol(ident_name) {
                    return throw!(3);
                }

                self.symbol_table.push_var(ident_name.clone())?;
                self.next();

                if self.token() != TokenType::Comma {
                    break;
                }
            }

            if self.token() != TokenType::Semicolon {
                return throw!(6);
            }
        }

        self.next();

        Ok(count)
    }

    fn statement(&mut self) -> MayFail {
        match self.kind() {
            TokenType::Ident => {
                let symbol_index = self
                    .symbol_table
                    .find_index_or_throw(self.token().get_meta()?, 7)?;

                if self.symbol_table.get_index(symbol_index)?.kind != SymbolKind::Var {
                    return throw!(8);
                }

                let token = self.next();

                if token != TokenType::Becomes {
                    return throw!(9);
                }

                self.next();
                self.expression()?;
                self.emit(
                    Instruction::STO,
                    self.symbol_table.get_index(symbol_index)?.addr,
                );
            }
            TokenType::Begin => {
                loop {
                    self.next();
                    self.statement()?;
                    if self.token() != TokenType::Semicolon {
                        break;
                    }
                }

                if self.token() != TokenType::End {
                    return throw!(10);
                }

                self.next();
            }
            TokenType::If => {
                self.next();
                self.condition()?;

                let jpc_index = self.iteration;
                self.emit(Instruction::JPC, 0);

                if self.token() != TokenType::Then {
                    return throw!(11);
                }

                self.next();
                self.statement()?;
                self.pcode[jpc_index].M = self.iteration;
            }
            TokenType::While => {
                self.next();

                let loop_index = self.iteration;
                self.condition()?;

                if self.token() != TokenType::Do {
                    return throw!(12);
                }

                self.next();

                let jpc_index = self.iteration;

                self.emit(Instruction::JPC, usize::MAX);
                self.statement()?;
                self.emit(Instruction::JMP, loop_index);

                self.pcode[jpc_index].M = self.iteration;
            }
            TokenType::Read => {
                self.next();

                if self.token() != TokenType::Ident {
                    return throw!(2);
                }

                let symbol_index = self
                    .symbol_table
                    .find_index_or_throw(self.token().get_meta()?, 7)?;

                if self.symbol_table.get_index(symbol_index)?.kind != SymbolKind::Var {
                    return throw!(8);
                }

                self.next();
                self.emit(Instruction::READ, usize::MAX);
                self.emit(
                    Instruction::STO,
                    self.symbol_table.get_index(symbol_index)?.addr,
                );
            }
            TokenType::Write => {
                self.next();
                self.expression()?;
                self.emit(Instruction::WRITE, usize::MAX);
            }
            token => unreachable!("Statement received an unknown token type: TokenType::{token:?}"),
        };

        Ok(())
    }

    fn condition(&mut self) -> MayFail {
        if self.token() != TokenType::Odd {
            self.next();
            self.expression()?;
            self.emit(Instruction::ODD, 0);
            Ok(())
        } else {
            self.expression()?;

            macro_rules! assert_kind {
                ($(($token:ident, $instr:ident)),*) => {
                    match self.kind() {
                        $(
                            TokenType::$token => {
                                self.next();
                                self.expression()?;
                                self.emit(Instruction::$instr, usize::MAX);
                                Ok(())
                            }
                        )*
                        _ => throw!(15)
                    }
                };
            }

            assert_kind![
                (Odd, ODD),
                (Eq, EQL),
                (Neq, NEG),
                (Les, LSS),
                (Leq, LEQ),
                (Gtr, GTR),
                (Geq, GEQ)
            ]
        }
    }

    fn expression(&mut self) -> MayFail {
        macro_rules! match_kind {
            ($kind:expr) => {
                while matches!($kind, TokenType::Minus | TokenType::Plus) {
                    match $kind {
                        TokenType::Minus => {
                            self.next();
                            self.term()?;
                            self.emit(Instruction::SUB, 0);
                        }
                        TokenType::Plus => {
                            self.next();
                            self.term()?;
                            self.emit(Instruction::ADD, 0);
                        }
                        _ => unreachable!(),
                    }
                }
            };
        }

        match self.kind() {
            TokenType::Minus => {
                self.next();
                self.term()?;
                self.emit(Instruction::NEG, 0);
                let kind = self.kind();
                match_kind!(kind);
            }
            kind => {
                if kind == TokenType::Plus {
                    self.next();
                }
                self.term()?;
                match_kind!(kind);
            }
        }
        Ok(())
    }

    fn term(&mut self) -> MayFail {
        self.factor()?;

        let kind = self.kind();

        macro_rules! match_kind {
            ($(($token:ident, $instr:ident)),*) => {
                while matches!(kind, $(TokenType::$token)|*) {
                    match kind {
                        $(
                            TokenType::$token => {
                                self.next();
                                self.factor()?;
                                self.emit(Instruction::$instr, 0);
                            }
                        )*
                        _ => unreachable!(),
                    }
                }
            };
        }

        Ok(match_kind![(Mult, MUL), (Slash, DIV), (Mod, MOD)])
    }

    fn factor(&mut self) -> MayFail {
        match self.kind() {
            TokenType::Ident => {
                let symbol_index = self
                    .symbol_table
                    .find_index_or_throw(self.token().get_meta()?, 7)?;

                let symbol = self.symbol_table.get_index(symbol_index)?;

                if symbol.kind == SymbolKind::Const {
                    self.emit(Instruction::LIT, symbol.value);
                } else {
                    self.emit(Instruction::LOD, symbol.addr);
                }

                self.next();
            }
            TokenType::Number => {
                self.emit(Instruction::LIT, usize::MAX);
                self.next();
            }
            TokenType::Lparent => {
                self.next();
                self.expression()?;

                if self.kind() != TokenType::Rparent {
                    return throw!(14);
                }

                self.next();
            }
            _ => return throw!(15),
        }

        Ok(())
    }
}

impl From<Vec<Token>> for TokenStream {
    fn from(value: Vec<Token>) -> Self {
        Self {
            iteration: 0,
            tokens: value,
            symbol_table: SymbolTable::new(),
            pcode: PCode::new(),
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

    let tokens = file
        .lines()
        .into_iter()
        .filter(|line| !line.is_empty() && !line.chars().all(char::is_whitespace))
        .map(Token::from)
        .collect::<Vec<_>>();

    println!("Tokens: {tokens:#?}");

    let mut token_stream = TokenStream::from(tokens);

    token_stream.program().unwrap();

    println!(
        "Assembly Code:\n\n{:>4}{:>5}{:>3}{:>3}",
        "Line", "OP", "L", "M"
    );

    token_stream.pcode.print();
    token_stream.symbol_table.print();

    token_stream.pcode.finish()
}
