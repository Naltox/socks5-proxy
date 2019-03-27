//
// Created by Narek Abovyan on 2019-03-24.
//

#ifndef C_PLAYGROUND_BUFFER_CHAIN_H
#define C_PLAYGROUND_BUFFER_CHAIN_H

typedef struct buff_chain {
    char* start;
    char* end;
    char* rptr;
    char* wptr;
    struct buff_chain* next;
} buff_chain;

buff_chain* buff_chain_new(int block_size);

//
//  Returns new tail of chain
//
buff_chain* buff_chain_write(buff_chain* chain, char* data, int size);

int buff_chain_length(buff_chain* chain);

void buff_chain_free_block(buff_chain* chain);

void buff_chain_free(buff_chain* chain);

#endif //C_PLAYGROUND_BUFFER_CHAIN_H
