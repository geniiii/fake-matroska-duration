#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>

typedef unsigned char byte;

extern void fs_copy(const char* const in_f, const char* const out_f);
extern const char* fs_basename(const char* path);

extern byte* fmd_memmem(byte* haystack, size_t haystack_len, const byte* const needle, const size_t needle_len);

extern void double_to_bebytes(byte bytes[sizeof(double)], double v_double);

#endif