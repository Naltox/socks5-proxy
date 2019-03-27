//
// Created by Narek Abovyan on 13/05/2018.
//

#ifndef C_PLAYGROUND_BUFFER_H
#define C_PLAYGROUND_BUFFER_H

#include <stddef.h>

typedef struct buffer {
    char* data;
    size_t used;
    size_t size;
} buffer;


buffer* buffer_new();

void buffer_free(buffer* b);

void buffer_write(buffer* b, char* data);

void buffer_write_with_known_size(buffer* b, char* data, int size);

char* buffer_get_data(buffer* b);

void buffer_print(buffer* b);

int buffer_get_size(buffer* b);

void buffer_clean(buffer* b);

void buffer_need_bytes(buffer* b, int need);

char* buffer_cur_data(buffer* b);

void buffer_add_used(buffer* b, int add);

#endif //C_PLAYGROUND_BUFFER_H
