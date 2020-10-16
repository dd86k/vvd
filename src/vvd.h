#include "vdisk/vdisk.h"

/*
	Bits	Description
	0:15	Global flags
	16:23	Function-specific flags
	24:31	Function internal specific flags
*/
enum {
	// Show a progress bar
	VVD_PROGRESS	= 0x10,
	// vvd_info: Show raw information
	VVD_INFO_RAW	= 0x100,
	// vvd_map flags
	//VVD_MAP_	= 0x1000,
	// vvd_compact flags
	//VVD_COMPACT_CLEAN_EMPTY	= 0x10000,
	// Internal: The backup GPT entries were selected
	VVD_INTERNAL_GPT_BKP	= 0x01000000,
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
