#include "yacc.h"
#include <stdbool.h>

Token *new_token(int type, char *input, int val, char *name) {
    Token *tk = malloc(sizeof(Token));
    tk->ty = type;
    tk->input = input;
    tk->val = val;
    tk->name = name;
    return tk;
}

bool is_identifier_character(char c) {
    return isalpha(c) || isdigit(c) || c == '_';
}

// Divides the string `p` into tokens and stores them in a token stream
Vector *tokenize(char *p) {
    Vector *tokens = new_vector();

    while (*p) {
        // Skip whitespace
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Read all digits in as base 10 numbers.
        // We also account for the very special case
        // where the first number is negative.
        if (isdigit(*p)) {
            vec_push(tokens, new_token(TK_NUM, p, strtol(p, &p, 10), NULL));
            continue;
        }

        // Allow recognizing single lowercase characters as identifiers
        if(isalpha(*p) || *p == '_') {
            int i = 0;
            char *identifier_name = malloc(128 * sizeof(char));
            while(is_identifier_character(*p)) {
                identifier_name[i++] = *(p++);
            }
            identifier_name[i] = 0;
            vec_push(tokens, new_token(TK_IDENT, p, *p, identifier_name));
            continue;
        }

        switch (*p) {
            case '=':
                if(*(p + 1) == '=') {
                    vec_push(tokens, new_token(TK_EQUAL, p, 0, NULL));
                    p += 2;
                    continue;
                }
            case '!':
                if(*(p + 1) == '=') {
                    vec_push(tokens, new_token(TK_NEQUAL, p, 0, NULL));
                    p += 2;
                    continue;
                }
            case '>':
                if(*(p + 1) == '=') {
                    vec_push(tokens, new_token(TK_GEQUAL, p, 0, NULL));
                    p += 2;
                    continue;
                }
            case '<':
                if(*(p + 1) == '=') {
                    vec_push(tokens, new_token(TK_LEQUAL, p, 0, NULL));
                    p += 2;
                    continue;
                }
            case '-':
            case '+':
            case '/':
            case '*':
            case ')':
            case '(':
            case ';':
            case '~':
            case '%':
                vec_push(tokens, new_token(*p, p, 0, NULL));
                p++;
                continue;
            default:
                fprintf(stderr, "Cannot tokenize: %s (Code Point: %d)\n", p, *p);
                exit(TOKENIZE_ERROR);
        }
    }

    vec_push(tokens, new_token(TK_EOF, p, 0, NULL));

    return tokens;
}