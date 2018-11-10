#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
};

typedef struct {
    int ty;         // Token type
    int val;        // Value of the token, if a number
    char *input;    // Token's string representation (for error messages)
} Token;

typedef struct {
    Token tokens[100];
    int *pos;
} TokenStream;

TokenStream *tokenize(char *p);

enum {
    ND_NUM = 256,   // Integer node type
    ND_IDENT,       // Identifier node type
    ND_EQUAL,       // Equality operator node type
    ND_NEQUAL,      // Not equals operator node type
};

typedef struct Node {
    int ty;         // Node type
    int val;        // Integer value if node is of type ND_NUM
    char name;      // Name of the identifier if type is ND_IDENT
    struct Node *lhs;
    struct Node *rhs;
} Node;

Node **parse_statements(TokenStream *token_stream);

void gen(Node *statement_tree);