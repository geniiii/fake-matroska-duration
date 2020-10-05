#include "fs.h"

#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

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
#include <fcntl.h>
	struct stat stat_buf;
	int in_fd, out_fd;
	off_t offset = 0;

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