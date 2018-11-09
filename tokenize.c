#include "yacc.h"

// Divides the string `p` into tokens and stores them in a token stream
TokenStream *tokenize(char *p) {
    TokenStream *new_token_stream = malloc(sizeof(TokenStream));
    new_token_stream->pos = malloc(sizeof(int));
    *(new_token_stream->pos) = 0;
    Token *tokens = new_token_stream->tokens;

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
            tokens[i].ty = TK_NUM;
            tokens[i].input = p;
            tokens[i].val = strtol(p, &p, 10);
            i++;
            continue;
        }

        // Allow recognizing single lowercase characters as identifiers
        if('a' <= *p && *p <= 'z') {
            tokens[i].ty = TK_IDENT;
            tokens[i].input = p;
            tokens[i].val = *p;
            i++;
            p++;
            continue;
        }

        switch (*p) {
            case '+':
            case '-':
            case '/':
            case '*':
            case ')':
            case '(':
            case '=':
            case ';':
                tokens[i].ty = *p;
                tokens[i].input = p;
                i++;
                p++;
                continue;
            default:
                fprintf(stderr, "Cannot tokenize: %s (Code Point: %d)\n", p, *p);
                exit(TOKENIZE_ERROR);
        }
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;

    return new_token_stream;
}