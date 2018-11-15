#include "yacc.h"

/**
 ** A parser to turn a stream of tokens into multiple statement expression trees
 **/

Node *unexpected_token(Token token, char *hint, int line_num, int pos) {
    fprintf(stderr, "[Line %d] Unexpected token occured while parsing: %c (Type: %i) (Pos: %i)\n", line_num, token.ty, token.ty, pos);
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

Node *nullary_operation_node(int op) {
    Node *node = malloc(sizeof(Node));
    node->ty = op;
    node->arity = 0;
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

Node *new_scope_node(bool descend) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_SCOPE;
    node->arity = 0;
    node->statements = new_vector();
    node->descend = descend;
    node->break_label = NULL;
    node->continue_label = NULL;
    return node;
}

Node *no_op() {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NOOP;
    node->arity = 0;
    return node;
}

Token *get_token(Vector *tokens, int *pos) {
    return (Token *)tokens->data[*pos];
}

// Checks (and consumes) the next token to ensure syntax
void expect_token(Vector *tokens, int *pos, int line_num, int type) {
    Token *expected = get_token(tokens, pos);
    if(expected->ty != type) {
        fprintf(stderr, "[Line %d] Unexpected token occured while parsing: %c (Type: %i) (Pos: %i)\n", line_num, expected->ty, expected->ty, *pos);
        fprintf(stderr, "Expected: %d\n", type);
        exit(PARSE_ERROR);
    }
    *pos = *pos + 1;
}

// Prototypes for back-referencing/mutual recursion
Node *parse_statement(Vector *tokens, int *pos, Node **current_scope_node);
Node *precedence_12(Vector *tokens, int *pos);

Node *parse_code(Vector *tokens) {
    int *pos = malloc(sizeof(int));
    *pos = 0;

    Node *global_scope = new_scope_node(false);
    Node **current_scope_node = &global_scope;
    Token *tk = get_token(tokens, pos);
    while(tk->ty != TK_EOF) {
        vec_push(global_scope->statements, parse_statement(tokens, pos, current_scope_node));
        tk = get_token(tokens, pos);
    }

    return global_scope;
}

Node *parse_scope(Vector *tokens, int *pos, Node **current_scope_node) {
    Node *new_scope = new_scope_node(true);
    Token *current_token = get_token(tokens, pos);

    while(current_token->ty != '}') {
        vec_push(new_scope->statements, parse_statement(tokens, pos, &new_scope));
        current_token = get_token(tokens, pos);
    }

    *pos = *pos + 1;
    new_scope->parent = *current_scope_node;
    return new_scope;
}

Node *parse_expression(Vector *tokens, int *pos) {
    Node *leftmost_match = precedence_12(tokens, pos);
    Token *current_token = get_token(tokens, pos);

    switch (current_token->ty) {
        case '=':
            // If we are doing an assignment, advance and try to evaluate the rest as a statement
            *pos = *pos + 1;
            return binary_operation_node('=', leftmost_match, parse_expression(tokens, pos));
        default:
            return leftmost_match;
    }
}

Node *parse_statement(Vector *tokens, int *pos, Node **current_scope_node) {
    Token *current_token = get_token(tokens, pos);
    Token *next_token;

    switch(current_token->ty) {
        // A statement might be opening new scope
        Node *cond_expression, *loop_body, *true_condition, *false_condition;
        case '{':
            *pos = *pos + 1;
            return parse_scope(tokens, pos, current_scope_node);
        case ';':
            *pos = *pos + 1;
            // We need to allow for empty statements, like when someone does while(i++ > 100);
            return no_op();
        case TK_BREAK:
            *pos = *pos + 1;
            expect_token(tokens, pos, __LINE__, ';');
            return nullary_operation_node(ND_BREAK);
        case TK_CONTINUE:
            *pos = *pos + 1;
            expect_token(tokens, pos, __LINE__, ';');
            return nullary_operation_node(ND_CONTINUE);
        case TK_IF:
            *pos = *pos + 1;
            expect_token(tokens, pos, __LINE__, '(');
            cond_expression = parse_expression(tokens, pos);
            expect_token(tokens, pos, __LINE__, ')');
            true_condition = parse_statement(tokens, pos, current_scope_node);
            false_condition = no_op();
            next_token = get_token(tokens, pos);
            if(next_token->ty == TK_ELSE) {
                *pos = *pos + 1;
                false_condition = parse_statement(tokens, pos, current_scope_node);
            }
            return ternary_operation_node(ND_IF, cond_expression, true_condition, false_condition);
        case TK_WHILE:
            *pos = *pos + 1;
            expect_token(tokens, pos, __LINE__, '(');
            cond_expression = parse_expression(tokens, pos);
            expect_token(tokens, pos, __LINE__, ')');
            loop_body = parse_statement(tokens, pos, current_scope_node);
            return binary_operation_node(ND_WHILE, cond_expression, loop_body);
        case TK_DO:
            *pos = *pos + 1;
            loop_body = parse_statement(tokens, pos, current_scope_node);
            expect_token(tokens, pos, __LINE__, TK_WHILE);
            expect_token(tokens, pos, __LINE__, '(');
            cond_expression = parse_expression(tokens, pos);
            expect_token(tokens, pos, __LINE__, ')');
            expect_token(tokens, pos, __LINE__, ';');
            return binary_operation_node(ND_DO, loop_body, cond_expression);
        // Otherwise, treat it as an expression separated by semicolons
        default: ;
            Node *expression = parse_expression(tokens, pos);
            next_token = get_token(tokens, pos);
            if (next_token->ty != ';') {
                unexpected_token(*next_token, "Expected semicolon.", __LINE__, *pos);
            }
            *pos = *pos + 1;
            return expression;
    }
}

// The general structure for a parsing function is as follows:
//      1. Try to parse a higher precedence (lower number) term as the left-hand side
//      2. If we find a token of this tier's precedence, create it as a node 
//         and parse the right hand side as the same precedence tier
//      3. If we don't find a token of this tier's precedence, just return the left-hand side.

// Precedence 0:
//  Parentheses, brackets, member selection via object name/pointer, 
//  postfix increment/decrement
Node *precedence_0(Vector *tokens, int *pos) {
    Token *current_token = get_token(tokens, pos);
    
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
    
    // We do things a little differently here since we have some prefix operators to deal with.
    Token *current_token = get_token(tokens, pos);
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
            Token *next_token = get_token(tokens, pos);
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

    Token *current_token = get_token(tokens, pos);
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

    Token *current_token = get_token(tokens, pos);
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

    Token *current_token = get_token(tokens, pos);
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 5:
//  Relational lt/gt/geq/leq
Node *precedence_5(Vector *tokens, int *pos) {
    Node *lhs = precedence_4(tokens, pos);

    Token *current_token = get_token(tokens, pos);
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

    Token *current_token = get_token(tokens, pos);
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

    Token *current_token = get_token(tokens, pos);
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 8:
//  Bitwise xor
Node *precedence_8(Vector *tokens, int *pos) {    
    Node *lhs = precedence_7(tokens, pos);

    Token *current_token = get_token(tokens, pos);
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 9:
//  Bitwise or
Node *precedence_9(Vector *tokens, int *pos) {    
    Node *lhs = precedence_8(tokens, pos);

    Token *current_token = get_token(tokens, pos);
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 10:
//  Logical AND
Node *precedence_10(Vector *tokens, int *pos) {
    Node *lhs = precedence_9(tokens, pos);

    Token *current_token = get_token(tokens, pos);
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 11:
//  Logical OR
Node *precedence_11(Vector *tokens, int *pos) {    
    Node *lhs = precedence_10(tokens, pos);

    Token *current_token = get_token(tokens, pos);
    switch (current_token->ty) {
        default:
            return lhs;
    }
}

// Precedence 12:
//  Ternary Conditional
Node *precedence_12(Vector *tokens, int *pos) {
    Node *lhs = precedence_11(tokens, pos);

    Token *current_token = get_token(tokens, pos);
    switch (current_token->ty) {
        case '?':
            *pos = *pos + 1;
            Node *middle = parse_expression(tokens, pos);
            Token *next_token = get_token(tokens, pos);
            if (next_token->ty != ':') {
                unexpected_token(*next_token, "Expected : in a ternary conditional!", __LINE__, *pos);
            }
            *pos = *pos + 1;
            return ternary_operation_node(ND_TERNARY_CONDITIONAL, lhs, middle, precedence_12(tokens, pos));
        default:
            return lhs;
    }
}