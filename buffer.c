//
// Created by Narek Abovyan on 13/05/2018.
//

#include "buffer.h"
#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <assert.h>

#define INITIAL_BUFFER_SIZE 1024
#define SCALE_FACTOR 1.5

buffer* buffer_new() {
    buffer* b = malloc(sizeof(buffer));
    b->size = INITIAL_BUFFER_SIZE;
    b->used = 0;
    b->data = malloc(INITIAL_BUFFER_SIZE * sizeof(char));
    return b;
}

void buffer_free(buffer* b) {
    free(b->data);
    free(b);
}

void buffer_write(buffer* b, char* data) {
    assert(data);

    int need = sizeof(char) * strlen(data);

    buffer_write_with_known_size(b, data, need);
}

void buffer_write_with_known_size(buffer* b, char* data, int size) {
    assert(size > 0);
    assert(data);

    if ((b->used + size) > b->size) {
        b->size *= SCALE_FACTOR;
        b->size += size;
        b->data = realloc(b->data, b->size * sizeof(char));
    }

    memcpy(b->data + b->used, data, size);
    b->used += size;
}

char* buffer_get_data(buffer* b) {
    return b->data;
}

void buffer_print(buffer* b) {
    char buff[b->used + 1];

    memcpy(buff, b->data, b->used);

    buff[b->used] = '\0';

    printf("%s\n", buff);
}

int buffer_get_size(buffer* b) {
    return b->used;
}

void buffer_clean(buffer* b) {
    memset(b->data, 0, b->size);
    b->used = 0;
}

void buffer_need_bytes(buffer* b, int need) {
    assert(need >= 0);

    if (need == 0)
        return;

    if ((b->used + need) > b->size) {
        b->size *= SCALE_FACTOR;
        b->size += need;
        b->data = realloc(b->data, b->size * sizeof(char));
    }
}

char* buffer_cur_data(buffer* b) {
    return b->data + b->used;
}

void buffer_add_used(buffer* b, int add) {
    b->used += add;
}