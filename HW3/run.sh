#!/usr/bin/env bash

gcc -std=c11 -O2 -Wall lex.c -o lex
gcc -std=c11 -O2 -Wall parsercodegen.c -o parsercodegen

./lex __ydg_input.txt
./parsercodegen