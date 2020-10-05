#ifndef MEM_H
#define MEM_H

#include <stddef.h>

typedef unsigned char byte;

extern byte* fmd_memmem(byte* haystack, size_t haystack_len, const byte* const needle, const size_t needle_len);
extern void double_to_bebytes(byte bytes[sizeof(double)], double v_double);

#endif