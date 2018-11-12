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
    declare_variable(top_level_scope, "foo");
    declare_variable(top_level_scope, "bar");

    // We should expect to retrieve variables that we've declared
    expect(__LINE__, 2, (long)top_level_scope->variables_declared->keys->len);
    expect(__LINE__, 4, (long)map_get(top_level_scope->variables_declared, "bar"));

    Scope *child_scope = new_scope(top_level_scope);
    Scope *second_child_scope = new_scope(top_level_scope);

    // We should expect to be able to find our newly created scopes again in the future
    expect(__LINE__, 2, top_level_scope->sub_scopes->len);

    // We should expect to not have variables declared again in children scopes
    declare_variable(child_scope, "foo");
    declare_variable(child_scope, "bar");
    expect(__LINE__, -1, (long)map_get(child_scope->variables_declared, "bar"));

    // We should expect two scopes at equal levels on the scope hierarchy to both be allowed to have the same variables
    declare_variable(child_scope, "buzz");
    declare_variable(child_scope, "bazz");
    declare_variable(second_child_scope, "buzz");
    declare_variable(second_child_scope, "bazz");
    expect(__LINE__, 4, (long)map_get(child_scope->variables_declared, "bazz"));
    expect(__LINE__, 4, (long)map_get(second_child_scope->variables_declared, "bazz"));
}

void run_test() {
    test_vector();
    test_map();
    test_scope();
    printf("OK\n");
}