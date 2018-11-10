#include "yacc.h"
#include <stdbool.h>

Token *new_token(int type, char *input, int val) {
    Token *tk = malloc(sizeof(Token));
    tk->ty = type;
    tk->input = input;
    tk->val = val;
    return tk;
}

// Divides the string `p` into tokens and stores them in a token stream
Vector *tokenize(char *p) {
    Vector *tokens = new_vector();
    bool can_be_negative = true;

    while (*p) {
        // Skip whitespace
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Read all digits in as base 10 numbers.
        // We also account for the very special case
        // where the first number is negative.
        if (isdigit(*p) || (can_be_negative && *p == '-')) {
            vec_push(tokens, new_token(TK_NUM, p, strtol(p, &p, 10)));
            can_be_negative = false;
            continue;
        }

        // Allow recognizing single lowercase characters as identifiers
        if('a' <= *p && *p <= 'z') {
            vec_push(tokens, new_token(TK_IDENT, p, *p));
            can_be_negative = false;
            p++;
            continue;
        }

        switch (*p) {
            case '=':
                if(*(p + 1) == '=') {
                    vec_push(tokens, new_token(TK_EQUAL, p, 0));
                    can_be_negative = true;
                    p += 2;
                    continue;
                }
            case '!':
                if(*(p + 1) == '=') {
                    vec_push(tokens, new_token(TK_NEQUAL, p, 0));
                    can_be_negative = true;
                    p += 2;
                    continue;
                }
            case '+':
            case '-':
            case '/':
            case '*':
            case ')':
            case '(':
            case ';':
                vec_push(tokens, new_token(*p, p, 0));
                can_be_negative = true;
                p++;
                continue;
            default:
                fprintf(stderr, "Cannot tokenize: %s (Code Point: %d)\n", p, *p);
                exit(TOKENIZE_ERROR);
        }
    }

    vec_push(tokens, new_token(TK_EOF, p, 0));

    return tokens;
}