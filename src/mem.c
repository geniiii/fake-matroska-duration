#include "mem.h"
#include <string.h>

byte* fmd_memmem(byte* haystack, size_t haystack_len,
				 const byte* const needle, const size_t needle_len) {
	if (haystack == NULL || haystack_len == 0) {
		return NULL;
	} else if (needle == NULL || needle_len == 0) {
		return NULL;
	}

	for (byte* h = haystack; haystack_len >= needle_len; ++h, --haystack_len) {
		if (!memcmp(h, needle, needle_len)) {
			return h;
		}
	}
	return NULL;
}

/* EBML uses big-endian, while most CPUs are little/bi-endian.
   This converts to big-endian on little-endian systems. */
void double_to_bebytes(byte bytes[sizeof(double)], double vdouble) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	byte lebytes[8];
	memcpy(lebytes, (byte*)(&vdouble), sizeof(double));
	bytes[0] = lebytes[7];
	bytes[1] = lebytes[6];
	bytes[2] = lebytes[5];
	bytes[3] = lebytes[4];
	bytes[4] = lebytes[3];
	bytes[5] = lebytes[2];
	bytes[6] = lebytes[1];
	bytes[7] = lebytes[0];
#else
	memcpy(bytes, (byte*)(&doubleVariable), sizeof(double));
#endif
}
