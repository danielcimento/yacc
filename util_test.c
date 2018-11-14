#include "yacc.h"

void expect(int line, int expected, int actual) {
    if (expected == actual) {
        return;
    }
    fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, actual);
    exit(1);
}

void test_vector() {
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (long i = 0; i < 100; i++) {
        vec_push(vec, (void *)i);
    }

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (long)vec->data[0]);
    expect(__LINE__, 50, (long)vec->data[50]);
    expect(__LINE__, 99, (long)vec->data[99]);
}

void test_map() {
    Map *map = new_map(NULL);
    expect(__LINE__, 0, (long)map_get(map, "foo"));

    Map *map2 = new_map((void *)(long)-1);
    expect(__LINE__, -1, (long)map_get(map2, "foo"));

    map_put(map, "foo", (void *)2);
    expect(__LINE__, 2, (long)map_get(map, "foo"));

    map_put(map, "bar", (void *)4);
    expect(__LINE__, 4, (long)map_get(map, "bar"));

    map_put(map, "foo", (void *)6);
    expect(__LINE__, 6, (long)map_get(map, "foo"));
}

void test_scope() {
    Scope *top_level_scope = new_scope(NULL);
    declare_variable(top_level_scope, "bar");

    // We should expect to retrieve variables that we've declared
    expect(__LINE__, 1, (long)top_level_scope->variables_declared->keys->len);

    VariableAddress *bar_location = get_variable_location(top_level_scope, "bar");
    expect(__LINE__, 8, bar_location->offset);
    expect(__LINE__, 0, bar_location->scopes_up);

    Scope *child_scope = new_scope(top_level_scope);
    Scope *second_child_scope = new_scope(top_level_scope);

    // We should expect to be able to find our newly created scopes again in the future
    expect(__LINE__, 2, top_level_scope->sub_scopes->len);

    // We should expect to not have variables declared again in children scopes
    declare_variable(child_scope, "bar");
    VariableAddress *bar_from_child_scope = get_variable_location(child_scope, "bar");
    expect(__LINE__, 1, bar_from_child_scope->scopes_up);
    expect(__LINE__, 8, bar_from_child_scope->offset);
    expect(__LINE__, -1, (long)map_get(child_scope->variables_declared, "bar"));

    // We should expect two scopes at equal levels on the scope hierarchy to both be allowed to have the same variables
    declare_variable(child_scope, "bazz");
    declare_variable(second_child_scope, "bazz");
    VariableAddress *bazz_location = get_variable_location(child_scope, "bazz");
    VariableAddress *bazz_location2 = get_variable_location(child_scope, "bazz");
    expect(__LINE__, 0, bazz_location->scopes_up);
    expect(__LINE__, 8, bazz_location->offset);
    expect(__LINE__, 0, bazz_location2->scopes_up);
    expect(__LINE__, 8, bazz_location2->offset);
}

void test_scope_resolution() {
    char *example_code = "foo = 2; bar = 3; {i = 0; i + 1; {bar = 3; buzz = 2;}} {i = 0; bar = 2;}";

    char *filename = "yacc_temp.yacc";
    if(access(filename, F_OK) != -1) {
        fprintf(stderr, "Please delete any files named \"yacc_temp.yacc\" when using literal input.\n");
        exit(EXTERNAL_ERROR);
    }
    FILE *input_file = fopen(filename, "w");
    fprintf(input_file, "%s", example_code);
    fclose(input_file);

    Vector *tokens = tokenize(fopen(filename, "r"));
    Scope *generated_scope = construct_scope_from_token_stream(tokens);

    expect(__LINE__, 2, generated_scope->sub_scopes->len);
    VariableAddress *bar_location = get_variable_location(generated_scope, "bar");
    expect(__LINE__, 0, bar_location->scopes_up);
    expect(__LINE__, 16, bar_location->offset);

    Scope *sub_scope = (Scope *)generated_scope->sub_scopes->data[0];
    expect(__LINE__, 1, sub_scope->sub_scopes->len);
    VariableAddress *bar_location2 = get_variable_location(sub_scope, "bar");
    expect(__LINE__, 1, bar_location2->scopes_up);
    expect(__LINE__, 16, bar_location2->offset);

    Scope *sub_sub_scope = (Scope *)sub_scope->sub_scopes->data[0];
    expect(__LINE__, 0, sub_sub_scope->sub_scopes->len);
    VariableAddress *bar_location3 = get_variable_location(sub_sub_scope, "bar");
    expect(__LINE__, 2, bar_location3->scopes_up);
    expect(__LINE__, 16, bar_location3->offset);

    remove(filename);
}

void run_test() {
    test_vector();
    test_map();
    test_scope();
    test_scope_resolution();
    printf("OK\n");
}