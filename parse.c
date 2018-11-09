#include "yacc.h"

/**
 ** A parser to turn a stream of tokens into multiple statement expression trees
 **/

Node *unexpected_token(Token token, char *hint) {
    fprintf(stderr, "Unexpected token occured while parsing: %s (Type: %i)\n", token.input, token.ty);
    if(hint != NULL) {
        fprintf(stderr, "Hint: %s\n", hint);
    }
    exit(1);
    return NULL;
}

Node *new_operation_node(int op, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = op;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_identifier_node(char name) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_IDENT;
    node->name = name;
    return node;
}

Node *new_numeric_node(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

// Prototypes for back-referencing/mutual recursion
Node *statement(TokenStream *token_stream);
Node *assignment(TokenStream *token_stream);
Node *expr(TokenStream *token_stream);
Node *mul(TokenStream *token_stream);
Node *term(TokenStream *token_stream);

Node **parse_statements(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    // Keep track of our initial position to keep things tidy™
    int old_pos = *pos;

    // As long as we haven't hit the end of the file, we keep parsing new statements
    // statement() should fail if it doesn't advance (i.e. find a semicolon), so this shouldn't infinitely loop
    Node **statements = malloc(100 * sizeof(Node));
    for(int i = 0; tokens[*pos].ty != TK_EOF; i++) {
        statements[i] = statement(token_stream);
    }

    *(token_stream->pos) = old_pos;
    return statements;
}

// assign -> <expr> <assign'> ;
Node *statement(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = expr(token_stream);

    switch(tokens[*pos].ty) {
        case ';':
            // When we hit a semicolon, advance and return our completed statement
            *pos = *pos + 1;
            return lhs;
        case '=':
            // If we are doing an assignment, advance and try to evaluate the rest as a statement
            *pos = *pos + 1;
            return new_operation_node('=', lhs, statement(token_stream));
        default:
            // Throw an error if we don't have a semicolon
            return unexpected_token(tokens[*pos], "You may be missing a semicolon.");
    }
}

// Create an expression tree from a stream of tokens
Node *expr(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = mul(token_stream);
    switch (tokens[*pos].ty) {
        case TK_EOF:
        case ')':
        case '=':
        case ';':
            return lhs;
        case '+':
            *pos = *pos + 1;
            return new_operation_node('+', lhs, expr(token_stream));
        case '-':
            *pos = *pos + 1;
            return new_operation_node('-', lhs, expr(token_stream));
        default:
            return unexpected_token(tokens[*pos], NULL);
    }
}

Node *mul(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = term(token_stream);
    switch (tokens[*pos].ty) {
        case TK_EOF:
        case '+':
        case '-':
        case ')':
        case '=':
        case ';':
            return lhs;
        case '*':
            *pos = *pos + 1;
            return new_operation_node('*', lhs, mul(token_stream));
        case '/':
            *pos = *pos + 1;
            return new_operation_node('/', lhs, mul(token_stream));
        default:
            return unexpected_token(tokens[*pos], NULL);
    }
}

// A term can either be a number, or an entire expression wrapped in parentheses
Node *term(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;
    
    switch (tokens[*pos].ty) {
        case TK_NUM:
            *pos = *pos + 1;
            return new_numeric_node(tokens[*pos - 1].val);
        case TK_IDENT:
            *pos = *pos + 1;
            return new_identifier_node(tokens[*pos - 1].val);
        case '(':
            *pos = *pos + 1;
            Node *node = expr(token_stream);
            if (tokens[*pos].ty != ')') {
                unexpected_token(tokens[*pos], "Make sure all parentheses are properly enclosed.");
            }
            *pos = *pos + 1;
            return node;
        default:
            return unexpected_token(tokens[*pos], NULL);
    }
}