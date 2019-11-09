#include "vdisk.h"

enum {	// OPERATIONS, main operations from CLI
	MODE_INFO	= 'I',	// Disk information
	MODE_STATS	= 'S',	// Statistics
	MODE_MAP	= 'M',	// Map (print allocation map)
	MODE_NEW	= 'N',	// New empty disk
	MODE_COMPACT	= 'C',	// Compress/compact/trim
	MODE_DEFRAG	= 'D',	// Defragmentation tool
	MODE_CONVERT	= 'T',	// Translate format
	MODE_RESIZE	= 'R',	// Shrink/expand
	MODE_PARTITION	= 'P',	// TUI Partition tool
};

enum {	// CLI/VVD error codes
	ECLIOK	= 0,
	ECLIARG	= 1,	// Invalid/missing CLI option
	ECLIOPEN	= 2,	// Could not open or create VDISK
	ECLIFORMAT	= 3,	// Format not supported for OPERATION
	ECLIALLOC	= 4,	// Memory allocation failed
};

/**
 * MODE_INFO: Print VDISK information to stdout.
 * 
 * This includes information about the VDISK format and type, MBR, GPT, and
 * when available, the operating system filesystem.
 */
int vvd_info(VDISK *vd);

/**
 * MODE_MAP: Print VDISK allocation map to stdout.
 */
int vvd_map(VDISK *vd);

/**
 * MODE_NEW: 
 */
int vvd_new(VDISK *vd, uint64_t vsize);

/**
 * MODE_COMPACT: Compact a VDISK.
 * 
 * First, the VDISK is checked if the type is dynamic.
 * If so, it is defragmented (regarding blocks), then proceeds to remove
 * unallocated blocks from the VDISK.
 */
int vvd_compact(VDISK *vd);
