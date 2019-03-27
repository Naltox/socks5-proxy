//
// Created by Narek Abovyan on 22/04/2018.
//

#ifndef C_PLAYGROUND_BUFF_UTILS_H
#define C_PLAYGROUND_BUFF_UTILS_H

#define SDV(x, y) x = (typeof (x))((char *)(x) + (y))

#define READ_CHAR(s, x) x = *s++;

#define READ_INT(s, x) x = *(int *)(s); SDV (s, sizeof (int))

#define READ_INT_16(s, x) x = *(int16_t *)(s); SDV (s, sizeof (int16_t))

#define WRITE_UINT(s, x) *(unsigned int *)(s) = x; SDV (s, sizeof (unsigned int))

#define WRITE_UINT_16(s, x) *(uint16_t *)(s) = x; SDV (s, sizeof (uint16_t))

#define WRITE_CHAR(s, x) *s++ = x;

#endif //C_PLAYGROUND_BUFF_UTILS_H
