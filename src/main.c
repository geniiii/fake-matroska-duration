#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "argtable3.c"
#include "endian.h"

#define VERSION "1.0.0"
#define BUF_SIZE 512

#define MATROSKA "matroska"
#define WEBM "webm"

typedef unsigned char byte;

const byte durationIdentifier[2] = {0x44, 0x89};

struct arg_lit *help, *version;
struct arg_dbl* ms;
struct arg_file *o, *file;
struct arg_end* end;

/* EBML uses big-endian, while most CPUs are little/bi-endian.
   This (should) converts to big-endian. */
void doubleToBytes(byte bytes[sizeof(double)], double doubleVariable) {
#ifdef __BYTE_ORDER == __LITTLE_ENDIAN
	byte lEndianBytes[8];
	memcpy(lEndianBytes, (byte*)(&doubleVariable), sizeof(double));
	bytes[0] = lEndianBytes[7];
	bytes[1] = lEndianBytes[6];
	bytes[2] = lEndianBytes[5];
	bytes[3] = lEndianBytes[4];
	bytes[4] = lEndianBytes[3];
	bytes[5] = lEndianBytes[2];
	bytes[6] = lEndianBytes[1];
	bytes[7] = lEndianBytes[0];
#else
	memcpy(bytes, (byte*)(&doubleVariable), sizeof(double));
#endif
}

/* I hope no (supported) OS ever uses anything different.. */
const char* portable_ish_basename(const char* path) {
	char* base = strrchr(path, '/');
	if (!base) {
		base = strrchr(path, '\\');
	}
	return base ? base + 1 : path;
}

/* TODO: Fallback using fwrite, includes for Darwin/Linux, maybe add *BSD? */
void copy(const char* inf, const char* outf) {
#ifdef _WIN32
	CopyFileA(inf, outf, 0);
#elif __APPLE__
	copyfile(inf, outf, NULL, COPYFILE_DATA);
#elif __linux
	struct stat stat_buf;
	int in_fd, out_fd;
	offset_t offset = 0;

	in_fd = open(inf, O_RDONLY);
	fstat(in_fd, &stat_buf);
	out_fd = open(outf, O_WRONLY | O_CREAT, stat_buf.st_mode);

	sendfile(out_fd, in_fd, &offset, stat_buf.st_size);

	close(out_fd);
	close(in_fd);
#endif
}
/* TODO: Only define our own on compilers that don't have it */
byte* memmem(const byte* haystack, size_t haystack_len,
			 const byte* const needle, const size_t needle_len) {
	if (haystack == NULL)
		return NULL;  // or assert(haystack != NULL);
	if (haystack_len == 0)
		return NULL;
	if (needle == NULL)
		return NULL;  // or assert(needle != NULL);
	if (needle_len == 0)
		return NULL;

	for (const char* h = haystack;
		 haystack_len >= needle_len;
		 ++h, --haystack_len) {
		if (!memcmp(h, needle, needle_len)) {
			return h;
		}
	}
	return NULL;
}

int main(int argc, char** argv) {
	void* argtable[] = {
		help = arg_litn("h", "help", 0, 1, "display this help and exit"),
		version = arg_litn("v", "version", 0, 1, "display version info and exit"),
		o = arg_filen("o", "output", "faked-<file>", 0, 1, "output file"),
		file = arg_filen(NULL, NULL, "<file>", 1, 1, "input file"),
		ms = arg_dbln(NULL, NULL, "<ms>", 1, 1, "faked length of video"),
		end = arg_end(20),
	};

	int exitcode = 0;
	int nerrors = arg_parse(argc, argv, argtable);
	const char* progname = portable_ish_basename(argv[0]);

	/* initialized here as exit tries to free it */
	FILE* fp = NULL;

	if (help->count > 0) {
		printf("Usage: %s", progname);
		arg_print_syntax(stdout, argtable, "\n");
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		goto exit;
	}

	if (version->count > 0) {
		puts("Version: " VERSION);
		goto exit;
	}

	if (nerrors > 0) {
		arg_print_errors(stdout, end, progname);
		printf("Try '%s --help' for more information.\n", progname);
		exitcode = 1;
		goto exit;
	}

	const char* filename = *file->filename;

	fp = fopen(filename, "rb");
	if (!fp) {
		fputs("Failed to open original file\n", stderr);
		exitcode = 1;
		goto exit;
	}

	fseek(fp, 0, SEEK_END);
	const size_t size = ftell(fp);
	rewind(fp);

	if (size <= BUF_SIZE) {
		fputs("File too small", stderr);
		exitcode = 1;
		goto exit;
	}

	/* Read only first 512 bytes
	   It's doubtful that the duration identifier will be located after the first 512. */
	byte buf[BUF_SIZE];
	fread(buf, sizeof buf, sizeof(byte), fp);
	fclose(fp);

	if (!memmem(buf, sizeof buf, MATROSKA, sizeof MATROSKA - 1) && !memmem(buf, sizeof buf, WEBM, sizeof WEBM - 1)) {
		fputs("Input file doesn't appear to be Matroska. Please use WebM/MKV.", stderr);
		exitcode = 1;
		goto exit;
	}

	/* beginning of identifier */
	byte* addr = memmem(buf, sizeof buf, durationIdentifier, sizeof durationIdentifier);
	if (!addr) {
		fputs("Could not find duration identifier in first 512 bytes. If you're sure this is a bug, please open an issue at https://github.com/geniiii/fake-matroska-duration.", stderr);
		exitcode = 2;
		goto exit;
	}
	/* + identifier + size = offset of length (double) */
	addr += sizeof durationIdentifier + sizeof(byte);

	/* Convert new milliseconds (double) to big-endian array of bytes */
	byte msAsBytes[sizeof(double)];
	doubleToBytes(msAsBytes, *ms->dval);
	for (byte i = 0; i < sizeof(double); ++i) {
		addr[i] = msAsBytes[i];
	}

	if (o->count == 0) {
		/* TODO: This may be problematic when the input file is in a directory. */
		char* fakedFilename = malloc(sizeof "faked-" + strlen(*file->basename));
		sprintf(fakedFilename, "faked-%s", *file->basename);

		fprintf(stderr, "No output filename passed, attempting to use %s...\n", fakedFilename);

		copy(filename, fakedFilename);
		fp = fopen(fakedFilename, "r+b");

		free(fakedFilename);
	} else {
		copy(filename, *o->filename);
		fp = fopen(*o->filename, "r+b");
	}

	if (!fp) {
		fputs("Failed to create faked video\n", stderr);
		exitcode = 1;
		goto exit;
	}
	const size_t written = fwrite(buf, sizeof(byte), 512, fp);
	if (written != 512) {
		fputs("Number of bytes written differs from original size", stderr);
		exitcode = 2;
		goto exit;
	}

exit:
	if (fp) {
		fclose(fp);
	}
	arg_freetable(argtable, sizeof argtable / sizeof argtable[0]);
	return exitcode;
}
