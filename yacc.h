#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int fpeek(FILE *stream);

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
    void *default_value;
} Map;

Map *new_map(void *default_value);
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);

enum {
    TOKENIZE_ERROR = 1,
    PARSE_ERROR = 2,
    CODEGEN_ERROR = 3,
    SCOPE_ERROR = 4,
    EXTERNAL_ERROR = 5,
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
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
    TK_DO,
    TK_BREAK,
    TK_CONTINUE,
    TK_LEFT_SHIFT,
    TK_RIGHT_SHIFT,
    TK_LAND,
    TK_LOR,
    TK_GOTO,
    TK_LABEL,
};

typedef struct {
    int ty;         // Token type
    int val;        // Value of the token, if a number
    char *name;
} Token;

Vector *tokenize(FILE *stream);

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
    ND_SCOPE,
    ND_NOOP,
    ND_IF,
    ND_WHILE,
    ND_DO,
    ND_BREAK,
    ND_CONTINUE,
    ND_FOR,
    ND_LEFT_SHIFT,
    ND_RIGHT_SHIFT,
    ND_LAND,
    ND_LOR,
    ND_GOTO,
    ND_LABEL,
};

typedef struct Node {
    int ty;                 // Node type
    int arity;
    int val;                // Integer value if node is of type ND_NUM
    char *name;             // Name of the identifier if type is ND_IDENT or ND_LABEL
    struct Node *left;      // Left child. First arg in binary/ternary operations
    struct Node *middle;    // Middle child. First arg in unary operations. Second arg in ternary operations
    struct Node *right;     // Right child. Second arg in binary operations. Third arg in ternary operations
    struct Node *extra;     // Used only for for-loops
    Vector *statements;     // Used for scope nodes
    struct Node *parent;    // Used for scope node navigation
    bool descend;
    char *break_label;      // Used to keep track of which label a break/continue statement should jump to
    char *continue_label;   
} Node;

Node *parse_code(Vector *tokens);

typedef struct Scope {
    Vector *sub_scopes; 
    Map *variables_declared;
    struct Scope *parent_scope;
    int scopes_traversed;
    char *break_label;      // Used to keep track of which label a break/continue statement should jump to
    char *continue_label;   
} Scope;

typedef struct {
    int offset;     // How far away is the variable from the base pointer of its scope
    int scopes_up;  // How many base pointers have to be climbed to reach the variable
} VariableAddress;

Scope *new_scope(Scope *parent_scope);
void declare_variable(Scope *target_scope, char *variable_name);
VariableAddress *get_variable_location(Scope *current_scope, char *variable_name);
Scope *construct_scope_from_token_stream(Vector *tokens);
Scope *get_next_child_scope(Scope *current_scope);

// void gen(Node *statement_tree, Map *local_variables);
void gen_scope(Node *node, Scope **local_scope);

void run_test();