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

Map *get_reserved_words() {
    Map *reserved_word_map = new_map((void *)(long)-1);
    map_put(reserved_word_map, "if", (void *)(long)TK_IF);
    map_put(reserved_word_map, "else", (void *)(long)TK_ELSE);
    map_put(reserved_word_map, "for", (void *)(long)TK_FOR);

    return reserved_word_map;
}

// Divides the string `p` into tokens and stores them in a token stream
Vector *tokenize(char *p) {
    Vector *tokens = new_vector();
    Map *reserved_word_map = get_reserved_words();

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

        // Check for words (for reserved keywords and identifiers)
        if(isalpha(*p) || *p == '_') {
            // String parsing
            int i = 0;
            char *identifier_name = malloc(128 * sizeof(char));
            while(is_identifier_character(*p)) {
                identifier_name[i++] = *(p++);
            }
            identifier_name[i] = 0;

            // Look up any potential reserved word this maps to, and if it doesn't set it as an identifier
            int word_code = (long)map_get(reserved_word_map, identifier_name);
            if(word_code != -1) {
                vec_push(tokens, new_token(word_code, p, 0, NULL));
            } else {
                vec_push(tokens, new_token(TK_IDENT, p, 0, identifier_name));
            }
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
                if(*(p + 1) == '-') {
                    vec_push(tokens, new_token(TK_DECREMENT, p, 0, NULL));
                    p += 2;
                    continue;
                }
            case '+':
                if(*(p + 1) == '+') {
                    vec_push(tokens, new_token(TK_INCREMENT, p, 0, NULL));
                    p += 2;
                    continue;
                }
            case '/':
            case '*':
            case ')':
            case '(':
            case '}':
            case '{':
            case ';':
            case '~':
            case '%':
            case ':':
            case '?':
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