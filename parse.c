#include "yacc.h"

/**
 ** A parser to turn a stream of tokens into multiple statement expression trees
 **/

Node *unexpected_token(Token token, char *hint, int line_num, int pos) {
    fprintf(stderr, "[Line %d] Unexpected token occured while parsing: %s (Type: %i) (Pos: %i)\n", line_num, token.input, token.ty, pos);
    if(hint != NULL) {
        fprintf(stderr, "Hint: %s\n", hint);
    }
    exit(PARSE_ERROR);
    return NULL;
}

Node *ternary_operation_node(int op, Node *left, Node *middle, Node *right) {
    Node *node = malloc(sizeof(Node));
    node->ty = op;
    node->arity = 3;
    node->left = left;
    node->middle = middle;
    node->right = right;
    return node;
}

Node *binary_operation_node(int op, Node *left, Node *right) {
    Node *node = malloc(sizeof(Node));
    node->ty = op;
    node->arity = 2;
    node->left = left;
    node->right = right;
    return node;
}

Node *unary_operation_node(int op, Node *child) {
    Node *node = malloc(sizeof(Node));
    node->ty = op;
    node->arity = 1;
    node->middle = child;
    return node;
}

Node *new_identifier_node(char *name) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_IDENT;
    node->arity = 0;
    node->name = name;
    return node;
}

Node *new_numeric_node(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->arity = 0;
    node->val = val;
    return node;
}

Node *new_scope_node() {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_SCOPE;
    node->arity = 0;
    node->statements = new_vector();
    return node;
}

// Prototypes for back-referencing/mutual recursion
Node *statement(Vector *tokens, int *pos);
Node *precedence_12(Vector *tokens, int *pos);

Node *parse_code(Vector *tokens) {
    int *pos = malloc(sizeof(int));
    *pos = 0;

    // We start in the "global scope" node
    Node *current_scope_node = new_scope_node();

    // Then we go through each token until the end of the file
    Token *current_token = (Token *)tokens->data[*pos];
    while(current_token->ty != TK_EOF) {
        switch(current_token->ty) {
            // When entering a new scope,
            case '{': ;
                Node *child_scope = new_scope_node();
                // consider that scope one of our "statements"
                vec_push(current_scope_node->statements, child_scope);
                child_scope->parent = current_scope_node;
                // but push future statements into this new scope
                current_scope_node = child_scope;
                *pos = *pos + 1;
                break;
            // When leaving a scope,
            case '}':
                // Just set future statements to go back in the scope above.
                current_scope_node = current_scope_node->parent;
                *pos = *pos + 1;
                break;
            default:
                vec_push(current_scope_node->statements, statement(tokens, pos));
        }
        current_token = (Token *)tokens->data[*pos];
    }

    return current_scope_node;
}

// assign -> <expr> <assign'> ;
Node *statement(Vector *tokens, int *pos) {
    Node *lhs = precedence_12(tokens, pos);
    Token *current_token = (Token *)tokens->data[*pos];

    switch (current_token->ty) {
        case ';':
            // When we hit a semicolon, advance and return our completed statement
            *pos = *pos + 1;
            return lhs;
        case '=':
            // If we are doing an assignment, advance and try to evaluate the rest as a statement
            *pos = *pos + 1;
            return binary_operation_node('=', lhs, statement(tokens, pos));
        default:
            // Throw an error if we don't have a semicolon
            return unexpected_token(*current_token, "You may be missing a semicolon.", __LINE__, *pos);
    }
}

// The general structure for a precedence function is as follows:
//      1. Try to parse a higher precedence (lower number) as the left-hand side
//      2. If we find a token of this tier's precedence, create it as a node 
//         and parse the right hand side as the same precedence tier
//      3. If we don't find a token of this tier's precedence, just return the left-hand side.


// Precedence 0:
//  Parentheses, brackets, member selection via object name/pointer, 
//  postfix increment/decrement
Node *precedence_0(Vector *tokens, int *pos) {
    Token *current_token = (Token *)tokens->data[*pos];
    
    switch (current_token->ty){
        case TK_NUM:
            *pos = *pos + 1;
            return new_numeric_node(current_token->val);
        case TK_IDENT:
            *pos = *pos + 1;
            return new_identifier_node(current_token->name);
        case '(':
            *pos = *pos + 1;
            Node *node = precedence_12(tokens, pos);
            Token *next_token = (Token *)tokens->data[*pos];
            if (next_token->ty != ')') {
                unexpected_token(*next_token, "Make sure all parentheses are properly enclosed.", __LINE__, *pos);
            }
            *pos = *pos + 1;
            return node;
        default:
            return unexpected_token(*current_token, NULL, __LINE__, *pos);
    }
}

// Precedence 1 (Right-to-left associative):
//  Prefix increment/decrement, unary plus/minus, logical negation, 
//  bitwise complement, casts, dereference, address, sizeof
Node *precedence_1(Vector *tokens, int *pos) {
    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        case TK_DECREMENT:
            *pos = *pos + 1;
            return unary_operation_node(ND_PRE_DECREMENT, precedence_1(tokens, pos));
        case TK_INCREMENT:
            *pos = *pos + 1;
            return unary_operation_node(ND_PRE_INCREMENT, precedence_1(tokens, pos));
        case '-':
            *pos = *pos + 1;
            return unary_operation_node(ND_UNARY_NEG, precedence_1(tokens, pos));
        case '+':
            *pos = *pos + 1;
            return unary_operation_node(ND_UNARY_POS, precedence_1(tokens, pos));
        case '~':
            *pos = *pos + 1;
            return unary_operation_node(ND_UNARY_BIT_COMPLEMENT, precedence_1(tokens, pos));
        case '!':
            *pos = *pos + 1;
            return unary_operation_node(ND_UNARY_BOOLEAN_NOT, precedence_1(tokens, pos));
        default: ;
            Node *next_node = precedence_0(tokens, pos);
            Token *next_token = (Token *)tokens->data[*pos];
            switch (next_token->ty) {
                case TK_INCREMENT:
                    *pos = *pos + 1;
                    return unary_operation_node(ND_POST_INCREMENT, next_node);
                case TK_DECREMENT:
                    *pos = *pos + 1;
                    return unary_operation_node(ND_POST_DECREMENT, next_node);
                default:
                    return next_node;
            }
    }
}

// Precedence 2:
//  Multiplication, division, modulus
Node *precedence_2(Vector *tokens, int *pos) {
    Node *lhs = precedence_1(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        case '*':
            *pos = *pos + 1;
            return binary_operation_node('*', lhs, precedence_2(tokens, pos));
        case '/':
            *pos = *pos + 1;
            return binary_operation_node('/', lhs, precedence_2(tokens, pos));
        case '%':
            *pos = *pos + 1;
            return binary_operation_node('%', lhs, precedence_2(tokens, pos));
        default:
            return lhs;
    }
}

// Precedence 3:
//  Addition, subtraction
Node *precedence_3(Vector *tokens, int *pos) {
    Node *lhs = precedence_2(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        case '+':
            *pos = *pos + 1;
            return binary_operation_node('+', lhs, precedence_3(tokens, pos));
        case '-':
            *pos = *pos + 1;
            return binary_operation_node('-', lhs, precedence_3(tokens, pos));
        default:
            return lhs;
    }
}

// Precedence 4:
//  Bitwise left/right shift
Node *precedence_4(Vector *tokens, int *pos) {
    Node *lhs = precedence_3(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 5:
//  Relational lt/gt/geq/leq
Node *precedence_5(Vector *tokens, int *pos) {
    Node *lhs = precedence_4(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        case TK_LEQUAL:
                *pos = *pos + 1;
                return binary_operation_node(ND_LEQUAL, lhs, precedence_5(tokens, pos));
            case TK_GEQUAL:
                *pos = *pos + 1;
                return binary_operation_node(ND_GEQUAL, lhs, precedence_5(tokens, pos));
            case '>':
            case '<':
                *pos = *pos + 1;
                return binary_operation_node(current_token->ty, lhs, precedence_5(tokens, pos));
        default:
            return lhs;
    }
}

// Precedence 6:
//  Relational eq/neq
Node *precedence_6(Vector *tokens, int *pos) {
    Node *lhs = precedence_5(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        case TK_EQUAL:
            *pos = *pos + 1;
            return binary_operation_node(ND_EQUAL, lhs, precedence_6(tokens, pos));
        case TK_NEQUAL:
            *pos = *pos + 1;
            return binary_operation_node(ND_NEQUAL, lhs, precedence_6(tokens, pos));
        default:
            return lhs;
    }
}

// Precedence 7: 
//  Bitwise AND
Node *precedence_7(Vector *tokens, int *pos) {    
    Node *lhs = precedence_6(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 8:
//  Bitwise xor
Node *precedence_8(Vector *tokens, int *pos) {    
    Node *lhs = precedence_7(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 9:
//  Bitwise or
Node *precedence_9(Vector *tokens, int *pos) {    
    Node *lhs = precedence_8(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 10:
//  Logical AND
Node *precedence_10(Vector *tokens, int *pos) {
    Node *lhs = precedence_9(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 11:
//  Logical OR
Node *precedence_11(Vector *tokens, int *pos) {    
    Node *lhs = precedence_10(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 12:
//  Ternary Conditional
Node *precedence_12(Vector *tokens, int *pos) {
    Node *lhs = precedence_11(tokens, pos);

    Token *current_token = (Token *)tokens->data[*pos];
    switch (current_token->ty) {
        case '?':
            *pos = *pos + 1;
            Node *middle = precedence_12(tokens, pos);
            Token *next_token = (Token *)tokens->data[*pos];
            if (next_token->ty != ':') {
                unexpected_token(*next_token, "Expected : in a ternary conditional!", __LINE__, *pos);
            }
            *pos = *pos + 1;
            return ternary_operation_node(ND_TERNARY_CONDITIONAL, lhs, middle, precedence_12(tokens, pos));
        default:
            return lhs;
    }
}