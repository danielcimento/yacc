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
    Vector *statements = parse_statements(token_stream);

    // Preliminary headers for assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Function prologue:
    printf("\tpush rbp\n");
    printf("\tmov rbp, rsp\n");
    
    // Calculate all the space we need for variables
    Map *local_variables = new_map(NULL);
    // For every token, if it's an identifier we haven't seen yet, it gets the address 8n bits away, 
    // where n is the number of identifiers we already had
    for(int i = 0; i < token_stream->len; i++) {
        Token *current_token = (Token *)token_stream->data[i];
        if(current_token->ty == TK_IDENT) {
            if(map_get(local_variables, current_token->name) == NULL) {
                map_put(local_variables, current_token->name, (void *)(long)(local_variables->keys->len * 8));
            }
        }
    }
    printf("\tsub rsp, %d\n", local_variables->keys->len * 8);
    
    // Generate every statement we've written
    for(int i = 0; i < statements->len; i++) {
        gen((Node *)statements->data[i], local_variables);
        // Technically the value should already be in rax, but we should clear the stack anyway.
        printf("\tpop rax\n");
    }

    // Function epilogue:
    printf("\tmov rsp, rbp\n");
    printf("\tpop rbp\n");
    printf("\tret\n");
    return 0;
}