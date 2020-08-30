/**
 * Bochs Virtual HD Image
 * 
 * Header is 512 bytes, including the general header.
 * 
 * Little-endian
 * 
 * http://bochs.sourceforge.net/cgi-bin/topper.pl?name=New+Bochs+Documentation&url=http://bochs.sourceforge.net/doc/docbook/development/index.html
 */

#include <stdint.h>

typedef struct {
	char signacture[32];	// "Bochs Virtual HD Image"
	char type[16];	// "Redolog"
	char subtype[16];	// "Undoable", "Volatile", "Growing"
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
