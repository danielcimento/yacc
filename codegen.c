#include "yacc.h"

int LABELS_GENERATED = 0;
void gen(Node *statement_tree, Scope **local_scope);

# ifdef DEBUG
    # define comment(s, ...) printf("\t\t\t; "); printf(s, ##__VA_ARGS__); printf("\n")
# else
    # define comment(s, ...) printf("\n")
# endif

bool places_on_stack(int ty) {
    switch(ty) {
        case ND_SCOPE:
        case ND_WHILE:
        case ND_IF:
        case ND_DO:
        case ND_BREAK:
        case ND_CONTINUE:
        case ND_NOOP:
            return false;
        default:
            return true;
    }
}

void scope_epilogue() {
    printf("\tmov rsp, rbp\n");
    printf("\tpop rbp\n");
}

// Generate the code to put an lval's address on the stack.
void gen_lval(Node *node, Scope **local_scope) {
    if(node->ty == ND_IDENT) {
        printf("\tmov rax, rbp\n");

        // Look up the address of our local variables
        VariableAddress *referenced_var_add = get_variable_location(*local_scope, node->name);

        for(int i = 0; i < referenced_var_add->scopes_up; i ++) {
            printf("\tmov rax, [rax]\n"); // Climb up one base pointer
        }

        printf("\tsub rax, %d\n", referenced_var_add->offset);

        // Push the memory address of our variable onto the stack
        printf("\tpush rax\n");
    } else {
        fprintf(stderr, "Expected an lval but found %d\n", node->ty);
        exit(CODEGEN_ERROR);
    }
}

void gen_scope(Node *node, Scope **local_scope) {
    // Go into our new scope
    if(node->descend) {
        *local_scope = get_next_child_scope(*local_scope);
    }

    if(node->parent) {
        switch(node->parent->ty) {
            case ND_WHILE:
            case ND_DO:
            case ND_FOR:
                (*local_scope)->break_label = node->parent->break_label;
                (*local_scope)->continue_label = node->parent->continue_label;
                break;
            default:
                break;
        }
    }

    // Function prologue:
    printf("\tpush rbp\n");
    printf("\tmov rbp, rsp\n");

    printf("\tsub rsp, %d", (*local_scope)->variables_declared->keys->len * 8);
    comment("Allocate %d variables to the stack", (*local_scope)->variables_declared->keys->len);

    // Generate every statement in this scope
    for(int i = 0; i < node->statements->len; i++) {
        Node *current_node = (Node *)node->statements->data[i];
        gen(current_node, local_scope);
        // Before we can return, we have to keep our stack balanced. But we can't pop after things that act like scopes (as recusively they've already been balanced).
        if(places_on_stack(current_node->ty)) printf("\tpop rax\n");
    }

    // Function epilogue:
    scope_epilogue();

    if(node->descend) {
        // Leave our scope
        *local_scope = (*local_scope)->parent_scope;
        // Mark that we've completed traversing this scope
        (*local_scope)->scopes_traversed = (*local_scope)->scopes_traversed + 1;
    }
}

void gen_unary(Node *statement_tree, Scope **local_scope) {
    switch(statement_tree->ty) {
        // Unary negation
        case ND_UNARY_NEG:
            gen(statement_tree->middle, local_scope);
            printf("\tpop rax\n");
            printf("\tneg rax\n");
            printf("\tpush rax\n");
            break;
        // This case is sort of like a no-op, but it can have some side effects in compilation (like co-ercing an lvalue to an rvalue)
        case ND_UNARY_POS:
            gen(statement_tree->middle, local_scope);
            break;
        case ND_UNARY_BIT_COMPLEMENT:
            gen(statement_tree->middle, local_scope);
            printf("\tpop rax\n");
            printf("\tnot rax\n");
            printf("\tpush rax\n");
            break;
        // TODO: Find if there's a more canonical way to perform boolean !
        case ND_UNARY_BOOLEAN_NOT:
            gen(statement_tree->middle, local_scope);
            printf("\tpop rax\n");
            printf("\tcmp rax, 0\n");
            printf("\tsete al\n");
            printf("\tmovzb rax, al\n");
            printf("\tpush rax\n");
            break;
        case ND_PRE_INCREMENT:
            gen_lval(statement_tree->middle, local_scope);
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
            gen_lval(statement_tree->middle, local_scope);
            printf("\tpop rax\n");
            // Then, get the value inside rax and decrement it 
            printf("\tmov rdi, [rax]\n");
            printf("\tdec rdi\n");
            // Store it back into rax's address and put the new value on the stack
            printf("\tmov [rax], rdi\n");
            printf("\tpush rdi\n");
            break;
        case ND_POST_DECREMENT:
            gen_lval(statement_tree->middle, local_scope);
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
            gen_lval(statement_tree->middle, local_scope);
            // Keep the value in rax on the stack
            printf("\tpop rax\n");
            printf("\tpush [rax]\n");
            // Load the value in rax
            printf("\tmov rdi, [rax]\n");
            // Increment the value and store it in [rax] (value on stack is unchanged)
            printf("\tinc rdi\n");
            printf("\tmov [rax], rdi\n");
            break;
        default:
            fprintf(stderr, "Unknown unary operation: %d\n", statement_tree->ty);
            exit(CODEGEN_ERROR);
    }
} 

void gen_binary(Node *statement_tree, Scope **local_scope) {
    // Special cases that don't follow the "evaluate args, pop args, compute" sequence
    switch(statement_tree->ty) {
        int current_label;
        case '=':
            // The left-hand side of any assignment must be an lval
            gen_lval(statement_tree->left, local_scope);
            // Generate the value that we want to put into this lval
            gen(statement_tree->right, local_scope);
            printf("\tpop rdi\n");
            printf("\tpop rax\n");
            printf("\tmov [rax], rdi\n");
            // By storing our value back on the stack we can chain assignments
            printf("\tpush rdi\n");
            return;
        case ND_WHILE:
            current_label = LABELS_GENERATED++;
            statement_tree->break_label = malloc(sizeof(char) * 32);
            snprintf(statement_tree->break_label, 32, "wle_%d", current_label);
            statement_tree->continue_label = malloc(sizeof(char) * 32);
            snprintf(statement_tree->continue_label, 32, "wlb_%d", current_label);
            printf("wlb_%d:\n", current_label);
            // Evaluate the conditional
            gen(statement_tree->left, local_scope);
            printf("\tpop rax\n");
            printf("\ttest rax, rax\n");
            // If the conditional is false, end the loop
            printf("\tjz wle_%d\n", current_label);
            // Before we can generate a child scope, we need to keep track of where that scope should break/continue to
            statement_tree->right->parent = statement_tree;
            gen(statement_tree->right, local_scope);
            if(places_on_stack(statement_tree->right->ty)) printf("\tpop rax\n");
            // After we finish the loop body, jump back to the condition
            printf("\tjmp wlb_%d\n", current_label);
            printf("wle_%d:\n", current_label);
            return;
        case ND_DO:
            current_label = LABELS_GENERATED++;
            statement_tree->break_label = malloc(sizeof(char) * 32);
            snprintf(statement_tree->break_label, 32, "dwe_%d", current_label);
            statement_tree->continue_label = malloc(sizeof(char) * 32);
            snprintf(statement_tree->continue_label, 32, "dwc_%d", current_label);
            printf("dwb_%d:\n", current_label);
            // Execute the loop body
            statement_tree->left->parent = statement_tree;
            gen(statement_tree->left, local_scope);
            if(places_on_stack(statement_tree->left->ty)) printf("\tpop rax\n");
            // Evaluate the conditional
            printf("dwc_%d:\n", current_label);
            gen(statement_tree->right, local_scope);
            printf("\tpop rax\n");
            printf("\ttest rax, rax\n");
            // If the conditional is true, continue the loop
            printf("\tjnz dwb_%d\n", current_label);
            printf("dwe_%d:\n", current_label);
            return;
        default:
            break;
    }

    gen(statement_tree->left, local_scope);
    gen(statement_tree->right, local_scope);
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
            fprintf(stderr, "Unknown binary operation: %d\n", statement_tree->ty);
            exit(CODEGEN_ERROR);
    }
    printf("\tpush rax\n");
}

void gen_ternary(Node *statement_tree, Scope **local_scope) {
    int current_label;
    switch(statement_tree->ty) {
        // Ternary Operation
        case ND_TERNARY_CONDITIONAL:
            current_label = LABELS_GENERATED++;
            // First, we write the code to compute the value of the boolean expression
            gen(statement_tree->left, local_scope);
            // Pop the value from the stack and jump to the false condition if 0
            printf("\tpop rax\n");
            printf("\ttest rax, rax\n");
            printf("\tjz cond_f_%d\n", current_label);
            // Assuming we haven't jumped, we're in the true branch
            gen(statement_tree->middle, local_scope);
            printf("\tjmp cond_end_%d\n", current_label);
            printf("cond_f_%d:\n", current_label);
            gen(statement_tree->right, local_scope);
            printf("cond_end_%d:\n", current_label);
            // Our original assumption is that our recursive trees end by putting their value on the stack, so we don't need to do anything else.
            break;
        case ND_IF:
            // Same as ternary conditionals, but we have to remember to pop the stack when we aren't given a block as an argument
            current_label = LABELS_GENERATED++;
            gen(statement_tree->left, local_scope);
            // Pop the value from the stack and jump to the false condition if 0
            printf("\tpop rax\n");
            printf("\ttest rax, rax\n");
            printf("\tjz cond_f_%d\n", current_label);
            gen(statement_tree->middle, local_scope);
            if(places_on_stack(statement_tree->middle->ty)) printf("\tpop rax\n");
            printf("\tjmp cond_end_%d\n", current_label);
            printf("cond_f_%d:\n", current_label);
            gen(statement_tree->right, local_scope);
            if(places_on_stack(statement_tree->right->ty)) printf("\tpop rax\n");
            printf("cond_end_%d:\n", current_label);
            break;
        default: 
            fprintf(stderr, "Unknown ternary operation: %d\n", statement_tree->ty);
            exit(CODEGEN_ERROR);
    }
}

void gen_quaternary(Node *statement_tree, Scope **local_scope) {
    int current_label;
    switch(statement_tree->ty) {
        case ND_FOR:
            current_label = LABELS_GENERATED++;
            statement_tree->break_label = malloc(sizeof(char) * 32);
            snprintf(statement_tree->break_label, 32, "fle_%d", current_label);
            statement_tree->continue_label = malloc(sizeof(char) * 32);
            snprintf(statement_tree->continue_label, 32, "flc_%d", current_label);
            // Evaluate the initializer
            gen(statement_tree->left, local_scope);
            if(places_on_stack(statement_tree->left->ty)) printf("\tpop rax\n");
            printf("flc_%d:\n", current_label);
            // Evaluate the conditional
            gen(statement_tree->middle, local_scope);
            if(places_on_stack(statement_tree->middle->ty)) {
                printf("\tpop rax\n");
                printf("\ttest rax, rax\n");
                printf("\tjz fle_%d\n", current_label);
            }
            statement_tree->extra->parent = statement_tree;
            // Evaluate the loop body
            gen(statement_tree->extra, local_scope);
            if(places_on_stack(statement_tree->extra->ty)) printf("\tpop rax\n");
            // Evaluate the post-loop statement
            gen(statement_tree->right, local_scope);
            if(places_on_stack(statement_tree->right->ty)) printf("\tpop rax\n");
            // Go back to conditional
            printf("\tjmp flc_%d\n", current_label);
            printf("fle_%d:\n", current_label);
            break;
        default: 
            fprintf(stderr, "Unknown quaternary operation: %d\n", statement_tree->ty);
            exit(CODEGEN_ERROR);
    }
}

void gen(Node *statement_tree, Scope **local_scope) {
    int scopes_to_break = 1;

    switch(statement_tree->arity) {
        case 4:
            gen_quaternary(statement_tree, local_scope);
            break;
        case 3:
            gen_ternary(statement_tree, local_scope);
            break;
        case 2:
            gen_binary(statement_tree, local_scope);
            break;
        case 1:
            gen_unary(statement_tree, local_scope);
            break;
        default:
            switch(statement_tree->ty) {
                case ND_BREAK: ;
                    Scope *breakable_scope = *local_scope;
                    while(breakable_scope->break_label == NULL) {
                        if(breakable_scope->parent_scope == NULL) {
                            fprintf(stderr, "Could not find a scope to break from. Considering 'break' as a no-op.\n");
                            return;
                        }
                        breakable_scope = breakable_scope->parent_scope;
                        scopes_to_break++;
                    }
                    while(scopes_to_break-- > 0) scope_epilogue();
                    printf("\tjmp %s\n", breakable_scope->break_label);
                    break;
                case ND_CONTINUE: ;
                    Scope *continuable_scope = *local_scope;
                    while(continuable_scope->continue_label == NULL) {
                        if(continuable_scope->parent_scope == NULL) {
                            fprintf(stderr, "Could not find a scope to continue from. Considering 'continue' as a no-op.\n");
                            return;
                        }
                        continuable_scope = continuable_scope->parent_scope;
                        scopes_to_break++;
                    }
                    while(scopes_to_break-- > 0) scope_epilogue();
                    printf("\tjmp %s\n", continuable_scope->continue_label);
                    break;
                case ND_NOOP:
                    break;
                case ND_SCOPE:
                    gen_scope(statement_tree, local_scope);
                    break;
                case ND_NUM:
                    // For numbers, we only push the direct value on the stack
                    printf("\tpush %d", statement_tree->val);
                    comment("Place %d onto the stack", statement_tree->val);
                    break;
                case ND_IDENT:
                    // Fetch the value in that address and store it on the stack
                    gen_lval(statement_tree, local_scope);
                    printf("\tpop rax\n");
                    printf("\tmov rax, [rax]\n");
                    printf("\tpush rax\n");
                    break;
                default:
                    fprintf(stderr, "Unexpected arity %d for expression of type %d\n", statement_tree->arity, statement_tree->ty);
                    exit(CODEGEN_ERROR);
            }
    } 
}
