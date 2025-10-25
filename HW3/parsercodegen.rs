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

#[derive(Clone, Copy, Debug, PartialEq)]
enum TokenType {
    Skip = 1,
    Ident,
    Number,
    Plus,
    Minus,
    Mult,
    Slash,
    Mod,
    Eq,
    Neq,
    Les,
    Leq,
    Gtr,
    Geq,
    Even,
    Odd,
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
}

struct Register {
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

impl std::ops::Deref for PCode {
    type Target = Vec<Register>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl std::ops::DerefMut for PCode {
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
        self.find_index_or_throw(&name, 3)?;
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
}

#[derive(PartialEq)]
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

#[derive(Clone, PartialEq)]
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

    fn get_curr_token(&self) -> &Token {
        self.get_token(self.iteration).unwrap()
    }

    fn get_curr_kind(&self) -> TokenType {
        self.get_curr_token().kind
    }

    fn program(&mut self) -> MayFail {
        match self.tokens.ends_with(&[TokenType::Period.into()]) {
            false => Err(throw(1).into()),
            true => self.block(),
        }
    }

    fn block(&mut self) -> MayFail {
        self.const_declaration()?;
        let number_of_vars = self.var_declaration()?;
        self.emit(Instruction::INC, number_of_vars + 3);
        self.statement()
    }

    fn const_declaration(&mut self) -> MayFail {
        if self.get_curr_kind() == TokenType::Const {
            self.iteration += 1;

            while self.get_curr_kind() == TokenType::Comma {
                let next_token = self.get_token(self.iteration + 1)?;

                if next_token != TokenType::Ident {
                    return Err(throw(2).into());
                }

                let ident_name = next_token.get_meta()?;

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

    fn var_declaration(&mut self) -> MayFail<usize> {
        let mut count = 0;

        if self.get_curr_kind() == TokenType::Var {
            self.iteration += 1;

            while self.get_curr_kind() == TokenType::Comma {
                count += 1;
                let next_token = self.get_token(self.iteration + 1)?;

                if next_token != TokenType::Ident {
                    return Err(throw(2).into());
                }

                let ident_name = next_token.get_meta()?;

                if self.symbol_table.has_symbol(ident_name) {
                    return Err(throw(3).into());
                }

                self.symbol_table.push_var(ident_name.clone())?;
                self.iteration += 2;
            }

            if self.get_curr_token() != TokenType::Semicolon {
                return Err(throw(6).into());
            }
        }

        Ok(count)
    }

    fn statement(&mut self) -> MayFail {
        let current_token = self.get_curr_kind();

        match current_token {
            TokenType::Ident => {
                let symbol_index = self
                    .symbol_table
                    .find_index_or_throw(self.get_curr_token().get_meta()?, 7)?;

                if self.symbol_table.get_index(symbol_index)?.kind != SymbolKind::Var {
                    return Err(throw(8).into());
                }

                if self.get_token(self.iteration + 1)?.kind != TokenType::Becomes {
                    return Err(throw(9).into());
                }

                self.iteration += 2;
                self.expression()?;

                self.emit(
                    Instruction::STO,
                    self.symbol_table.get_index(symbol_index)?.addr,
                );
            }
            TokenType::Begin => {
                self.iteration += 1;

                while self.get_curr_kind() == TokenType::Semicolon {
                    self.iteration += 1;
                    self.statement()?;
                }

                if self.get_curr_kind() != TokenType::End {
                    return Err(throw(10).into());
                }

                self.iteration += 1;
            }
            TokenType::While => {
                self.iteration += 1;

                let loop_index = self.iteration;
                self.condition()?;

                if self.get_curr_token() != TokenType::Do {
                    return Err(throw(12).into());
                }

                self.iteration += 1;
                let jpc_index = self.iteration;

                self.emit(Instruction::JPC, usize::MAX);
                self.statement()?;
                self.emit(Instruction::JMP, loop_index);

                self.pcode[jpc_index].M = self.iteration;
            }
            TokenType::Read => {
                self.iteration += 1;

                if self.get_curr_token() != TokenType::Ident {
                    return Err(throw(2).into());
                }

                let symbol_index = self
                    .symbol_table
                    .find_index_or_throw(self.get_curr_token().get_meta()?, 7)?;

                if self.symbol_table.get_index(symbol_index)?.kind != SymbolKind::Var {
                    return Err(throw(8).into());
                }

                self.iteration += 1;
                self.emit(Instruction::READ, usize::MAX);
                self.emit(
                    Instruction::STO,
                    self.symbol_table.get_index(symbol_index)?.addr,
                );
            }
            TokenType::Write => {
                self.iteration += 1;
                self.expression()?;
                self.emit(Instruction::WRITE, usize::MAX);
            }
            _ => unreachable!(
                "Statement received an unknown token type: TokenType::{current_token:?}"
            ),
        };

        Ok(())
    }

    fn condition(&mut self) -> MayFail {
        let kind = self.get_curr_kind();

        if kind != TokenType::Odd {
            self.expression()?;
        }

        macro_rules! assert_kind {
            ($(($token:ident, $instr:ident)),*) => {
                match kind {
                    $(
                        TokenType::$token => {
                            self.iteration += 1;
                            self.expression()?;
                            self.emit(Instruction::$instr, usize::MAX);
                            Ok(())
                        }
                    )*
                    _ => Err(throw(13).into())
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

    fn expression(&mut self) -> MayFail {
        macro_rules! match_kind {
            ($kind:expr) => {
                while matches!($kind, TokenType::Minus | TokenType::Plus) {
                    match $kind {
                        TokenType::Minus => {
                            self.iteration += 1;
                            self.term()?;
                            self.emit(Instruction::SUB, 0);
                        }
                        TokenType::Plus => {
                            self.iteration += 1;
                            self.term()?;
                            self.emit(Instruction::ADD, 0);
                        }
                        _ => unreachable!(),
                    }
                }
            };
        }

        match self.get_curr_kind() {
            TokenType::Minus => {
                self.iteration += 1;
                self.term()?;
                self.emit(Instruction::NEG, 0);
                let kind = self.get_curr_kind();
                match_kind!(kind);
            }
            kind => {
                if kind == TokenType::Plus {
                    self.iteration += 1;
                }
                self.term()?;
                match_kind!(kind);
            }
        }
        Ok(())
    }

    fn term(&mut self) -> MayFail {
        self.factor()?;

        let kind = self.get_curr_kind();

        macro_rules! match_kind {
            ($(($token:ident, $instr:ident)),*) => {
                while matches!(kind, $(TokenType::$token)|*) {
                    match kind {
                        $(
                            TokenType::$token => {
                                self.iteration += 1;
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
        match self.get_curr_kind() {
            TokenType::Ident => {
                let symbol_index = self
                    .symbol_table
                    .find_index_or_throw(self.get_curr_token().get_meta()?, 7)?;

                if self.symbol_table.get_index(symbol_index)?.kind == SymbolKind::Const {
                    self.emit(
                        Instruction::LIT,
                        self.symbol_table.get_index(symbol_index)?.value,
                    );
                } else {
                    self.emit(
                        Instruction::LOD,
                        self.symbol_table.get_index(symbol_index)?.addr,
                    );
                }

                self.iteration += 1;
            }
            TokenType::Number => {
                self.emit(Instruction::LIT, usize::MAX);
            }
            TokenType::Lparent => {
                self.iteration += 1;
                self.expression()?;

                if self.get_curr_kind() != TokenType::Rparent {
                    return Err(throw(14).into());
                }

                self.iteration += 1;
            }
            _ => return Err(throw(15).into()),
        }
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

    let token_stream = TokenStream::from(
        file.lines()
            .into_iter()
            .map(Token::from)
            .collect::<Vec<_>>(),
    );

    println!(
        "Assembly Code:\n\n{:>4}{:>5}{:>3}{:>3}",
        "Line", "OP", "L", "M"
    );

    token_stream.pcode.finish()
}
