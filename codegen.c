#include "yacc.h"

// Generate the code to put an lval's address on the stack.
void gen_lval(Node *node, Map *local_variables) {
    if(node->ty == ND_IDENT) {
        printf("\tmov rax, rbp\n");
        // Look up the address of our local variables
        printf("\tsub rax, %ld\n", (long)map_get(local_variables, node->name));
        // Push the memory address of our variable onto the stack
        printf("\tpush rax\n");
    } else {
        fprintf(stderr, "Expected an lval but found %d\n", node->ty);
        exit(CODEGEN_ERROR);
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
            gen_lval(statement_tree->left, local_variables);
            // Generate the value that we want to put into this lval
            gen(statement_tree->right, local_variables);
            printf("\tpop rdi\n");
            printf("\tpop rax\n");
            printf("\tmov [rax], rdi\n");
            // By storing our value back on the stack we can chain assignments
            printf("\tpush rdi\n");
            break;
        // Unary negation
        case ND_UNARY_NEG:
            gen(statement_tree->middle, local_variables);
            printf("\tpop rax\n");
            printf("\tneg rax\n");
            printf("\tpush rax\n");
            break;
        // This case is sort of like a no-op, but it can have some side effects in compilation (like co-ercing an lvalue to an rvalue)
        case ND_UNARY_POS:
            gen(statement_tree->middle, local_variables);
            break;
        case ND_UNARY_BIT_COMPLEMENT:
            gen(statement_tree->middle, local_variables);
            printf("\tpop rax\n");
            printf("\tnot rax\n");
            printf("\tpush rax\n");
            break;
        // TODO: Find if there's a more canonical way to perform boolean !
        case ND_UNARY_BOOLEAN_NOT:
            gen(statement_tree->middle, local_variables);
            printf("\tpop rax\n");
            printf("\tcmp rax, 0\n");
            printf("\tsete al\n");
            printf("\tmovzb rax, al\n");
            printf("\tpush rax\n");
            break;
        case ND_PRE_INCREMENT:
            gen_lval(statement_tree->middle, local_variables);
            // Load the address into rax
            printf("\tpop rax\n");
            // Then, get the value inside rax and increment it 
            printf("\tmov rdi, [rax]\n");
            printf("\tinc rdi\n");
            // Store it back into rax's address and put the new value on the stack
            printf("\tmov [rax], rdi\n");
            printf("\tpush rdi\n");
            break;
        case ND_PRE_DECREMENT:
            gen_lval(statement_tree->middle, local_variables);
            printf("\tpop rax\n");
            // Then, get the value inside rax and decrement it 
            printf("\tmov rdi, [rax]\n");
            printf("\tdec rdi\n");
            // Store it back into rax's address and put the new value on the stack
            printf("\tmov [rax], rdi\n");
            printf("\tpush rdi\n");
            break;
        case ND_POST_DECREMENT:
            gen_lval(statement_tree->middle, local_variables);
            // Keep the value in rax on the stack
            printf("\tpop rax\n");
            printf("\tpush [rax]\n");
            // Load the value in rax
            printf("\tmov rdi, [rax]\n");
            // Decrement the value and store it in [rax] (value on stack is unchanged)
            printf("\tdec rdi\n");
            printf("\tmov [rax], rdi\n");
            break;
        case ND_POST_INCREMENT:
            gen_lval(statement_tree->middle, local_variables);
            // Keep the value in rax on the stack
            printf("\tpop rax\n");
            printf("\tpush [rax]\n");
            // Load the value in rax
            printf("\tmov rdi, [rax]\n");
            // Increment the value and store it in [rax] (value on stack is unchanged)
            printf("\tinc rdi\n");
            printf("\tmov [rax], rdi\n");
            break;
        // Default recursive case: binary operations
        default:
            gen(statement_tree->left, local_variables);
            gen(statement_tree->right, local_variables);
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
                case '%':
                    printf("\tmov rdx, 0\n");
                    printf("\tdiv rdi\n");
                    printf("\tmovzb rax, dl\n");
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
                case ND_GEQUAL:
                    printf("\tcmp rax, rdi\n");
                    printf("\tsetge al\n");
                    printf("\tmovzb rax, al\n");
                    break;
                case ND_LEQUAL:
                    printf("\tcmp rax, rdi\n");
                    printf("\tsetle al\n");
                    printf("\tmovzb rax, al\n");
                    break;
                case '>':
                    printf("\tcmp rax, rdi\n");
                    printf("\tsetg al\n");
                    printf("\tmovzb rax, al\n");
                    break;
                case '<':
                    printf("\tcmp rax, rdi\n");
                    printf("\tsetl al\n");
                    printf("\tmovzb rax, al\n");
                    break;
                default:
                    fprintf(stderr, "Unknown operator %c (%d)\n", statement_tree->ty, statement_tree->ty);
                    exit(CODEGEN_ERROR);
            }
            printf("\tpush rax\n");
    }   
}
