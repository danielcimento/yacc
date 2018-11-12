#include "yacc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "You did not provide the correct number of arguments! (Expected 1)\n");
        return 1;
    }

    if (strcmp(argv[1], "-test") == 0) {
        run_test();
        exit(0);
    }
    
    // Tokenize our input
    Vector *token_stream = tokenize(argv[1]);
    Node *global_scope_node = parse_code(token_stream);

    // Preliminary headers for assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    Scope *scope = construct_scope_from_token_stream(token_stream);

    gen_scope(global_scope_node, &scope, false);

    printf("\tret\n");
    return 0;
}