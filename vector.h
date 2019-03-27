//
// Created by Narek Abovyan on 21/05/2018.
//

//#ifndef C_PLAYGROUND_VECTOR_H
//#define C_PLAYGROUND_VECTOR_H

#include <stddef.h>

#define V_ADD_SUFF_(A, B) A##_##B
#define V_ADD_SUFF(A, B) V_ADD_SUFF_(A, B)

#if (!defined (V_TYPE) || !defined(V_NAME))
    #error V_TYPE or V_NAME is not declared
#endif

typedef struct {
    V_TYPE* data;
    size_t size;
    size_t used;
} V_NAME;

void V_ADD_SUFF(V_NAME, init) (V_NAME* v);

void V_ADD_SUFF(V_NAME, free) (V_NAME* v);

size_t V_ADD_SUFF(V_NAME, length) (V_NAME* v);

void V_ADD_SUFF(V_NAME, push) (V_NAME* v, V_TYPE val);

V_TYPE V_ADD_SUFF(V_NAME, get) (V_NAME* v, int index);

V_TYPE V_ADD_SUFF(V_NAME, get_unsafe) (V_NAME* v, int index);

V_TYPE V_ADD_SUFF(V_NAME, clean) (V_NAME* v);

//#endif //C_PLAYGROUND_VECTOR_H
