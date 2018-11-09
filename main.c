#include "yacc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "You did not provide the correct number of arguments! (Expected 1)\n");
        return 1;
    }
    
    // Tokenize our input
    TokenStream *token_stream = tokenize(argv[1]);
    Node **statements = parse_statements(token_stream);

    // Preliminary headers for assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Function prologue:
    printf("\tpush rbp\n");
    printf("\tmov rbp, rsp\n");
    // Right now we reserve space as though we'll use every variable
    printf("\tsub rsp, 208\n");
    
    // Generate every statement we've written
    for(int i = 0; statements[i]; i++) {
        gen(statements[i]);
        // Technically the value should already be in rax, but we should clear the stack anyway.
        printf("\tpop rax\n");
    }

    // Function epilogue:
    printf("\tmov rsp, rbp\n");
    printf("\tpop rbp\n");
    printf("\tret\n");
    return 0;
}