// ! regex header is usually not available on Windows, but it works fine on Eustis
// ! or any other linux based machine

/*
Assignment:
    lex - Lexical Analyzer for PL/0

Authors:
    Luiz Gustavo Santana Dias Gomes

Language: C (only)

To Compile:
    gcc -O2 -std=c11 -o lex lex.c

To Execute (on Eustis):
    ./lex <input file>

Where:
    <input file> is the path to the PL/0 source program

Notes:
    - Implement a lexical analyser for the PL/0 language .
    - The program must detect errors such as
    - numbers longer than five digits
    - identifiers longer than eleven characters
    - invalid characters .
    - The output format must exactly match the specification .
    - Tested on Eustis .

Class:
    COP 3402 - Systems Software - Fall 2025

Instructor:
    Dr. Jie Lin

Due Date:
    Friday, October 3, 2025 at 11:59 PM ET
*/

#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>

// Provided by the assignment
typedef enum {
    skipsym = 1,  // Skip / ignore token
    identsym,     // Identifier [a-zA-Z][a-z-A-Z0-9]*
    numbersym,    // Number [0-9]+
    plussym,      // +
    minussym,     // -
    multsym,      // *
    slashsym,     // /
    eqsym,        // =
    neqsym,       // <>
    lessym,       // <
    leqsym,       // <=
    gtrsym,       // >
    geqsym,       // >=
    lparentsym,   // (
    rparentsym,   // )
    commasym,     // ,
    semicolonsym, // ;
    periodsym,    // .
    becomessym,   // :=
    beginsym,     // begin
    endsym,       // end
    ifsym,        // if
    fisym,        // fi
    thensym,      // then
    whilesym,     // while
    dosym,        // do
    callsym,      // call
    constsym,     // const
    varsym,       // var
    procsym,      // procedure
    writesym,     // write
    readsym,      // read
    elsesym,      // else
    evensym       // even
} TokenType;

#define MAX_TOKENS 10000

// [Source 1](https://www.geeksforgeeks.org/c/regular-expressions-in-c/)
// [Source 2](https://thelinuxcode.com/regular-expression-c/)
// [Source 3](https://www.geeksforgeeks.org/dsa/write-regular-expressions/)
int main(int argc, char *argv[]) {
    regex_t regex;

    if (regcomp(&regex, "^([A-Za-z_][A-Za-z0-9_]*|:=|<=|>=|<>|\\+|\\-|\\*|/|=|<|>|\\(|\\)|,|;|\\.)", 0)) {
        printf("Error compiling keyword regex\n");
        return 1;
    }

    FILE *file_ptr = fopen(argv[1], "r");
    // Try to read input file; argv[1] is the path where input file is located
    if (!file_ptr) {
        puts("Could not read input file. Maybe path is wrong?");
        return 1;
    }

    printf("\n%s\n\n", "Source Program:");

    // Hold all characters that were in the input file
    char program_characters[MAX_TOKENS];

    // Print all the contents in the input file
    int ch;
    int i = 0;
    while ((ch = fgetc(file_ptr)) != EOF) {
        program_characters[i] = ch;
        i++;
        printf("%c", ch);
    }

    // Print lexeme table, space with 10 characters aligned left for each header
    // headers: lexeme, token type
    printf("\n%s\n\n%-10s%-10s\n", "Lexeme Table:", "lexeme", "token type");

    regmatch_t matches[MAX_TOKENS];

    regexec(&regex, program_characters, MAX_TOKENS, matches, 0);

    for (int i = 0; i < MAX_TOKENS; i++) {
        if (matches[i].rm_so != -1) {
            printf("%-10s", &program_characters[matches[i].rm_so]);
            printf("%-10s\n", "identifier");
        } else {
            break;
        }
    }

    return 0;
}