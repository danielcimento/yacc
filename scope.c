#include "yacc.h"

// Creates and returns a new scope. If the parent scope was passed in, 
// adds a new reference to this scope in it's sub_scopes variable 
Scope *new_scope(Scope *parent_scope) {
    Scope *scope = malloc(sizeof(Scope));
    scope->sub_scopes = new_vector();
    scope->variables_declared = new_map((void *)(long)-1);
    scope->parent_scope = parent_scope;

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
        map_put(target_scope->variables_declared, variable_name, (void *)(long)(target_scope->variables_declared->keys->len * 4));
    }
}
