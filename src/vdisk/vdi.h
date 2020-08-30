/**
 * VDI: Virtualbox DIsk
 * 
 * https://forums.virtualbox.org/viewtopic.php?t=8046
 */

#include <stdint.h>
#include "../uid.h"

#define VDI_SIGNATURE	"<<< Oracle VM VirtualBox Disk Image >>>\n"
#define VDI_SIGNATURE_OLDER	"<<< InnoTek VirtualBox Disk Image >>>\n"

/**
 * Block marked as free is not allocated in image file, read from this
 * block may returns any random data.
 */
static const uint32_t VDI_BLOCK_FREE = ~0;

/**
 * Block marked as zero is not allocated in image file, read from this
 * block returns zeroes.
 */
static const uint32_t VDI_BLOCK_ZERO = ~1;

enum {
	VDI_HEADER_MAGIC	= 0xBEDA107F,
	VDI_SIGNATURE_SIZE	= 64,
	VDI_COMMENT_SIZE	= 256,

	VDI_DISK_DYN	= 1,
	VDI_DISK_FIXED	= 2,
	VDI_DISK_UNDO	= 3,
	VDI_DISK_DIFF	= 4,

	VDI_BLOCKSIZE	= 1024 * 1024,	// Typical block size, 1 MiB
};

typedef struct {
	uint32_t cCylinders;
	uint32_t cHeads;
	uint32_t cSectors;
	uint32_t cbSector;
} VDIDISKGEOMETRY;

typedef struct { // Excludes char[64] at start
	uint32_t magic;
	uint16_t majorv;
	uint16_t minorv;
} VDI_HDR;

typedef struct { // v0.0
	uint32_t type;
	uint32_t fFlags;
	uint8_t  szComment[VDI_COMMENT_SIZE];
	VDIDISKGEOMETRY LegacyGeometry;
	uint64_t disksize;
	uint32_t blocksize;
	uint32_t totalblocks;
	uint32_t blocksalloc;
	UID      uuidCreate;
	UID      uuidModify;
	UID      uuidLinkage;
} VDIHEADER0;

typedef struct { // v1.1
	uint32_t hdrsize;
	uint32_t type;
	uint32_t fFlags;
	uint8_t  szComment[VDI_COMMENT_SIZE];
	uint32_t offBlocks;	// Byte offset to BAT
	uint32_t offData;	// Byte offset to first block
	VDIDISKGEOMETRY LegacyGeometry;
	uint32_t u32Dummy;	// Used to be translation value for geometry
	uint64_t disksize;
	uint32_t blocksize;	// Block size in bytes
	uint32_t blocksextra;
	uint32_t totalblocks;
	uint32_t blocksalloc;
	UID      uuidCreate;
	UID      uuidModify;
	UID      uuidLinkage;
	UID      uuidParentModify;
	uint32_t cCylinders;	// v1.1
	uint32_t cHeads;	// v1.1
	uint32_t cSectors;	// v1.1
	uint32_t cbSector;	// v1.1
//	uint8_t  pad[40];
} VDIHEADER1;

struct VDISK;

int vdisk_vdi_open(struct VDISK *vd, uint32_t flags, uint32_t internal);

int vdisk_vdi_read_sector(struct VDISK *vd, void *buffer, uint64_t index);

int vdisk_vdi_compact(struct VDISK *vd, void(*cb_progress)(uint32_t type, void *data));
