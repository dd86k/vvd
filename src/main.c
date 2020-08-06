#ifdef _WIN32 // Windows
#include <Windows.h>
#include <WinBase.h>
#include <tchar.h>
#include <wchar.h>
#else // POSIX

#endif

#define INCLUDE_TESTS	1
#define PROJECT_VERSION	"0.0.0"

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
#ifdef ENDIAN_LITTLE
	"ENDIAN_LITTLE\n"
#endif
#ifdef ENDIAN_BIG
	"ENDIAN_BIG\n"
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
	"sizeof	wchar_t		%u\n"
	"Running tests...\n",
	(int)sizeof(VDISK),
	(int)sizeof(wchar_t)
	);
	assert(sizeof(MBR) == 512);
	assert(sizeof(MBR_PARTITION) == 16);
	assert(sizeof(CHS) == 3);
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
	struct progress_t p;
	assert(os_pinit(&p, PROG_MODE_NONE, 10) == 0);
	for (uint32_t i = 0; i <= 10; ++i) {
		os_pupdate(&p, i);
		Sleep(1000);
	}
	os_pfinish(&p);
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

static void help() {
	puts(
	"Manage virtual disks\n"
	"  Usage: vvd OPERATION [FILE [OPTIONS]]\n"
	"\nOPERATION\n"
	"  info       Get vdisk image information\n"
	"  new        Create new empty vdisk\n"
	"  map        Show allocation map\n"
	"  compact    Compact vdisk image\n"
	"  help       Show help page and exit\n"
	"  version    Show version page and exit\n"
	"  license    Show license page and exit\n"
	"\nOPTIONS\n"
	"  --raw           Open as RAW\n"
	"  --create-raw    Create as RAW\n"
	"  --create-dyn    Create vdisk as dynamic\n"
	"  --create-fixed  Create vdisk as fixed\n"
	);
	exit(EXIT_SUCCESS);
}

static void version(void) {
	printf(
	"vvd-" __PLATFORM__ " " PROJECT_VERSION " (" __DATE__ " " __TIME__ ")\n"
#ifdef __VERSION__
	"Compiler: " __VERSION__ "\n"
#endif
	"MIT License: Copyright (c) 2019-2020 dd86k <dd@dax.moe>\n"
	"Project page: <https://github.com/dd86k/vvd>\n\n"
	"VDISK   OPERATIONS\n"
	"VDI     info, map, new, compact\n"
	"VMDK    info\n"
	"VHD     info, map\n"
	"VHDX    \n"
	"QED     \n"
	"QCOW    \n"
	"VHD     \n"
	"PHDD    \n"
	"RAW     info\n"
	);
	exit(EXIT_SUCCESS);
}

static void license() {
	puts(
	"Copyright 2019-2020 dd86k <dd@dax.moe>\n"
	"\n"
	"Permission to use, copy, modify, and/or distribute this software for any\n"
	"purpose with or without fee is hereby granted, provided that the above\n"
	"copyright notice and this permission notice appear in all copies.\n"
	"\n"
	"THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH\n"
	"REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND\n"
	"FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,\n"
	"INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM\n"
	"LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR\n"
	"OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\n"
	"PERFORMANCE OF THIS SOFTWARE."
	);
	exit(EXIT_SUCCESS);
}

#ifdef _WIN32
#define MAIN int wmain(int argc, wchar_t **argv)
#define vstr(quote) L##quote
#define OSCHARFMT "%ls"
static int scmp(const wchar_t *s, const wchar_t *t) {
	return wcscmp(s, t) == 0;
}
#else
#define MAIN int main(int argc, char **argv)
#define vstr(quote) quote
#define OSCHARFMT "%s"
static int scmp(const char *s, const char *t) {
	return strcmp(s, t) == 0;
}
#endif

//TODO: Consider hashing strings for faster lookups
//	Either SuperFastHash [1] or xxHash [2]
//	[1] http://www.azillionmonkeys.com/qed/hash.html
//	[2] https://github.com/Cyan4973/xxHash

/**
 * Match a patch to an exception with the VDISK_FORMAT_* enum.
 * 
 * \returns VDISK_FORMAT enum or 0
 */
static int vdextauto(const _oschar *path) {
	if (extcmp(path, vstr("vdi")))	return VDISK_FORMAT_VDI;
	if (extcmp(path, vstr("vmdk")))	return VDISK_FORMAT_VMDK;
	if (extcmp(path, vstr("vhd")))	return VDISK_FORMAT_VHD;
	if (extcmp(path, vstr("vhdx")))	return VDISK_FORMAT_VHDX;
	return 0;
}

// Main entry point. This only performs intepreting the command-line options
// for the core functions.
MAIN {
	if (argc <= 1)
		help();

	uint32_t mflags = 0;	// main: Command-line flags
	uint32_t oflags = 0;	// vdisk_open: file flags
	uint32_t cflags = 0;	// vdisk_create: file flags
	VDISK vdin;	// vdisk IN
	VDISK vdout;	// vdisk OUT
	uint64_t vsize;	// virtual disk size, used in 'new' and 'resize'
	const _oschar *defopt1 = NULL; // Default option 1 (typically file input)
	const _oschar *defopt2 = NULL; // Default option 2 (typically file output/size)

	// Additional arguments are processed first, since they're simpler
	//TODO: --progress: shows progress bar whenever available
	//TODO: --verbose: prints those extra lines (>v0.10.0)
	for (size_t argi = 2; argi < argc; ++argi) {
		const _oschar *arg = argv[argi];
		//
		// Open flags
		//
		if (scmp(arg, vstr("--raw"))) {
			oflags |= VDISK_RAW;
			continue;
		}
		//
		// Create flags
		//
		if (scmp(arg, vstr("--create-raw"))) {
			cflags |= VDISK_RAW;
			continue;
		}
		if (scmp(arg, vstr("--create-dynamic"))) {
			cflags |= VDISK_CREATE_DYN;
			continue;
		}
		if (scmp(arg, vstr("--create-fixed"))) {
			cflags |= VDISK_CREATE_FIXED;
			continue;
		}
		//
		// Default arguments
		//
		if (defopt1 == NULL) {
			defopt1 = arg;
			continue;
		}
		if (defopt2 == NULL) {
			defopt2 = arg;
			continue;
		}
		//
		// Unknown argument
		//
		fprintf(stderr, "main: '" OSCHARFMT "' unknown option\n", arg);
		return EXIT_FAILURE;
	}

	const _oschar *action = argv[1];

	//
	// Operations
	//

	if (scmp(action, vstr("info"))) {
		if (defopt1 == NULL) {
			fputs("main: missing vdisk\n", stderr);
			return EXIT_FAILURE;
		}
		if (vdisk_open(&vdin, defopt1, oflags)) {
			vdisk_perror(&vdin);
			return vdin.errcode;
		}
		return vvd_info(&vdin, 0);
	}

	if (scmp(action, vstr("map"))) {
		if (defopt1 == NULL) {
			fputs("main: missing vdisk\n", stderr);
			return EXIT_FAILURE;
		}
		if (vdisk_open(&vdin, defopt1, oflags)) {
			vdisk_perror(&vdin);
			return vdin.errcode;
		}
		return vvd_map(&vdin, 0);
	}

	if (scmp(action, vstr("compact"))) {
		if (defopt1 == NULL) {
			fputs("main: missing vdisk\n", stderr);
			return EXIT_FAILURE;
		}
		if (vdisk_open(&vdin, defopt1, 0)) {
			vdisk_perror(&vdin);
			return vdin.errcode;
		}
		if (vvd_compact(&vdin, 0)) {
			vdisk_perror(&vdin);
			return vdin.errcode;
		}
		return EXIT_FAILURE;
	}

	if (scmp(action, vstr("defrag"))) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	if (scmp(action, vstr("new"))) {
		if (defopt1 == NULL) {
			fputs("main: missing path specifier\n", stderr);
			return EXIT_FAILURE;
		}
		if (defopt2 == NULL) {
			fputs("main: missing size specifier\n", stderr);
			return EXIT_FAILURE;
		}

		// Get vdisk type out of extension name
		int format = vdextauto(defopt1);
		if (format == 0) {
			fputs("main: could not determine vdisk type\n", stderr);
			return EXIT_FAILURE;
		}

		// Convert binary text (e.g. "10G") to a 64-bit number
		if (sbinf(defopt2, &vsize)) {
			fputs("main: invalid binary size\n", stderr);
			return EXIT_FAILURE;
		}

		return vvd_new(defopt1, format, vsize, cflags);
	}

	if (scmp(action, vstr("resize"))) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	if (scmp(action, vstr("repair"))) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	if (scmp(action, vstr("convert"))) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	//
	// Pages
	//

	if (scmp(action, vstr("ver"))) {
		puts(PROJECT_VERSION);
		return EXIT_SUCCESS;
	}
	if (scmp(action, vstr("version")) || scmp(action, vstr("--version")))
		version();
	if (scmp(action, vstr("help")) || scmp(action, vstr("--help")))
		help();
	if (scmp(action, vstr("license")) || scmp(action, vstr("--license")))
		license();
#ifdef INCLUDE_TESTS
	if (scmp(action, vstr("--test")))
		test();
#endif

	fprintf(stderr, "main: '" OSCHARFMT "' unknown operation, see 'vvd help'\n", action);
	return EXIT_FAILURE;
}
