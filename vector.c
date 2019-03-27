//
// Created by Narek Abovyan on 21/05/2018.
//

#include <printf.h>
#include <stdlib.h>
#include <string.h>

#define STR_(D) #D
#define STR(D) STR_(D)

#define VEC_INITIAL_SIZE 1024
#define VEC_SCALE_FACTOR 1.5

void V_ADD_SUFF(V_NAME, init) (V_NAME* v) {
    v->size = VEC_INITIAL_SIZE;
    v->used = 0;
    v->data = malloc(VEC_INITIAL_SIZE * sizeof(V_TYPE));
}

void V_ADD_SUFF(V_NAME, free) (V_NAME* v) {
    free(v->data);
}

size_t V_ADD_SUFF(V_NAME, length) (V_NAME* v) {
    return v->used;
}

void V_ADD_SUFF(V_NAME, push) (V_NAME* v, V_TYPE val) {

    if ((v->used + 1) > v->size) {
        v->size *= VEC_SCALE_FACTOR;
        v->size += 1;
        v->data = realloc(v->data, v->size * sizeof(V_TYPE));
        printf("realloc, new size: %d\n", v->size);
    }

    v->data[v->used] = val;
    v->used++;
}

V_TYPE V_ADD_SUFF(V_NAME, get) (V_NAME* v, int index) {
    if (index > v->used)
        return NULL;

    return v->data[index];
}

V_TYPE V_ADD_SUFF(V_NAME, get_unsafe) (V_NAME* v, int index) {
    return v->data[index];
}

V_TYPE V_ADD_SUFF(V_NAME, clean) (V_NAME* v) {
    memset(v->data, 0, v->size);
    v->used = 0;
}