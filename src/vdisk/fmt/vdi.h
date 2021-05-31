/**
 * VDI: Virtualbox Disk
 * 
 * Little-endian
 * 
 * Source: https://forums.virtualbox.org/viewtopic.php?t=8046
 */

#include <stdint.h>
#include "utils/uid.h"

#define VDI_SIGNATURE	"<<< VirtualBox Disk Image >>>\n"
#define VDI_SIGNATURE_VBOX	"<<< Oracle VM VirtualBox Disk Image >>>\n"
#define VDI_SIGNATURE_OLDER	"<<< InnoTek VirtualBox Disk Image >>>\n"
#define VDI_SIGNATURE_QEMU	"<<< QEMU VM Disk Image >>>\n"

/**
 * Block marked as free is not allocated in image file, read from this
 * block may returns any random data.
 */
static const uint32_t VDI_BLOCK_FREE = 0xffffffff;

/**
 * Block marked as zero is not allocated in image file, read from this
 * block returns zeroes. May be also known as "discarded".
 */
static const uint32_t VDI_BLOCK_ZERO = 0xfffffffe;

#define VDI_IS_ALLOCATED(x)	(x < VDI_BLOCK_FREE)

enum {
	VDI_HEADER_MAGIC	= 0xBEDA107F,
	VDI_SIGNATURE_SIZE	= 64,
	VDI_COMMENT_SIZE	= 256,

	VDI_DISK_DYN	= 1,
	VDI_DISK_FIXED	= 2,
	VDI_DISK_UNDO	= 3,
	VDI_DISK_DIFF	= 4,

	VDI_BLOCKSIZE	= 1048576,	// Default block size, 1 MiB
};

typedef struct {
	uint32_t cCylinders;
	uint32_t cHeads;
	uint32_t cSectors;
	uint32_t cbSector;
} VDI_DISKGEOMETRY;

typedef struct {
	char     signature[64];	// Typically starts with "<<< "
	uint32_t magic;
	uint16_t majorver;
	uint16_t minorver;
} VDI_HDR;

typedef struct { // v0.0
	uint32_t type;
	uint32_t fFlags;
	uint8_t  szComment[VDI_COMMENT_SIZE];
	VDI_DISKGEOMETRY LegacyGeometry;
	uint64_t disksize;
	uint32_t blocksize;
	uint32_t blockstotal;
	uint32_t blocksalloc;
	UID      uuidCreate;
	UID      uuidModify;
	UID      uuidLinkage;
} VDI_HEADERv0;

typedef struct { // v1.1
	uint32_t hdrsize;
	uint32_t type;
	uint32_t fFlags;
	uint8_t  szComment[VDI_COMMENT_SIZE];
	uint32_t offBlocks;	// Byte offset to BAT
	uint32_t offData;	// Byte offset to first block
	VDI_DISKGEOMETRY LegacyGeometry;
	uint32_t u32Dummy;	// Used to be translation value for geometry
	uint64_t capacity;
	uint32_t blk_size;	// Block size in bytes
	uint32_t blk_extra;
	uint32_t blk_total;	// Total amount of blocks
	uint32_t blk_alloc;
	UID      uuidCreate;
	UID      uuidModify;
	UID      uuidLinkage;
	UID      uuidParentModify;
	uint32_t cCylinders;	// v1.1
	uint32_t cHeads;	// v1.1
	uint32_t cSectors;	// v1.1
	uint32_t cbSector;	// v1.1
//	uint8_t  pad[40];
} VDI_HEADERv1;

typedef struct {
	uint32_t *offsets;	// Offset table
	uint32_t mask;	// Block bit mask
	uint32_t shift;	// Block shift positions
	uint16_t majorver;
} VDI_INTERNALS;

typedef struct {
	VDI_HDR hdr;
	union {
		VDI_HEADERv0 v0;
		VDI_HEADERv1 v1;
	};
	VDI_INTERNALS in;
} VDI_META;

static const uint32_t VDI_META_ALLOC = sizeof(VDI_META);

struct VDISK;

int vdisk_vdi_open(struct VDISK *vd, uint32_t flags, uint32_t internal);

int vdisk_vdi_create(struct VDISK *vd, uint64_t capacity, uint32_t flags);

int vdisk_vdi_read_sector(struct VDISK *vd, void *buffer, uint64_t index);

int vdisk_vdi_compact(struct VDISK *vd);
