#include "yacc.h"

/**
 ** A parser to turn a stream of tokens into multiple statement expression trees
 **/

Node *unexpected_token(Token token, char *hint) {
    fprintf(stderr, "Unexpected token occured while parsing: %s (Type: %i)\n", token.input, token.ty);
    if(hint != NULL) {
        fprintf(stderr, "Hint: %s\n", hint);
    }
    exit(PARSE_ERROR);
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
Node *precedence_12(TokenStream *token_stream);

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

    Node *lhs = precedence_12(token_stream);

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

// The general structure for a precedence function is as follows:
//      1. Extract the tokens and pos for easier manipulation
//      2. Try to parse a higher precedence (lower number) as the left-hand side
//      3. If we find a token of this tier's precedence, create it as a node 
//         and parse the right hand side as the same precedence tier
//      4. If we don't find a token of this tier's precedence, just return the left-hand side.


// Precedence 0:
//  Parentheses, brackets, member selection via object name/pointer, 
//  postfix increment/decrement
Node *precedence_0(TokenStream *token_stream) {
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
            Node *node = precedence_12(token_stream);
            if (tokens[*pos].ty != ')') {
                unexpected_token(tokens[*pos], "Make sure all parentheses are properly enclosed.");
            }
            *pos = *pos + 1;
            return node;
        default:
            return unexpected_token(tokens[*pos], NULL);
    }
}

// Precedence 1 (Right-to-left associative):
//  Prefix increment/decrement, unary plus/minus, logical negation, 
//  bitwise complement, casts, dereference, address, sizeof
Node *precedence_1(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_0(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}

// Precedence 2:
//  Multiplication, division, modulus
Node *precedence_2(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_1(token_stream);
    switch (tokens[*pos].ty) {
        case '*':
            *pos = *pos + 1;
            return new_operation_node('*', lhs, precedence_2(token_stream));
        case '/':
            *pos = *pos + 1;
            return new_operation_node('/', lhs, precedence_2(token_stream));
        default:
            return lhs;
    }
}

// Precedence 3:
//  Addition, subtraction
Node *precedence_3(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_2(token_stream);
    switch (tokens[*pos].ty) {
        case '+':
            *pos = *pos + 1;
            return new_operation_node('+', lhs, precedence_3(token_stream));
        case '-':
            *pos = *pos + 1;
            return new_operation_node('-', lhs, precedence_3(token_stream));
        default:
            return lhs;
    }
}

// Precedence 4:
//  Bitwise left/right shift
Node *precedence_4(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_3(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}

// Precedence 5:
//  Relational lt/gt/geq/leq
Node *precedence_5(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_4(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}

// Precedence 6:
//  Relational eq/neq
Node *precedence_6(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_5(token_stream);
    switch (tokens[*pos].ty) {
        case TK_EQUAL:
            *pos = *pos + 1;
            return new_operation_node(ND_EQUAL, lhs, precedence_6(token_stream));
        case TK_NEQUAL:
            *pos = *pos + 1;
            return new_operation_node(ND_NEQUAL, lhs, precedence_6(token_stream));
        default:
            return lhs;
    }
}

// Precedence 7: 
//  Bitwise AND
Node *precedence_7(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_6(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}

// Precedence 8:
//  Bitwise xor
Node *precedence_8(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_7(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}

// Precedence 9:
//  Bitwise or
Node *precedence_9(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_8(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}

// Precedence 10:
//  Logical AND
Node *precedence_10(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_9(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}

// Precedence 11:
//  Logical OR
Node *precedence_11(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_10(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}

// Precedence 12:
//  Ternary Conditional
Node *precedence_12(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = precedence_11(token_stream);
    switch (tokens[*pos].ty) {
        default:
            return lhs;
    }
}