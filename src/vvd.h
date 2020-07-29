#include "vdisk.h"

/**
 * Print VDISK information to stdout.
 * 
 * This includes information about the VDISK format and type, MBR, GPT, and
 * when available, the operating system filesystem.
 */
int vvd_info(VDISK *vd);

/**
 * Print VDISK allocation map to stdout.
 */
int vvd_map(VDISK *vd, uint32_t flags);

/**
 * 
 */
int vvd_new(const _vchar *vd, int format, uint64_t vsize, int flags);

/**
 * Compact a VDISK.
 * 
 * First, the VDISK is checked if the type is dynamic.
 * If so, it is defragmented (regarding blocks), then proceeds to remove
 * unallocated blocks from the VDISK.
 */
int vvd_compact(VDISK *vd);
