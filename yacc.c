#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 ** Predefining structs and struct related methods
 **/

// Tokenizer:
enum {
    TK_NUM = 256,   // Integer tokens
    TK_EOF,         // End of input token
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

// Expression Tree Parser:
enum {
    ND_NUM = 256,   // Integer node type
};

typedef struct Node {
    int ty;         // Node type
    int val;        // Integer value if node is of type ND_NUM
    struct Node *lhs;
    struct Node *rhs;
} Node;

Node *new_operation_node(int op, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = op;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_numeric_node(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

// Divides the string `p` into tokens and stores them in a token stream
TokenStream *tokenize(char *p) {
    TokenStream *new_token_stream = malloc(sizeof(TokenStream));
    new_token_stream->pos = malloc(sizeof(int));
    *(new_token_stream->pos) = 0;
    Token *tokens = new_token_stream->tokens;

    int i = 0;
    while (*p) {
        // Skip whitespace
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Read all digits in as base 10 numbers.
        // We also account for the very special case
        // where the first number is negative.
        if (isdigit(*p) || (i == 0 && *p == '-')) {
            tokens[i].ty = TK_NUM;
            tokens[i].input = p;
            tokens[i].val = strtol(p, &p, 10);
            i++;
            continue;
        }

        switch (*p) {
            case '+':
            case '-':
            case '/':
            case '*':
            case ')':
            case '(':
                tokens[i].ty = *p;
                tokens[i].input = p;
                i++;
                p++;
                continue;
            default:
                fprintf(stderr, "Cannot tokenize: %s\n", p);
                exit(1);
        }
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;

    return new_token_stream;
}

// Prototypes for back-referencing/mutual recursion
Node *expr(TokenStream *token_stream);
Node *mul(TokenStream *token_stream);
Node *term(TokenStream *token_stream);

Node *parse_expression_tree(TokenStream *token_stream) {
    int old_pos = *(token_stream->pos);
    Node *node = expr(token_stream);
    *(token_stream->pos) = old_pos;
    return node;
}

// Create an expression tree from a stream of tokens
Node *expr(TokenStream *token_stream) {
    Token *tokens = token_stream->tokens;
    int *pos = token_stream->pos;

    Node *lhs = mul(token_stream);
    switch (tokens[*pos].ty) {
        case TK_EOF:
        case ')':
            return lhs;
        case '+':
            *pos = *pos + 1;
            return new_operation_node('+', lhs, expr(token_stream));
        case '-':
            *pos = *pos + 1;
            return new_operation_node('-', lhs, expr(token_stream));
        default:
            fprintf(stderr, "Unexpected token in expr: %s (%i) (%d)\n", tokens[*pos].input, tokens[*pos].ty, *pos);
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
            return lhs;
        case '*':
            *pos = *pos + 1;
            return new_operation_node('*', lhs, mul(token_stream));
        case '/':
            *pos = *pos + 1;
            return new_operation_node('/', lhs, mul(token_stream));
        default:
            fprintf(stderr, "Unexpected token in mul: %s (%i) (%d)\n", tokens[*pos].input, tokens[*pos].ty, *pos);
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
        case '(':
            *pos = *pos + 1;
            Node *node = expr(token_stream);
            if (tokens[*pos].ty != ')') {
                fprintf(stderr, "Mismatched parentheses: %s", tokens[*pos].input);
            }
            *pos = *pos + 1;
            return node;
        default:
            fprintf(stderr, "Unexpected token in term: %s (%i) (%d)\n", tokens[*pos].input, tokens[*pos].ty, *pos);
    }
}

// Compiling an expression tree to assembly uses a recursive approach:
//      1. If our node is a number (leaf), just push it to the stack.
//      2. If our node is an operation, compile the left hand side, 
//      then the right hand side, then pop both and perform the operation, \
//      and finally return the result to the stack.
void compile_expression_tree(Node *expression_tree) {
    switch(expression_tree->ty) {
        // Base case: numbers
        case ND_NUM:
            printf("\tpush %d\n", expression_tree->val);
            break;
        // Recursive case: operations
        default:
            compile_expression_tree(expression_tree->lhs);
            compile_expression_tree(expression_tree->rhs);
            printf("\tpop rdi\n");
            printf("\tpop rax\n");
            switch(expression_tree->ty) {
                case '*':
                    printf("\tmul rdi\n");
                    break;
                case '/':
                    printf("\tmov rdx, 0\n");
                    printf("\tdiv rdi\n");
                    break;
                case '+':
                    printf("\tadd rax, rdi\n");
                    break;
                case '-':
                    printf("\tsub rax, rdi\n");
                    break;
                default:
                    fprintf(stderr, "Unknown operator %c (%d)\n", expression_tree->ty, expression_tree->ty);
                    exit(1);
            }
            printf("\tpush rax\n");
    }   
}

void error(Token token) {
    fprintf(stderr, "Unexpected token: %s\n", token.input);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "You did not provide the correct number of arguments! (Expected 1)\n");
        return 1;
    }
    
    // Tokenize our input
    TokenStream *token_stream = tokenize(argv[1]);
    Token *tokens = token_stream->tokens;

    // At the present time, we parse our expression tree, but we keep using the tokens as before.
    Node *expression_tree = parse_expression_tree(token_stream);

    // Preliminary headers for assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    compile_expression_tree(expression_tree);
    // Technically the value should already be in rax, but we should clear the stack anyway.
    printf("\tpop rax\n");

    printf("\tret\n");
    return 0;
}