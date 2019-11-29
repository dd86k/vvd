#ifdef _WIN32
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
	"Copyright (c) 2019 dd86k\n"
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
#else
#define MAIN int main(int argc, char **argv)
#endif

//
// main
//

MAIN {
	if (argc <= 1)
		help();

	char op_mode;
	int oflags = 0;	// open file flags
	int cflags = 0;	// create file flags

	if (argv[1][1] == '-') { // long arguments
		_vchar *h = argv[1] + 2;
#ifdef _WIN32
		if (wcscmp(h, L"help") == 0)
			help();
		if (wcscmp(h, L"version") == 0)
			version();
		if (wcscmp(h, L"license") == 0)
			license();
#ifdef INCLUDE_TESTS
		if (wcscmp(h, L"test") == 0)
			test();
#endif // INCLUDE_TESTS
		fprintf(stderr, "main: \"%ls\" option unknown, aborting\n", h);
#else
		if (strcmp(h, "help") == 0)
			help();
		if (strcmp(h, "version") == 0)
			version();
		if (strcmp(h, "license") == 0)
			license();
#ifdef INCLUDE_TESTS
		if (strcmp(h, "test") == 0)
			test();
#endif // INCLUDE_TESTS
		fprintf(stderr, "main: \"%s\" option unknown, aborting\n", h);
#endif
		return ECLIARG;
	} else if (argv[1][0] == '-') { // short arguments
		_vchar *h = argv[1];
		while (*++h) switch (*h) {
		case MODE_INFO:	// -I
		case MODE_MAP:	// -M
		case MODE_NEW:	// -N
		case MODE_COMPACT:	// -C
			op_mode = *h;
			break;
		case 'r':
			oflags |= VDISK_OPEN_RAW;
			break;
		case 'c':
			cflags |= VDISK_OPEN_RAW;
			break;
		default:
			fprintf(stderr, "main: unknown parameter '%c', aborting\n", *h);
			return ECLIARG;
		}
	}

	VDISK vdin;	// vdisk IN
	VDISK vdout;	// vdisk OUT
	uint64_t vsize;	// virtual disk size, used in -N and -R

	size_t fargi;	// File IN argument index
	switch (op_mode) {
	/*case MODE_RESIZE:
		if (argc < 4)
			goto L_MISSING_ARGS;
		fargi = 3;
		break;*/
	case MODE_NEW:
		if (argc < 4) // Needs vvd -N TYPE SIZE
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

		if (argc > 4) {
#ifdef _WIN32
			if (wcscmp(argv[4], L"fixed") == 0)
				oflags |= VDISK_CREATE_FIXED;
			else if (wcscmp(argv[4], L"dynamic") == 0)
				oflags |= VDISK_CREATE_DYN;
#else
			if (strcmp(argv[4], "fixed") == 0)
				oflags |= VDISK_CREATE_FIXED;
			else if (strcmp(argv[4], "dynamic") == 0)
				oflags |= VDISK_CREATE_DYN;
#endif
		} // ELSE DEFAULT: dynamic (vdisk_open)
		break;
	case MODE_COMPACT:
	case MODE_INFO:
	case MODE_MAP:
		if (argc < 3) {
L_MISSING_ARGS:
			fputs("main: Missing parameters, aborting\n", stderr);
			return ECLIARG;
		}
		fargi = 2;
		break;
	default:
		fputs("main: Invalid operation mode, aborting\n", stderr);
		return ECLIARG;
	}

	if (vdisk_open(argv[fargi], &vdin, oflags)) {
		vdisk_perror(__func__);
		return vdisk_errno;
	}

	switch (op_mode) {
	case MODE_INFO:	return vvd_info(&vdin);
	case MODE_MAP:	return vvd_map(&vdin);
	case MODE_NEW:	return vvd_new(&vdin, vsize);
	case MODE_COMPACT:	return vvd_compact(&vdin);
	default: assert(0);
	}

	return EXIT_SUCCESS;
}
