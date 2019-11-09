#include <stdint.h>

enum {
	VDMK_COMPRESSED = 0x10000, // Flag[BIN_FLENGTH]
	VMDK_2G_SPLIT_SIZE = 2047 * 1024 * 1024, // 64K Grain size *512 (2G)
};
enum {
	VMDK_MARKER_EOS	= 0,	// end-of-stream
	VMDK_MARKER_GT	= 1,	// grain table marker
	VMDK_MARKER_GD	= 2,	// grain directory marker
	VMDK_MARKER_FOOTER	= 3,	// footer marker

	VMDK_DISK_DYN	= 1,	// (SELF DEFINED) A.K.A. Sparse
	VMDK_DISK_FIXED	= 2,	// (SELF DEFINED) 
};

typedef struct VMDK_HDR {
	uint32_t magicNumber;
	uint32_t version;
	uint32_t flags;
	uint64_t capacity; // in sectors
	uint64_t grainSize;
	uint64_t descriptorOffset;
	uint64_t descriptorSize;
	uint32_t numGTEsPerGT;
	uint64_t rgdOffset;
	uint64_t gdOffset;
	uint64_t overHead;
	uint8_t  uncleanShutdown;	// "Bool"
	uint8_t  singleEndLineChar;	// usually '\n'
	uint8_t  nonEndLineChar;	// usually ' '
	uint8_t  doubleEndLineChar1;	// usually '\r'
	uint8_t  doubleEndLineChar2;	// usually '\n'
	uint16_t compressAlgorithm;
	uint8_t  pad[433];
} VMDK_HDR; // 512 bytes then 10 KiB of text buffer

typedef struct VMDK_MARKER {
	uint64_t uSector;
	uint32_t cbSize;
	uint32_t uType;
	uint8_t  pad[496];
} VMDK_MARKER;

struct VDISK;

/**
 * Print VMDK information to stdout.
 */
void vmdk_info(struct VDISK *vd);
