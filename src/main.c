#ifdef _WIN32 // Windows
#include <Windows.h>
#include <WinBase.h>
#include <tchar.h>
#include <wchar.h>
#else // POSIX

#endif

#define __DATETIME__ __DATE__ " " __TIME__
#define PROJECT_VERSION	"0.0.0"
#define COPYRIGHT "Copyright (c) 2019-2021 dd86k <dd@dax.moe>"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "utils/bin.h"
#include "utils/platform.h"
#include "utils/hash.h"
#include "vvd.h"

//TODO: Move tests into its own compilation unit
//      e.g. tests/structs.c, tests/qed.c, etc.
#ifdef DEBUG
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
	"sizeof	VDI_HDR		%u+%u\n"
	"sizeof	VMDK_HDR	%u\n"
	"sizeof	VHD_HDR		%u\n"
	"sizeof	VHDX_HDR	%u+%u\n"
	"sizeof	QED_HDR		%u\n"
	"sizeof	QCOW_HDR	%u\n"
	"sizeof	PHDD_HDR	%u\n"
	"sizeof	wchar_t		%u\n"
	"Running tests...\n",
	(int)sizeof(VDISK),
	(int)sizeof(VDI_HDR), (int)sizeof(VDI_HEADERv1),
	(int)sizeof(VMDK_HDR),
	(int)sizeof(VHD_HDR),
	(int)sizeof(VHDX_HDR), (int)sizeof(VHDX_HEADER1),
	(int)sizeof(QED_HDR),
	(int)sizeof(QCOW_HDR),
	(int)sizeof(PHDD_HDR),
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
	assert(sizeof(VDI_DISKGEOMETRY) == 16);
	assert(sizeof(VDI_HEADERv0) == 348);
	assert(sizeof(VDI_HEADERv1) == 400);
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

static void help() {
	puts(
	"Manage virtual disks\n"
	"  Usage: vvd OPERATION [FILE] [OPTIONS]\n"
	"         vvd PAGE\n"
	"         vvd {--help|--version|--license}\n"
	"\n"
	"OPERATION\n"
	"  info       Get vdisk image information\n"
	"  new        Create new empty vdisk\n"
	"  map        Show allocation map\n"
	"  compact    Compact vdisk image\n"
	"\n"
	"PAGES\n"
	"  help       Show help page and exit\n"
	"  ver        Only show version and exit\n"
	"  version    Show version page and exit\n"
	"  license    Show license page and exit\n"
	"\n"
	"OPTIONS\n"
	"  --raw           Open as RAW\n"
	"  --create-raw    Create as RAW\n"
	"  --create-dyn    Create vdisk as dynamic\n"
	"  --create-fixed  Create vdisk as fixed\n"
	);
	exit(EXIT_SUCCESS);
}

static void version(void) {
	printf(
	"vvd-" __PLATFORM__ " " PROJECT_VERSION " (compiled " __DATETIME__ ")\n"
#ifdef __VERSION__
	"Compiler: " __VERSION__ "\n"
#endif
	"MIT License: " COPYRIGHT "\n"
	"Project page: <https://github.com/dd86k/vvd>\n"
	"Defines: "
#ifdef DEBUG
	"DEBUG "
#endif
#ifdef TRACE
	"TRACE "
#endif
	"\n\n"
	"FORMAT	OPERATIONS\n"
	"VDI	info, map, new, compact\n"
	"VMDK	info\n"
	"VHD	info, map\n"
	"VHDX	\n"
	"QED	info, map\n"
	"QCOW	\n"
	"PHDD	\n"
	"RAW	info\n"
	);
	exit(EXIT_SUCCESS);
}

static void license() {
	puts(
	COPYRIGHT "\n"
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
//TODO: Check if UNICODE is defined
#define MAIN wmain(int argc, wchar_t **argv)
#else
#define MAIN main(int argc, char **argv)
#endif

// NOTICE: Hashes may be cute, but for the command-line interface (CLI), that
//         would require both UTF-8 and UTF-16LE versions for all hashes (and
//         possibly more encodings), so no thanks. Besides, I'm lazy, and this
//         is obviously not critical code.

/**
 * Match a patch to an exception with the VDISK_FORMAT_* enum.
 * 
 * \returns VDISK_FORMAT_* enumeration value
 */
static int vdextauto(const oschar *path) {
	if (extcmp(path, osstr("vdi")))  return VDISK_FORMAT_VDI;
	if (extcmp(path, osstr("vmdk"))) return VDISK_FORMAT_VMDK;
	if (extcmp(path, osstr("vhd")))  return VDISK_FORMAT_VHD;
	if (extcmp(path, osstr("vhdx"))) return VDISK_FORMAT_VHDX;
	if (extcmp(path, osstr("qed")))  return VDISK_FORMAT_QED;
	if (extcmp(path, osstr("qcow")) || extcmp(path, osstr("qcow2")))
		return VDISK_FORMAT_QCOW;
	if (extcmp(path, osstr("hdd")))  return VDISK_FORMAT_PHDD;
	return VDISK_FORMAT_NONE;
}

// Main entry point. This only performs intepreting the command-line options
// for the core functions in vvd.c.
int MAIN {
	if (argc <= 1)
		help();

	struct settings_t settings = {};
	VDISK vdin;	// vdisk IN
	VDISK vdout;	// vdisk OUT
	const oschar *defopt = NULL;	// Default option for input file

	// Additional arguments are processed first, since they're simpler
	//TODO: --verbose (>v0.10.0)
	//TODO: --verify-repair (or verify operation?)
	for (size_t argi = 2; argi < argc; ++argi) {
		const oschar *arg = argv[argi];
		
		// Generic
		if (osstrcmp(arg, osstr("--size")) == 0) {
			if (argi + 1 >= argc) {
				fputs("main: missing argument for --size\n", stderr);
				return EXIT_FAILURE;
			}
			if (strtobin(&settings.vsize, argv[++argi])) {
				fputs("main: failed to convert binary number\n", stderr);
				return EXIT_FAILURE;
			}
			continue;
		}
		if (osstrcmp(arg, osstr("--progress")) == 0) {
			settings.progressbar = 1;
			continue;
		}
		
		// vdisk_open flags
		if (osstrcmp(arg, osstr("--raw")) == 0) {
			#if TRACE
			VVDTRACE("--raw toggled");
			#endif	// TRACE
			settings.vdisk.flags |= VDISK_RAW;
			continue;
		}
		
		// vdisk_create flags
		if (osstrcmp(arg, osstr("--create-raw")) == 0) {
			settings.vdisk.flags |= VDISK_RAW;
			continue;
		}
		if (osstrcmp(arg, osstr("--create-dynamic")) == 0) {
			settings.vdisk.flags |= VDISK_CREATE_TYPE_DYNAMIC;
			continue;
		}
		if (osstrcmp(arg, osstr("--create-fixed")) == 0) {
			settings.vdisk.flags |= VDISK_CREATE_TYPE_FIXED;
			continue;
		}
		
		// vvd_info flags
		if (osstrcmp(arg, osstr("--info-full")) == 0) {
			settings.info.full = 1;
			continue;
		}
		
		// Default argument
		if (defopt == NULL) {
			defopt = arg;
			continue;
		}

		fprintf(stderr, "main: '" OSCHARFMT "' unknown option\n", arg);
		return EXIT_FAILURE;
	}

	const oschar *action = argv[1];

	//
	// Main operation actions
	//

	if (osstrcmp(action, osstr("info")) == 0) {
		if (defopt == NULL) {
			fputs("main: missing vdisk\n", stderr);
			return EXIT_FAILURE;
		}
		if (vdisk_open(&vdin, defopt, settings.vdisk.flags)) {
			vvd_perror(&vdin);
			return vdin.err.num;
		}
		return vvd_info(&vdin, &settings);
	}

	if (osstrcmp(action, osstr("map")) == 0) {
		if (defopt == NULL) {
			fputs("main: missing vdisk\n", stderr);
			return EXIT_FAILURE;
		}
		if (vdisk_open(&vdin, defopt, settings.vdisk.flags)) {
			vvd_perror(&vdin);
			return vdin.err.num;
		}
		return vvd_map(&vdin, 0);
	}

	if (osstrcmp(action, osstr("compact")) == 0) {
		if (defopt == NULL) {
			fputs("main: missing vdisk\n", stderr);
			return EXIT_FAILURE;
		}
		if (vdisk_open(&vdin, defopt, 0)) {
			vvd_perror(&vdin);
			return vdin.err.num;
		}
		if (vvd_compact(&vdin, 0)) {
			vvd_perror(&vdin);
			return vdin.err.num;
		}
		return EXIT_FAILURE;
	}

	if (osstrcmp(action, osstr("defrag")) == 0) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	if (osstrcmp(action, osstr("new")) == 0) {
		if (defopt == NULL) {
			fputs("main: missing path specifier\n", stderr);
			return EXIT_FAILURE;
		}
		if (settings.vsize == 0) {
			fputs("main: capacity cannot be zero\n", stderr);
			return EXIT_FAILURE;
		}

		// Get vdisk type out of extension name
		int format = vdextauto(defopt);
		if (format == VDISK_FORMAT_NONE) {
			fputs("main: unknown extension\n", stderr);
			return EXIT_FAILURE;
		}

		return vvd_new(defopt, format, settings.vsize, &settings);
	}

	if (osstrcmp(action, osstr("resize")) == 0) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	if (osstrcmp(action, osstr("verify")) == 0) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	if (osstrcmp(action, osstr("convert")) == 0) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	if (osstrcmp(action, osstr("upgrade")) == 0) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	if (osstrcmp(action, osstr("cleanfs")) == 0) {
		fputs("main: not implemented\n", stderr);
		return EXIT_FAILURE;
	}

	//
	// Internal things
	//

	if (osstrcmp(action, osstr("internalhash")) == 0) {
		if (defopt == NULL) {
			fputs("main: missing argument\n", stderr);
			return EXIT_FAILURE;
		}
#ifdef _WIN32
		char buffer[200];
		wcstombs(buffer, defopt, 200);
		printf("%08X\n", hash_string((char*)buffer));
#else
		printf("%08X\n", hash_string((char*)defopt));
#endif
		return EXIT_SUCCESS;
	}

	if (osstrcmp(action, osstr("internalguidhash")) == 0) {
		if (defopt == NULL) {
			fputs("main: missing argument\n", stderr);
			return EXIT_FAILURE;
		}
		UID uid;
#ifdef _WIN32
		char buffer[200];
		wcstombs(buffer, defopt, 200);
		int r = uid_parse(&uid, buffer, UID_GUID);
#else
		int r = uid_parse(&uid, defopt, UID_GUID);
#endif
		if (r) {
			if (r < 0)
				perror("main");
			else
				printf("main: failed to parse UID, got %d items\n", r);
			return EXIT_FAILURE;
		}
		printf("%08X\n", hash_compute((const char*)&uid, 16));
		return EXIT_SUCCESS;
	}

	//TODO: internalmbrtype <hexcode>
	//TODO: internalgpttype <GUID>

	//
	// Pages
	//

	if (osstrcmp(action, osstr("ver")) == 0) {
		puts(PROJECT_VERSION);
		return EXIT_SUCCESS;
	}
	if (osstrcmp(action, osstr("version")) == 0 || osstrcmp(action, osstr("--version")) == 0)
		version();
	//TODO: Help system in its own module
	if (osstrcmp(action, osstr("help")) == 0 || osstrcmp(action, osstr("--help")) == 0)
		help();
	if (osstrcmp(action, osstr("license")) == 0 || osstrcmp(action, osstr("--license")) == 0)
		license();
#ifdef DEBUG
	if (osstrcmp(action, osstr("--test")) == 0)
		test();
#endif

	fprintf(stderr, "main: '" OSCHARFMT "' unknown operation, see 'vvd help'\n", action);
	return EXIT_FAILURE;
}
