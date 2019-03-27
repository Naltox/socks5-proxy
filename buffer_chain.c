//
// Created by Narek Abovyan on 2019-03-24.
//

#include "buffer_chain.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

buff_chain* buff_chain_new(int block_size) {
    buff_chain* chain = malloc(sizeof(buff_chain));

    chain->start = malloc((size_t) block_size);
    chain->end = chain->start + block_size;
    chain->rptr = chain->wptr = chain->start;
    chain->next = NULL;

    return chain;
}

buff_chain* buff_chain_write(buff_chain* chain, char* data, int size) {
    buff_chain* block = chain;

    while(size > 0) {
        long free = block->end - block->wptr;
        if (free >= size) {
            memcpy(block->wptr, data, size);
            block->wptr += size;
            return block;
        } else {
            memcpy(block->wptr, data, free);
            block->wptr += free;
            data += free;
            size -= free;
            buff_chain* new_block = buff_chain_new((int) (block->end - block->start));
            block->next = new_block;
            block = new_block;
        }
    }

    return block;
}

int buff_chain_length(buff_chain* chain) {
    int chain_size = 0;
    while (chain != NULL) {
        chain_size++;
        chain = chain->next;
    }
    return chain_size;
}

void buff_chain_free_block(buff_chain *chain) {
    free(chain->start);
    free(chain);
}

void buff_chain_free(buff_chain* chain) {
    while (chain != NULL) {
        buff_chain* c = chain;
        chain = chain->next;
        buff_chain_free_block(c);
    }
}