#include "yacc.h"

Vector *new_vector() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector *vec, void *elem) {
    if(vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, (vec->capacity) * sizeof(void *));
    }
    vec->data[vec->len++] = elem;
}

Map *new_map(void *default_value) {
    Map *map = malloc(sizeof(Map));
    map->keys = new_vector();
    map->vals = new_vector();
    map->default_value = default_value;
    return map;
}

void map_put(Map *map, char *key, void *val) {
    vec_push(map->keys, key);
    vec_push(map->vals, val);
}

void *map_get(Map *map, char *key) {
    for(int i = map->keys->len - 1; i >= 0; i--) {
        if(strcmp(map->keys->data[i], key) == 0) {
            return map->vals->data[i];
        }
    }
    return map->default_value;
}