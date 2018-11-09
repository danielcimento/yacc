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
    TK_IDENT,       // Identifier tokens
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
    ND_IDENT,       // Identifier node type
};

typedef struct Node {
    int ty;         // Node type
    int val;        // Integer value if node is of type ND_NUM
    char name;      // Name of the identifier if type is ND_IDENT
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

        // Allow recognizing single lowercase characters as identifiers
        if('a' <= *p && *p <= 'z') {
            tokens[i].ty = TK_IDENT;
            tokens[i].input = p;
            tokens[i].val = *p;
            i++;
            p++;
            continue;
        }

        switch (*p) {
            case '+':
            case '-':
            case '/':
            case '*':
            case ')':
            case '(':
            case '=':
            case ';':
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
            fprintf(stderr, "Unexpected token occured during statement parse. Maybe missing a semicolon?: %s (%i) (%d)\n", tokens[*pos].input, tokens[*pos].ty, *pos);
            exit(1);
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
        case TK_IDENT:
            *pos = *pos + 1;
            return new_identifier_node(tokens[*pos - 1].val);
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

// Generate the code to put an lval's address on the stack.
void gen_lval(Node *node) {
    if(node->ty == ND_IDENT) {
        printf("\tmov rax, rbp\n");
        // Add n-byte offset, where n is that character's ordinal location in the alphabet
        printf("\tsub rax, %d\n", ('z'- node->name + 1) * 8);
        // Push the memory address of our variable onto the stack
        printf("\tpush rax\n");
    }
}

// Generate assembly code from an expression tree using a recursive approach.
void gen(Node *statement_tree) {
    switch(statement_tree->ty) {
        case ND_NUM:
            // For numbers, we only push the direct value on the stack
            printf("\tpush %d\n", statement_tree->val);
            break;
        case ND_IDENT:
            // Fetch the value in that address and store it on the stack
            gen_lval(statement_tree);
            printf("\tpop rax\n");
            printf("\tmov rax, [rax]\n");
            printf("\tpush rax\n");
            break;
        case '=':
            // The left-hand side of any assignment must be an lval
            gen_lval(statement_tree->lhs);
            // Generate the value that we want to put into this lval
            gen(statement_tree->rhs);
            printf("\tpop rdi\n");
            printf("\tpop rax\n");
            printf("\tmov [rax], rdi\n");
            // By storing our value back on the stack we can chain assignments
            printf("\tpush rdi\n");
            break;
        // Recursive case: operations
        default:
            gen(statement_tree->lhs);
            gen(statement_tree->rhs);
            printf("\tpop rdi\n");
            printf("\tpop rax\n");
            switch(statement_tree->ty) {
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
                    fprintf(stderr, "Unknown operator %c (%d)\n", statement_tree->ty, statement_tree->ty);
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
    Node **statements = parse_statements(token_stream);

    // Preliminary headers for assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Function prologue:
    printf("\tpush rbp\n");
    printf("\tmov rbp, rsp\n");
    // Right now we reserve space as though we'll use every variable
    printf("\tsub rsp, 208\n");
    
    // Generate every statement we've written
    for(int i = 0; statements[i]; i++) {
        gen(statements[i]);
        // Technically the value should already be in rax, but we should clear the stack anyway.
        printf("\tpop rax\n");
    }

    // Function epilogue:
    printf("\tmov rsp, rbp\n");
    printf("\tpop rbp\n");
    printf("\tret\n");
    return 0;
}