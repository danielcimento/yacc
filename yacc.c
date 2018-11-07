#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Enumerated token types
enum {
    TK_NUM = 256,   // Integer tokens
    TK_EOF,         // End of input token
};

typedef struct {
    int ty;         // Token type
    int val;        // Value of the token, if a number
    char *input;    // Token's string representation (for error messages)
} Token;

// Divides the string `p` into tokens and stores them in `token_stream`
void tokenize(char *p, Token token_stream[]) {
    int i = 0;
    while (*p) {
        // Skip whitespace
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Read all digits in as base 10 numbers.
        // We also account for the very special case
        // where the first number is negative.
        if (isdigit(*p) || (i == 0 && *p == '-')) {
            token_stream[i].ty = TK_NUM;
            token_stream[i].input = p;
            token_stream[i].val = strtol(p, &p, 10);
            i++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            token_stream[i].ty = *p;
            token_stream[i].input = p;
            i++;
            p++;
            continue;
        }

        fprintf(stderr, "Cannot tokenize: %s\n", p);
        exit(1);
    }

    token_stream[i].ty = TK_EOF;
    token_stream[i].input = p;
}

void error(Token token) {
    fprintf(stderr, "Unexpected token: %s\n", token.input);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "You did not provide the correct number of arguments! (Expected 1)\n");
        return 1;
    }
    
    // Tokenize our input
    Token tokens[100];
    tokenize(argv[1], tokens);

    // Preliminary headers for assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    if (tokens[0].ty != TK_NUM) {
        error(tokens[0]);
    }
    // Load our first token into the register
    printf("\tmov rax, %d\n", tokens[0].val);

    int i = 1;
    while (tokens[i].ty != TK_EOF) {
        if (tokens[i].ty == '+') {
            i++;
            if (tokens[i].ty != TK_NUM) {
                error(tokens[i]);
            }
            printf("\tadd rax, %d\n", tokens[i].val);
            i++;
            continue;
        }

        if (tokens[i].ty == '-') {
            i++;
            if (tokens[i].ty != TK_NUM) {
                error(tokens[i]);
            }
            printf("\tsub rax, %d\n", tokens[i].val);
            i++;
            continue;
        }

        error(tokens[i]);
    }

    printf("\tret\n");
    return 0;
}