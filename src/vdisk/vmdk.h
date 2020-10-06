/**
 * VMware Disk image
 * 
 * Little-endian
 * 
 * +-------------+
 * | 0 | 1 | ... | Grain Directory Entries
 * +-------------+
 *   |
 * +---
 * | 1 Grain Table Entries
 * 
 * Sources:
 * - VMware Virtual Disks Virtual Disk Format 1.1
 * - VMware Virtual Disk Format 5.0
 */

#include <stdint.h>

enum {
	VMDK_F_VALID_NL	= 0x1,	// Valid newline detection
	VMDK_F_REDUNDANT_TABLE	= 0x2,	// Redundant grain table will be used
	VMDK_F_ZEROED_GTE	= 0x4,	// Zeroed-grain GTE will be used
	VDMK_F_COMPRESSED	= 0x10000,	// Grains are compressed
	VMDK_F_MARKERS	= 0x20000,	// Markers used

	VMDK_C_NONE	= 0,	// No compression is used
	VMDK_C_DEFLATE	= 1,	// DEFLATE (RFC 1951) is used

	VMDK_2G_SPLIT_SIZE	= 2047 * 1024 * 1024, // grainSize*sectorSize = 2 GiB
	VMDK_TEXT_LENGTH	= 10 * 1024,	// 10K text overhead buffer
	VMDK_GRAINSIZE_DEFAULT	= 64 * 1024	// Default being 64K
};
enum {
	VMDK_MARKER_EOS	= 0,	// end-of-stream
	VMDK_MARKER_GT	= 1,	// grain table marker
	VMDK_MARKER_GD	= 2,	// grain directory marker
	VMDK_MARKER_FOOTER	= 3,	// footer marker

	VMDK_DISK_DYN	= 1,	// (Internal) Sparse
	VMDK_DISK_FIXED	= 2,	// (Internal) Monolithic
};

typedef struct {
	uint32_t magicNumber;
	uint32_t version;	// v1 or v2
	uint32_t flags;	// See VMDK_F_* values
	uint64_t capacity;	// Disk capacity in sectors
	uint64_t grainSize;	// Block size in sectors
	uint64_t descriptorOffset;	// If set, embedded descriptor offset in sectors
	uint64_t descriptorSize;	// If set, embedded descriptor size in sectors
	uint32_t numGTEsPerGT;	// Number of entries in a grain table, typically 512
	uint64_t rgdOffset;	// Offset to level 0 redundant metadata in sectors
	uint64_t gdOffset;	// Offset to level 0 metadata (grain directory) in sectors
	uint64_t overHead;	// Offset to data in sectors
	uint8_t  uncleanShutdown;	// Acts as a boolean value
	uint8_t  singleEndLineChar;	// Typically '\n'
	uint8_t  nonEndLineChar;	// Typically ' '
	uint8_t  doubleEndLineChar1;	// Typically '\r'
	uint8_t  doubleEndLineChar2;	// Typically '\n'
	uint16_t compressAlgorithm;	// See VMDK_C_* values
	uint8_t  pad[433];
} VMDK_HDR;

typedef struct {
	uint64_t uSector;
	uint32_t cbSize;
	uint32_t uType;
	uint8_t  pad[496];
} VMDK_MARKER;

typedef struct {
	uint32_t *l0_offsets;	// Grain Directory offsets
	uint32_t *l1_offsets;	// Grain Table offsets
	uint32_t mask;	// Bit offset mask
	uint32_t shift;	// Bit offset shift
	uint64_t overhead;	// data overhead in bytes
} VMDK_INTERNALS;

typedef struct {
	VMDK_HDR hdr;
	VMDK_INTERNALS in;
} VMDK_META;

static const uint32_t VMDK_META_ALLOC = sizeof(VMDK_META);

struct VDISK;

int vdisk_vmdk_open(struct VDISK *vd, uint32_t flags, uint32_t internal);

int vdisk_vmdk_sparse_read_lba(struct VDISK *vd, void *buffer, uint64_t index);
