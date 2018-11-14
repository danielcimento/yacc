#include "yacc.h"

int main(int argc, char **argv) {
    // First check to see if we're testing. We don't run anything.
    for(int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-test") == 0) {
            if(argc > 2) {
                fprintf(stderr, "You shouldn't pass more arguments/flags when running tests.\n");
            }
            run_test();
            exit(0);
        }
    }

    char *filename = NULL;
    char *string_literal = NULL;
    for(int i = 1; i < argc; i++) {
        // If we have something which isn't a flag or flag argument, it's our file.
        if(argv[i][0] != '-' && argv[i-1][0] != '-') {
            if(string_literal) fprintf(stderr, "You shouldn't use both file input and literal input. Preferring file input.\n");
            filename = argv[i];
        }
        if(strcmp(argv[i],"-l") == 0) {
            string_literal = argv[i+1];
            if(filename) {
                fprintf(stderr, "You shouldn't use both file input and literal input. Preferring file input.\n");
                string_literal = NULL;
            }
        }
    }

    FILE *input_file;
    if(filename) {
        input_file = fopen(filename, "r");
        if(!input_file) {
            fprintf(stderr, "Could not find file specified!\n");
            exit(EXTERNAL_ERROR);
        }
    } else if(string_literal) {
        filename = "yacc_temp.yacc";
        if(access(filename, F_OK) != -1) {
            fprintf(stderr, "Please delete any files named \"yacc_temp.yacc\" when using literal input.\n");
            exit(EXTERNAL_ERROR);
        }
        input_file = fopen(filename, "w");
        fprintf(input_file, "%s", string_literal);
        fclose(input_file);
        input_file = fopen(filename, "r");
    } else {
        fprintf(stderr, "Couldn't understand input. Terminating.\n");
        exit(EXTERNAL_ERROR);
    }

    // Tokenize our input
    Vector *token_stream = tokenize(input_file);
    Node *global_scope_node = parse_code(token_stream);

    // Preliminary headers for assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    Scope *scope = construct_scope_from_token_stream(token_stream);

    gen_scope(global_scope_node, &scope);

    printf("\tret\n");

    if(string_literal) {
        remove(filename);
    }
    return 0;
}