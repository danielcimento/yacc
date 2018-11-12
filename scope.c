#include "yacc.h"

// Creates and returns a new scope. If the parent scope was passed in, 
// adds a new reference to this scope in it's sub_scopes variable 
Scope *new_scope(Scope *parent_scope) {
    Scope *scope = malloc(sizeof(Scope));
    scope->sub_scopes = new_vector();
    scope->variables_declared = new_map((void *)(long)-1);
    scope->parent_scope = parent_scope;
    scope->scopes_traversed = 0;

    if(parent_scope != NULL) {
        vec_push(parent_scope->sub_scopes, (void *)scope);
    }
    return scope;
}

// Check if the variable in question has already been declared in this scope or a scope above it
bool variable_already_declared(Scope *target_scope, char *variable_name) {
    if ((long)map_get(target_scope->variables_declared, variable_name) != -1) {
        return true;
    }

    if(target_scope->parent_scope == NULL) {
        return false;
    } else {
        return variable_already_declared(target_scope->parent_scope, variable_name);
    }
}

void declare_variable(Scope *target_scope, char *variable_name) {
    if (variable_already_declared(target_scope, variable_name)) {
        return;
    } else {
        // TODO: Eventually add support for types larger than 8 bytes
        map_put(target_scope->variables_declared, variable_name, (void *)(long)(target_scope->variables_declared->keys->len * 8));
    }
}

VariableAddress *gvl_helper(Scope *current_scope, char *variable_name, int scopes_climbed) {
    if(current_scope == NULL) {
        fprintf(stderr, "Use of undeclared variable %s.\n", variable_name);
        exit(SCOPE_ERROR);
    }

    int lookup_in_current_scope = (long)map_get(current_scope->variables_declared, variable_name);
    if (lookup_in_current_scope != -1) {
        VariableAddress *new_address = malloc(sizeof(VariableAddress));
        new_address->offset = lookup_in_current_scope;
        new_address->scopes_up = scopes_climbed;
        return new_address;
    } else {
        return gvl_helper(current_scope->parent_scope, variable_name, scopes_climbed + 1);
    }
}

VariableAddress *get_variable_location(Scope *current_scope, char *variable_name) {
    return gvl_helper(current_scope, variable_name, 0);
}

Scope *construct_scope_from_token_stream(Vector *tokens) {
    Scope *current_scope = new_scope(NULL);

    // Loop through all the tokens
    for (int pos = 0; tokens->data[pos]; pos++) {
        Token tk = *((Token *)tokens->data[pos]);
        // Every time we have an open brace, open a new child scope
        if(tk.ty == '{') {
            current_scope = new_scope(current_scope);
        // If we've closed the scope, try to go up one level in the scope.
        } else if(tk.ty == '}') {
            current_scope = current_scope->parent_scope;
            if (current_scope == NULL) {
                fprintf(stderr, "Mismatched braces!\n");
                exit(SCOPE_ERROR);
            }
        // When we find the identifier, try to create it in the current scope.
        } else if(tk.ty == TK_IDENT) {
            declare_variable(current_scope, tk.name);
        }
    }

    if(current_scope->parent_scope != NULL) {
        fprintf(stderr, "Mismatched braces!\n");
        exit(SCOPE_ERROR);
    }
    return current_scope;
}

Scope *get_next_child_scope(Scope *current_scope) {
    return (Scope *)current_scope->sub_scopes->data[current_scope->scopes_traversed];
}