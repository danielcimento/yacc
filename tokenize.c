#include "yacc.h"
#include <stdbool.h>

bool is_identifier_character(char c) {
    return isalpha(c) || isdigit(c) || c == '_';
}

int fpeek(FILE *stream) {
    int c;
    c = fgetc(stream);
    ungetc(c, stream);
    return c;
}

Token *new_token(int type, int val, char *name) {
    Token *tk = malloc(sizeof(Token));
    tk->ty = type;
    tk->val = val;
    tk->name = name;
    return tk;
}

Map *get_reserved_words() {
    Map *reserved_word_map = new_map((void *)(long)-1);
    map_put(reserved_word_map, "if", (void *)(long)TK_IF);
    map_put(reserved_word_map, "else", (void *)(long)TK_ELSE);
    map_put(reserved_word_map, "while", (void *)(long)TK_WHILE);
    map_put(reserved_word_map, "for", (void *)(long)TK_FOR);

    return reserved_word_map;
}

enum {
    NO_COMMENT = 0,
    INLINE_COMMENT = 1,
    BLOCK_COMMENT = 2
};

// Divides the string `p` into tokens and stores them in a token stream
Vector *tokenize(FILE *stream) {
    int position = 0;
    int comment_state = NO_COMMENT;
    char c;                             // currently read character
    Vector *tokens = new_vector();
    Map *reserved_word_map = get_reserved_words();

    while ((c = fgetc(stream)) != EOF) {
        // Skip whitespace
        if (isspace(c) || comment_state != NO_COMMENT) {
            // Finish block comments only on 
            if(comment_state == BLOCK_COMMENT && c == '*') {
                if(fpeek(stream) == '/') {
                    fgetc(stream);
                    comment_state = NO_COMMENT;
                    continue;
                }
            }
            // Complete inline comments after a newline character shows up.
            if(comment_state == INLINE_COMMENT && c == '\n') comment_state = NO_COMMENT;
            continue;
        }

        // Read all digit sequences in as numbers.
        if (isdigit(c)) {
            int base = 10;
            int num_val = 0;
            int num_representation;

            if(c == '0') {
                if(fpeek(stream) == 'x') {
                    fgetc(stream);
                    base = 16;
                } else if(fpeek(stream) == 'b') {
                    fgetc(stream);
                    base = 2;
                } else {
                    base = 8;
                }
            }

            do {
                num_val *= base;
                if(c > 'f') {
                    fprintf(stderr, "Invalid digit in hexadecimal number: %c\n", c);
                    exit(TOKENIZE_ERROR);
                } if(c > '7' && base == 8) {
                    fprintf(stderr, "Invalid digit in octal number: %c\n", c);
                    exit(TOKENIZE_ERROR);
                } if(c > '1' && base == 2) {
                    fprintf(stderr, "Invalid digit in binary number: %c\n", c);
                }
                num_representation = (c > '9') ? c - 'a' + 10 : c - '0';
                num_val += num_representation;
            } while(isdigit(c = fgetc(stream)) || (base == 16 && c >= 'a' && c <= 'f'));
            ungetc(c, stream);

            vec_push(tokens, new_token(TK_NUM, num_val, NULL));
            continue;
        }

        // Check for words (for reserved keywords and identifiers)
        if(isalpha(c) || c == '_') {
            // String parsing
            int i = 0;
            char *identifier_name = malloc(128 * sizeof(char));
            do {
                identifier_name[i++] = c;
            } while(is_identifier_character(c = fgetc(stream)));
            // Close string and put back the character that isn't part of our string
            identifier_name[i] = 0;
            ungetc(c, stream);

            // Look up any potential reserved word this maps to, and if it doesn't set it as an identifier
            int word_code = (long)map_get(reserved_word_map, identifier_name);
            if(word_code != -1) {
                vec_push(tokens, new_token(word_code, 0, NULL));
            } else {
                vec_push(tokens, new_token(TK_IDENT, 0, identifier_name));
            }
            continue;
        }

        switch (c) {
            case '=':
                if(fpeek(stream) == '=') {
                    vec_push(tokens, new_token(TK_EQUAL, 0, NULL));
                    fgetc(stream);
                    continue;
                }
            case '!':
                if(fpeek(stream) == '=') {
                    vec_push(tokens, new_token(TK_NEQUAL, 0, NULL));
                    fgetc(stream);
                    continue;
                }
            case '>':
                if(fpeek(stream) == '=') {
                    vec_push(tokens, new_token(TK_GEQUAL, 0, NULL));
                    fgetc(stream);
                    continue;
                }
            case '<':
                if(fpeek(stream) == '=') {
                    vec_push(tokens, new_token(TK_LEQUAL, 0, NULL));
                    fgetc(stream);
                    continue;
                }
            case '-':
                if(fpeek(stream) == '-') {
                    vec_push(tokens, new_token(TK_DECREMENT, 0, NULL));
                    fgetc(stream);
                    continue;
                }
            case '+':
                if(fpeek(stream) == '+') {
                    vec_push(tokens, new_token(TK_INCREMENT, 0, NULL));
                    fgetc(stream);
                    continue;
                }
            case '/':
                if(fpeek(stream) == '/') {
                    comment_state = INLINE_COMMENT;
                    fgetc(stream);
                    continue;
                } else if (fpeek(stream) == '*') {
                    comment_state = BLOCK_COMMENT;
                    fgetc(stream);
                    continue;
                }
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
                vec_push(tokens, new_token(c, 0, NULL));
                continue;
            default:
                fprintf(stderr, "Cannot tokenize \"%c\" at position %d (Code Point: %d)\n", c, position, c);
                exit(TOKENIZE_ERROR);
        }
        position++;
    }

    vec_push(tokens, new_token(TK_EOF, 0, NULL));

    if(comment_state == BLOCK_COMMENT) {
        fprintf(stderr, "Warning: File ends before a block comment is closed. This won't cause issues now, but may cause unintended bugs in the future!\n");
    }
    return tokens;
}