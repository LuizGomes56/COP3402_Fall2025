mod lex;
mod parsercodegen;
mod vm;

type MayFail<T = ()> = Result<T, Box<dyn std::error::Error>>;

fn main() -> MayFail {
    lex::lex();
    Ok(())
    // parsercodegen::parsercodegen()
}
