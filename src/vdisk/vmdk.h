/**
 * Based on information provided in
 * VMware Virtual Disks Virtual Disk Format 1.1
 * and
 * VMware Virtual Disk Format 5.0
 * documents.
 */
#include <stdint.h>

enum {
	VDMK_COMPRESSED = 0x10000, // Flag
	VMDK_2G_SPLIT_SIZE = 2047 * 1024 * 1024, // 64K Grain size *512 (2G)
	VMDK_TEXT_LENGTH = 10 * 1024	// 10K text overhead buffer
};
enum {
	VMDK_MARKER_EOS	= 0,	// end-of-stream
	VMDK_MARKER_GT	= 1,	// grain table marker
	VMDK_MARKER_GD	= 2,	// grain directory marker
	VMDK_MARKER_FOOTER	= 3,	// footer marker

	VMDK_DISK_DYN	= 1,	// (SELF DEFINED) Sparse
	VMDK_DISK_FIXED	= 2,	// (SELF DEFINED) 
};

// 512 bytes then 10 KiB of text buffer
typedef struct {
	uint32_t magicNumber;
	uint32_t version;
	uint32_t flags;
	uint64_t capacity;	// Disk capacity in sectors
	uint64_t grainSize;	// Block size in sectors
	uint64_t descriptorOffset;
	uint64_t descriptorSize;
	uint32_t numGTEsPerGT;
	uint64_t rgdOffset;
	uint64_t gdOffset;
	uint64_t overHead;
	uint8_t  uncleanShutdown;	// Acts as a boolean value
	uint8_t  singleEndLineChar;	// Typically '\n'
	uint8_t  nonEndLineChar;	// Typically ' '
	uint8_t  doubleEndLineChar1;	// Typically '\r'
	uint8_t  doubleEndLineChar2;	// Typically '\n'
	uint16_t compressAlgorithm;
	uint8_t  pad[433];
} VMDK_HDR;

typedef struct {
	uint64_t uSector;
	uint32_t cbSize;
	uint32_t uType;
	uint8_t  pad[496];
} VMDK_MARKER;

//TODO: grain list

struct VDISK;

int vdisk_vmdk_open(struct VDISK *vd, uint32_t flags, uint32_t internal);
