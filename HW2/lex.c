#include <stdio.h>

// Provided by the assignment
typedef enum {
    skipsym = 1,  // Skip / ignore token
    identsym,     // Identifier
    numbersym,    // Number
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

int main(int argc, char *argv[]) {
    FILE *file_ptr = fopen(argv[1], "r");
    if (!file_ptr) {
        puts("Could not read input file. Maybe path is wrong?");
        return 1;
    }

    int ch;
    while ((ch = fgetc(file_ptr)) != EOF) {
        printf("%c", ch);
    }

    return 0;
}