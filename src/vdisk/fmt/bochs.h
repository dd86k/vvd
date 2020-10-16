/**
 * Bochs Virtual HD Image
 * 
 * Header is 512 bytes, including the general header.
 * 
 * Little-endian
 * 
 * http://bochs.sourceforge.net/doc/docbook/development/harddisk-redologs.html
 * §2.10
 */

#include <stdint.h>

static const uint32_t BOCHS_VERSION = 0x00020000;
static const uint32_t BOCHS_V1 = 0x00010000;
static const uint32_t BOCHS_UNALLOC = 0xffffffff;

typedef struct {
	char signacture[32];	// "Bochs Virtual HD Image"
	char type[16];	// "Redolog"
	char subtype[16];	// "Undoable", "Volatile", "Growing", "vvfat"
	uint32_t version;	// 0x00010000, 0x00020000
	uint32_t hdrsize;	// 512
} BOCH_HDR;

typedef struct {
	uint32_t nentries;	// Number of entries in catalog
	uint32_t bitmapsize;	// Cluster size in bytes
	uint32_t extentsize;	// Extent cluster size in bytes
	uint32_t timestamp;	// ("Undoable" only) timestamp, FAT format
	uint64_t disksize;	// Disk capacity in bytes
} BOCH_REDOLOG_HDR;
