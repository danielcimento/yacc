#include "yacc.h"

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
                    exit(CODEGEN_ERROR);
            }
            printf("\tpush rax\n");
    }   
}
