@echo off

cargo run -- __ydg_input.txt
gcc -std=c11 -O2 -Wall parsercodegen.c -o parsercodegen
.\parsercodegen.exe > result.txt