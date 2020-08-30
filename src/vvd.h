#include "vdisk.h"

// Flags for vvd_* functions which the CLI should populate
// The lower 16 bits is for "generic" flags and the higher
// 16 bits is for function-specific flags:
//
// bits 31               16                 0
//      +--------+--------+--------+--------+
//      | Func.  specific |  Generic flags  |
//      +--------+--------+--------+--------+
enum {
	// Show a progress bar
	VVD_PROGRESS	= 0x10,
	// vvd_info: Show raw information
	VVD_INFO_RAW	= 0x10000,
	// vvd_map flags
	//VVD_MAP_	= 0x1000,
	// vvd_compact flags
	//VVD_COMPACT_CLEAN_EMPTY	= 0x10000,
};

/**
 * Print VDISK information to stdout.
 * 
 * This includes information about the VDISK format and type, MBR, GPT, and
 * when available, the operating system filesystem.
 */
int vvd_info(VDISK *vd, uint32_t flags);

/**
 * Print VDISK allocation map to stdout.
 */
int vvd_map(VDISK *vd, uint32_t flags);

/**
 * 
 */
int vvd_new(const oschar *vd, uint32_t format, uint64_t capacity, uint32_t flags);

/**
 * Compact a VDISK.
 * 
 * First, the VDISK is checked if the type is dynamic.
 * If so, it is defragmented (regarding blocks), then proceeds to remove
 * unallocated blocks from the VDISK.
 */
int vvd_compact(VDISK *vd, uint32_t flags);
