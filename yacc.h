#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *new_vector();
void vec_push(Vector *vec, void *elem);

typedef struct {
    Vector *keys;
    Vector *vals;
} Map;

Map *new_map();
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);

enum {
    TOKENIZE_ERROR = 1,
    PARSE_ERROR = 2,
    CODEGEN_ERROR = 3,
};

enum {
    TK_NUM = 256,   // Integer tokens
    TK_IDENT,       // Identifier tokens
    TK_EOF,         // End of input token
    TK_EQUAL,       // ==
    TK_NEQUAL,      // !=
    TK_GEQUAL,      // >=
    TK_LEQUAL,      // <=
    TK_INCREMENT,   // ++
    TK_DECREMENT,   // --
};

typedef struct {
    int ty;         // Token type
    int val;        // Value of the token, if a number
    char *name;
    char *input;    // Token's string representation (for error messages)
} Token;

Vector *tokenize(char *p);

enum {
    ND_NUM = 256,               // Integer node type
    ND_IDENT,                   // Identifier node type
    ND_EQUAL,                   // Equality operator node type
    ND_NEQUAL,                  // Not equals operator node type
    ND_GEQUAL,                  // >=
    ND_LEQUAL,                  // <=
    ND_UNARY_NEG,               // Unary -
    ND_UNARY_POS,               // Unary +
    ND_UNARY_BIT_COMPLEMENT,    // ~
    ND_UNARY_BOOLEAN_NOT,       // !
    ND_PRE_INCREMENT,
    ND_PRE_DECREMENT,
    ND_POST_INCREMENT,
    ND_POST_DECREMENT,
    ND_TERNARY_CONDITIONAL,     // cond ? a : b;
};

typedef struct Node {
    int ty;         // Node type
    int arity;
    int val;        // Integer value if node is of type ND_NUM
    char *name;      // Name of the identifier if type is ND_IDENT
    struct Node *left;      // Left child. First arg in binary/ternary operations
    struct Node *middle;    // Middle child. First arg in unary operations. Second arg in ternary operations
    struct Node *right;     // Right child. Second arg in binary operations. Third arg in ternary operations
} Node;

Vector *parse_statements(Vector *tokens);

void gen(Node *statement_tree, Map *local_variables);

void run_test();