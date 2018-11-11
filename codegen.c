#include "yacc.h"

// Generate the code to put an lval's address on the stack.
void gen_lval(Node *node, Map *local_variables) {
    if(node->ty == ND_IDENT) {
        printf("\tmov rax, rbp\n");
        // Look up the address of our local variables
        printf("\tsub rax, %ld\n", (long)map_get(local_variables, node->name));
        // Push the memory address of our variable onto the stack
        printf("\tpush rax\n");
    }
}

// Generate assembly code from an expression tree using a recursive approach.
void gen(Node *statement_tree, Map *local_variables) {
    switch(statement_tree->ty) {
        case ND_NUM:
            // For numbers, we only push the direct value on the stack
            printf("\tpush %d\n", statement_tree->val);
            break;
        case ND_IDENT:
            // Fetch the value in that address and store it on the stack
            gen_lval(statement_tree, local_variables);
            printf("\tpop rax\n");
            printf("\tmov rax, [rax]\n");
            printf("\tpush rax\n");
            break;
        case '=':
            // The left-hand side of any assignment must be an lval
            gen_lval(statement_tree->lhs, local_variables);
            // Generate the value that we want to put into this lval
            gen(statement_tree->rhs, local_variables);
            printf("\tpop rdi\n");
            printf("\tpop rax\n");
            printf("\tmov [rax], rdi\n");
            // By storing our value back on the stack we can chain assignments
            printf("\tpush rdi\n");
            break;
        // Recursive case: operations
        default:
            gen(statement_tree->lhs, local_variables);
            gen(statement_tree->rhs, local_variables);
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
                case ND_EQUAL:
                    printf("\tcmp rdi, rax\n");
                    printf("\tsete al\n");
                    printf("\tmovzb rax, al\n");
                    break;
                case ND_NEQUAL:
                    printf("\tcmp rdi, rax\n");
                    printf("\tsetne al\n");
                    printf("\tmovzb rax, al\n");
                    break;
                default:
                    fprintf(stderr, "Unknown operator %c (%d)\n", statement_tree->ty, statement_tree->ty);
                    exit(CODEGEN_ERROR);
            }
            printf("\tpush rax\n");
    }   
}
