/**
 * PHDD: Parallels Hard Disk Drive
 * 
 * https://github.com/qemu/qemu/blob/master/docs/interop/parallels.txt
 * VBox/Storage/Parallels.cpp
 */

#include <stdint.h>

// Parallels header structure
typedef struct {
	// Magic header
	char     magic[16];
	// Virtual disk version, currently 2
	uint32_t version;
	// (CHS) Heads
	uint32_t heads;
	// (CHS) Cylinders
	uint32_t cylinders;
	// (CHS) Number of sectors per track
	uint32_t sectorsPerTrack;
	// Number of entries in the allocation bitmap.
	uint32_t number_entries;
	// Total number of sectors
	uint32_t sectors;
	// Padding.
//	char Padding[24];
} PHDD_HDR;

struct VDISK;

int vdisk_phdd_open(struct VDISK *vd, uint32_t flags, uint32_t internal);
