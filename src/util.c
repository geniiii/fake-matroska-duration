#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "util.h"
#include <string.h>

/* TODO: error handling, maybe add *BSD? */
void fs_copy(const char* const in_f, const char* const out_f) {
#ifdef _WIN32
	CopyFileA(in_f, out_f, 0);
#elif __APPLE__
#include <copyfile.h>
	copyfile(in_f, out_f, NULL, COPYFILE_DATA);
#elif __linux
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
	struct stat stat_buf;
	int in_fd, out_fd;
	offset_t offset = 0;

	in_fd = open(in_f, O_RDONLY);
	fstat(in_fd, &stat_buf);
	out_fd = open(out_f, O_WRONLY | O_CREAT, stat_buf.st_mode);

	sendfile(out_fd, in_fd, &offset, stat_buf.st_size);

	close(out_fd);
	close(in_fd);
#else
	FILE* ifp = fopen(in_f, "rb");
	FILE* ofp = fopen(out_f, "wb");

	byte buffer[BUF_SIZE];
	size_t bytes;
	while (0 < (bytes = fread(buffer, 1, sizeof buffer, ifp))) {
		fwrite(buffer, 1, bytes, ofp);
	}

	fclose(ifp);
	fclose(ofp);
#endif
}

/* I hope no (supported) OS ever uses anything different.. */
const char* fs_basename(const char* path) {
	const char* base = strrchr(path, '/');
	if (!base) {
		base = strrchr(path, '\\');
	}
	return base ? base + 1 : path;
}

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
