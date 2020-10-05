#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argtable3.h"
#include "config.h"
#include "endian.h"
#include "fs.h"
#include "mem.h"

#define BUF_SIZE 512

const byte DURATION_IDENTIFIER[2] = {0x44, 0x89};
const byte MATROSKA_ID[4] = {0x1A, 0x45, 0xDF, 0xA3};

struct arg_lit *help, *version;
struct arg_dbl* ms;
struct arg_file *o, *file;
struct arg_end* end;

unsigned handle_args(void* argtable, const char* const progname, int argc, char** argv) {
	unsigned nerrors = arg_parse(argc, argv, argtable);
	unsigned exitcode = 0;

	if (help->count > 0) {
		printf("Usage: %s", progname);
		arg_print_syntax(stdout, argtable, "\n");
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
	}

	if (version->count > 0) {
		puts("Version: " VERSION);
	}

	if (nerrors > 0) {
		arg_print_errors(stdout, end, progname);
		printf("Try '%s --help' for more information.\n", progname);
		exitcode = 1;
	}

	return exitcode;
}

int main(int argc, char** argv) {
#ifdef _WIN32
	/* Warns if not run in the command prompt, closes after 10 seconds */
	HWND console_window = GetConsoleWindow();
	DWORD cw_process_id;
	GetWindowThreadProcessId(console_window, &cw_process_id);
	if (GetCurrentProcessId() == cw_process_id) {
		puts("This program is meant to be run in the command prompt.");
		Sleep(10 * 1000);
		return 1;
	}
#endif

	void* argtable[] = {
		help = arg_litn("h", "help", 0, 1, "display this help and exit"),
		version = arg_litn("v", "version", 0, 1, "display version info and exit"),
		o = arg_filen("o", "output", "faked-<file>", 0, 1, "output file"),
		file = arg_filen(NULL, NULL, "<file>", 1, 1, "input file"),
		ms = arg_dbln("d", "duration", "<ms>", 1, 1, "faked duration of video"),
		end = arg_end(20),
	};

	/* initialized here as exit tries to free it */
	FILE* fp = NULL;
	const char* progname = fs_basename(argv[0]);

	unsigned exitcode = handle_args(argtable, progname, argc, argv);
	if (exitcode > 0) {
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
		fputs("File too small\n", stderr);
		exitcode = 1;
		goto exit;
	}

	/* Read only first 512 bytes
	   I'm not sure if it's possible for the duration identifier to be located after the first 512. */
	byte buf[BUF_SIZE];
	fread(buf, sizeof buf, sizeof(byte), fp);
	fclose(fp);

	/* Search for Matroska identifier at the start of the file */
	if (!fmd_memmem(buf, sizeof MATROSKA_ID, MATROSKA_ID, sizeof MATROSKA_ID)) {
		fputs("Input file doesn't appear to be Matroska.\n", stderr);
		exitcode = 1;
		goto exit;
	}

	/* pointer to beginning of identifier */
	byte* addr = fmd_memmem(buf, sizeof buf, DURATION_IDENTIFIER, sizeof DURATION_IDENTIFIER);
	if (!addr) {
		fputs("Could not find duration identifier in first 512 bytes. If you're sure this is a bug, please open an issue at https://github.com/geniiii/fake-matroska-duration.\n", stderr);
		exitcode = 2;
		goto exit;
	}
	/* + identifier + size = offset of duration (double) */
	addr += sizeof DURATION_IDENTIFIER + sizeof(byte);

	/* Convert fake duration (double) to big-endian array of bytes */
	byte ms_as_bytes[sizeof(double)];
	double_to_bebytes(ms_as_bytes, *ms->dval);
	memcpy(addr, ms_as_bytes, sizeof(double));

	if (o->count == 0) {
		/* strlen includes null terminator */
		char* faked_filename = malloc(sizeof "faked-" + strlen(*file->basename));
		if (!faked_filename) {
			fputs("Out of memory?!\n", stderr);
			exitcode = 1;
			goto exit;
		}

		sprintf(faked_filename, "faked-%s", *file->basename);

		fprintf(stderr, "No output filename passed, attempting to use %s...\n", faked_filename);

		fs_copy(filename, faked_filename);
		fp = fopen(faked_filename, "r+b");

		free(faked_filename);
	} else {
		fs_copy(filename, *o->filename);
		fp = fopen(*o->filename, "r+b");
	}

	if (!fp) {
		fputs("Failed to create faked video\n", stderr);
		exitcode = 1;
		goto exit;
	}
	const size_t written = fwrite(buf, sizeof(byte), 512, fp);
	if (written != 512) {
		fputs("Number of bytes written differs from original size\n", stderr);
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
