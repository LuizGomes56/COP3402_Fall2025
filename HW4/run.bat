@echo off
cargo run -- input.txt > lex_out.txt
gcc parsercodegen_complete.c -o parsercodegen_complete.exe
parsercodegen_complete.exe > parser_out.txt
gcc vm.c -o vm.exe
vm.exe elf.txt
@REM vm.exe elf.txt > vm_out.txt