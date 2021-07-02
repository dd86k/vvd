#include "vdisk/vdisk.h"

// Command-line options
struct settings_t {
	uint64_t vsize;	// Virtual disk size, used in 'new' and 'resize'
	char progressbar;	// Show progress bar
	char verbose;	// Verbose level
	struct settings_info_t {
		char raw;	// Show raw, unformatted data values
		char full;	// Show all fields instead of a summary
	} info;
	struct settings_vdisk_t {
		uint32_t flags;
	} vdisk;
	struct settings_internals_t {
		char gpt_bkp;	// Uh uh, go fetch the backup GPT header!
	} internal;
};

/**
 * Print VDISK error.
 */
void vvd_perror(VDISK *vd);

/**
 * Print VDISK information to stdout.
 * 
 * This includes information about the VDISK format and type, MBR, GPT, and
 * when available, the operating system filesystem.
 */
int vvd_info(VDISK *vd, struct settings_t *);

/**
 * Print VDISK allocation map to stdout.
 */
int vvd_map(VDISK *vd, struct settings_t *);

/**
 * 
 */
int vvd_new(const oschar *vd, uint32_t format, uint64_t capacity, struct settings_t *);

/**
 * Compact a VDISK.
 * 
 * First, the VDISK is checked if the type is dynamic.
 * If so, it is defragmented (regarding blocks), then proceeds to remove
 * unallocated blocks from the VDISK.
 */
int vvd_compact(VDISK *vd, uint32_t flags);
