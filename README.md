# COP3402_Fall2025
System's Software class

### Compile P-Machine assignment
> gcc -O2 -Wall -std=c11 -o vm vm.c

AI PROMPTS USED SO FAR (COMMIT #6) Description = [CAL, SYS, JMP, INC, LIT]
#[CHAT GPT 5 THINKING]

# PROMPT #1
Corrija: println!("{:<width$}L{:<width$} M{:<width$} PC{:<width$}BP{:<width$}SP{:<width$}stack", width = 10);

RAW ANSWER:


Thought for 19s

Você precisa passar um valor para cada {} e aplicar a largura nesses valores
(não no texto literal dentro da string). Duas formas corretas:

```rs
// Com largura nomeada (reutiliza `w` em todos os campos)
println!(
    "{:<w$}{:<w$}{:<w$}{:<w$}{:<w$}{:<w$}",
    "L", "M", "PC", "BP", "SP", "stack",
    w = 10
);
```
ou, mais simples:
```rs
println!("{:<10}{:<10}{:<10}{:<10}{:<10}{:<10}", "L", "M", "PC", "BP", "SP", "stack");
```
Se preferir iterar:
```rs
for col in ["L", "M", "PC", "BP", "SP", "stack"] {
    print!("{:<10}", col);
}
println!();
```

# PROMPT #2
`unsafe extern "C" fn scanf(fmt: *const i8) {}` Como pegar input do console em Rust?

RAW ANSWER:


Thought for 18s
Em Rust, não use scanf. O idiomático é std::io.

1) Ler uma linha e converter
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

2) Vários valores na mesma linha
```rs
use std::io::{self, Read};

fn main() -> io::Result<()> {
    let mut line = String::new();
    io::stdin().read_line(&mut line)?;
    let mut it = line.split_whitespace();

    let a: i64 = it.next().unwrap().parse().unwrap();
    let b: i64 = it.next().unwrap().parse().unwrap();
    println!("{a} {b}");
    Ok(())
}
```

3) Muitas linhas (rápido)
```rs
use std::io::{self, BufRead};

fn main() {
    let stdin = io::stdin();
    for line in stdin.lock().lines() {
        let line = line.unwrap();
        // use `line`
    }
}
```

4) Ler único byte/caractere
```rs
use std::io::{self, Read};

fn main() -> io::Result<()> {
    let mut b = [0u8; 1];
    io::stdin().read_exact(&mut b)?;
    let ch = b[0] as char;
    println!("{ch}");
    Ok(())
}
```

Quer estilo scanf?

Não é recomendado, mas dá para usar crates como scan_fmt/text_io. Exemplo com scan_fmt:
```toml
# Cargo.toml
[dependencies]
scan_fmt = "0.2"
```
```rs
use scan_fmt::scanln_fmt;

fn main() {
    let (a, b): (i32, i32) = scanln_fmt!("{} {}", i32, i32).unwrap();
    println!("{a} {b}");
}
```

Se realmente precisar chamar scanf via FFI (não recomendado):
```rs
use std::ffi::CString;
use libc::{c_char, c_int};

extern "C" { fn scanf(fmt: *const c_char, ...) -> c_int; }

fn main() {
    unsafe {
        let fmt = CString::new("%d").unwrap();
        let mut x: c_int = 0;
        scanf(fmt.as_ptr(), &mut x);
        println!("{x}");
    }
}
```