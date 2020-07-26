#ifdef _WIN32 // Windows
#include <Windows.h>
#include <WinBase.h>
#include <tchar.h>
#include <wchar.h>
#else // POSIX

#endif

#define VERSION "0.0.0"
#define INCLUDE_TESTS 1

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "utils.h"
#include "vvd.h"
#include "platform.h"

#ifdef INCLUDE_TESTS
#include <assert.h>
#include "fs/gpt.h"
#include "fs/mbr.h"

//
// test
//

void test() {
	fputs(
	"* Defines\n"
#ifdef __LITTLE_ENDIAN__
	"__LITTLE_ENDIAN__\n"
#endif
#ifdef __BIG_ENDIAN__
	"__BIG_ENDIAN__\n"
#endif
#ifdef __NO_INLINE__
	"__NO_INLINE__\n"
#endif
	, stdout);
#ifdef _ILP32
	printf("_ILP32		%u\n", _ILP32);
#endif
#ifdef __SIZE_WIDTH__
	printf("__SIZE_WIDTH__		%u\n", __SIZE_WIDTH__);
#endif
	printf(
		"sizeof	VDISK		%u\n"
		"sizeof	wchar_t		%u\n",
		(int)sizeof(VDISK),
		(int)sizeof(wchar_t)
	);
	puts("Running tests...");
	assert(sizeof(MBR) == 512);
	assert(sizeof(MBR_PARTITION_ENTRY) == 16);
	assert(sizeof(CHS_ENTRY) == 3);
	assert(sizeof(GPT) == 512);
	assert(sizeof(GPT_ENTRY) == 512);
	assert(sizeof(LBA64) == 8);
	// VDI
	assert(sizeof(VDI_HDR) == 8);
	assert(sizeof(VDIDISKGEOMETRY) == 16);
	assert(sizeof(VDIHEADER0) == 348);
	assert(sizeof(VDIHEADER1) == 400);
	// VMDK
	assert(sizeof(VMDK_HDR) == 512);
	assert(sizeof(VMDK_MARKER) == 512);
	// VHD
	assert(sizeof(VHD_HDR) == 512);
	assert(sizeof(VHD_PARENT_LOCATOR) == 24);
	assert(sizeof(VHD_DYN_HDR) == 1024);
	// VHDX
	assert(sizeof(VHDX_HDR) == 520);
	assert(sizeof(VHDX_HEADER1) == 76);
	assert(sizeof(VHDX_REGION_HDR) == 16);
	assert(sizeof(VHDX_REGION_ENTRY) == 32);
	assert(sizeof(VHDX_LOG_HDR) == 64);
	assert(sizeof(VHDX_LOG_ZERO) == 32);
	assert(sizeof(VDHX_LOG_DESC) == 32);
	assert(sizeof(VHDX_LOG_DATA) == 4100);
	// utils
	assert(bswap16(0xAABB) == 0xBBAA);
	assert(bswap32(0xAABBCCDD) == 0xDDCCBBAA);
	assert(bswap64(0xAABBCCDD11223344) == 0x44332211DDCCBBAA);
#ifdef _WIN32
	assert(extcmp(L"test.bin", L"bin"));
#else
	assert(extcmp("test.bin", "bin"));
#endif
	puts("OK");
	exit(EXIT_SUCCESS);
}
#endif // INCLUDE_TESTS

//
// CLI part
//

void help() {
	puts(
	"Manage virtual disks\n"
	"  Usage: vvd OPERATION FILE [...]\n"
	"         vvd PAGE\n"
	"\nOPERATIONS\n"
	"  -I    Info    - Get vdisk image information\n"
	"  -N    New     - Create new empty vdisk\n"
	"  -M    Map     - Show allocation map\n"
	"  -C    Compact - Compact vdisk image\n"
	"\nOPTIONS\n"
	"   r    Open as RAW\n"
	"\nPAGE\n"
	"  --help       Print this help screen and quit\n"
	"  --version    Print version screen and quit\n"
	"  --license    Print license screen and quit\n"
	);
	exit(EXIT_SUCCESS);
}

void version(void) {
	printf(
	"vvd-" PLATFORM " " VERSION 
#ifdef TIMESTAMP
	" (" TIMESTAMP ")"
#endif
	"\n"
#ifdef __VERSION__
	"Compiler: " __VERSION__ "\n"
#endif
	"MIT License: Copyright (c) 2019 dd86k\n"
	"Project page: <https://github.com/dd86k/vvd>\n"
	"VDISK   OPERATIONS\n"
	"VDI     I M N C\n"
	"VMDK    I\n"
	"VHD     I M\n"
	"VHDX    \n"
	"QED     \n"
	"QCOW    \n"
	"VHD     \n"
	"PHDD    \n"
	"RAW     I\n"
	);
	exit(EXIT_SUCCESS);
}

void license() {
	puts(
	"Copyright (c) 2019-2020 dd86k <dd@dax.moe>\n"
	"\n"
	"Permission is hereby granted, free of charge, to any person obtaining a copy of\n"
	"this software and associated documentation files (the \"Software\"), to deal in\n"
	"the Software without restriction, including without limitation the rights to\n"
	"use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies\n"
	"of the Software, and to permit persons to whom the Software is furnished to do\n"
	"so, subject to the following conditions:\n"
	"\n"
	"The above copyright notice and this permission notice shall be included in all\n"
	"copies or substantial portions of the Software.\n"
	"\n"
	"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
	"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
	"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
	"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
	"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
	"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
	"SOFTWARE."
	);
	exit(EXIT_SUCCESS);
}

/**
 * Extension vdisk matcher, returns VDISK_FORMAT if matches an extension.
 * Otherwise 0.
 */
int vdextauto(_vchar *path) {
	if (extcmp(path, EXT_VDI))	return VDISK_FORMAT_VDI;
	if (extcmp(path, EXT_VMDK))	return VDISK_FORMAT_VMDK;
	if (extcmp(path, EXT_VHD))	return VDISK_FORMAT_VHD;
	return 0;
}

#ifdef _WIN32
#define MAIN int wmain(int argc, wchar_t **argv)
#define s(quote) L##quote
#define PSTR "%ls"
int scmp(const wchar_t *s, const wchar_t *t) {
	return wcscmp(s, t) == 0;
}
#else
#define MAIN int main(int argc, char **argv)
#define s(quote) ##quote
#define PSTR "%s"
int scmp(const char *s, const char *t) {
	return strcmp(s, t) == 0;
}
#endif

// Main entry point. This only performs intepreting the command-line options
// for the core functions.
MAIN {
	if (argc <= 1)
		help();

	int mflags = 0;	// main: Command-line flags
	int oflags = 0;	// vdisk_open: file flags
	int cflags = 0;	// vdisk_create: file flags
	VDISK vdin;	// vdisk IN
	VDISK vdout;	// vdisk OUT
	uint64_t vsize;	// virtual disk size, used in 'new' and 'resize'
	const _vchar *file_input = NULL;
	const _vchar *file_output = NULL;

	// Additional arguments are processed first, since they're simpler
	for (size_t argi = 2; argi < argc; ++argi) {
		const _vchar *arg = argv[argi];
		//
		// Open flags
		//
		if (scmp(arg, s("--raw"))) {
			oflags |= VDISK_RAW;
			continue;
		}
		//
		// Create flags
		//
		if (scmp(arg, s("--create-raw"))) {
			cflags |= VDISK_RAW;
			continue;
		}
		if (scmp(arg, s("--create-dynamic"))) {
			cflags |= VDISK_CREATE_DYN;
			continue;
		}
		if (scmp(arg, s("--create-fixed"))) {
			cflags |= VDISK_CREATE_FIXED;
			continue;
		}
		//
		// Default arguments
		//
		if (file_input == NULL) {
			file_input = arg;
			continue;
		}
		if (file_output == NULL) {
			file_output = arg;
			continue;
		}
		//
		// Unknown argument
		//
		fprintf(stderr, "main: '" PSTR "' unknown option\n", arg);
		return EXIT_FAILURE;
	}

	const _vchar *action = argv[1];

	// Action time
	if (scmp(action, s("info"))) {
		if (file_input == NULL) {
			fputs("main: missing vdisk", stderr);
			return EXIT_FAILURE;
		}
		if (vdisk_open(file_input, &vdin, oflags)) {
			vdisk_perror(__func__);
			return vdisk_errno;
		}
		return vvd_info(&vdin);
	}
	if (scmp(action, s("map"))) {
		fputs("main: missing vdisk", stderr);
		return EXIT_FAILURE;
	}
	if (scmp(action, s("compact"))) {
		fputs("main: missing vdisk", stderr);
		return EXIT_FAILURE;
	}
	if (scmp(action, s("resize"))) {
		fputs("main: not implemented", stderr);
		return EXIT_FAILURE;
	}
	if (scmp(action, s("defrag"))) {
		fputs("main: not implemented", stderr);
		return EXIT_FAILURE;
	}
	if (scmp(action, s("new"))) {
		fputs("main: not implemented", stderr);
		/*if (argc < 4) // Needs vvd -N TYPE SIZE
			goto L_MISSING_ARGS;

		fargi = 2;
		oflags |= VDISK_CREATE;

		vdin.format = vdextauto(argv[2]);
		if (vdisk_default(&vdin)) {
			vdisk_perror(__func__);
			return vdisk_errno;
		}

		if (sbinf(argv[3], &vsize)) {
			fputs("main: Invalid binary size, must be higher than 0\n", stderr);
			return ECLIARG;
		}
		*/
		return EXIT_FAILURE;
	}
	if (scmp(action, s("version")) || scmp(action, s("--version")))
		version();
	if (scmp(action, s("help")) || scmp(action, s("--help")))
		help();
	if (scmp(action, s("--license")))
		license();
#ifdef INCLUDE_TESTS
	if (scmp(action, s("test")))
		test();
#endif

	fprintf(stderr, "main: '" PSTR "' unknown action, see 'vvd help'\n", action);
	return EXIT_FAILURE;
}
