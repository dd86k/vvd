/**
 * VHDX: (Microsoft) Virtual Hard Disk eXtended
 * 
 * Little Endian
 * 
 * Source: MS-VHDX v20160714
 */

#include <stdint.h>
#include "uid.h"

static const uint64_t VHDX_MAGIC = 0x656C696678646876;	// "vhdxfile"
static const uint32_t VHDX_HDR1_MAGIC = 0x64616568;	// "head"
static const uint32_t VHDX_REGION_MAGIC = 0x69676572;	// "regi"
static const uint32_t VHDX_LOG_HDR_MAGIC = 0x65676F6C;	// "loge"
static const uint32_t VHDX_LOG_ZERO_MAGIC = 0x6F72657A;	// "zero"
static const uint32_t VHDX_LOG_DESC_MAGIC = 0x63736564;	// "desc"
static const uint32_t VHDX_LOG_DATA_MAGIC = 0x61746164;	// "data"
static const uint64_t VHDX_METADATA_MAGIC = 0x617461646174656D;	// "metadata"

enum {
	VHDX_HEADER1_LOC = 64 * 1024,	// 64 KiB
	VHDX_HEADER2_LOC = VHDX_HEADER1_LOC * 2,	// 128 KiB
	VHDX_REGION1_LOC = VHDX_HEADER1_LOC * 3,	// 192 KiB
	VHDX_REGION2_LOC = VHDX_HEADER1_LOC * 4,	// 256 KiB
	VDHX_LOG_ALIGN = 4 * 1024,	// 4 KiB
};

typedef struct {
	uint64_t magic;
	union {
		uint8_t  u8creator[512];
		uint16_t u16creator[256];
	};
} VHDX_HDR;

typedef struct {
	uint32_t magic;
	uint32_t crc32;
	uint32_t seqnumber;
	UID      filewrite;
	UID      datawrite;
	UID      log;
	uint16_t logversion;
	uint16_t version;
	uint32_t logsize;
	uint64_t logoffset;
} VHDX_HEADER1;

typedef struct {
	uint32_t magic;
	uint32_t crc32;
	uint32_t count;
	uint32_t res;
} VHDX_REGION_HDR;

typedef struct {
	// BAT	2DC27766-F623-4200-9D64-115E9BFD4A08	required
	// METADATA	8B7CA206-4790-4B9A-B8FE-575F050F886E	required
	UID      guid;
	uint64_t offset;
	uint32_t length;
	uint32_t required;
} VHDX_REGION_ENTRY;

typedef struct {
	uint32_t magic;
	uint32_t crc32;
	uint32_t count;
	uint32_t tail;
	uint64_t sequence;
	uint32_t desccount;
	uint32_t res;
	UID      guid;
	uint64_t flushedoffset;
	uint64_t lastoffset;
} VHDX_LOG_HDR;

typedef struct {
	uint32_t magic;
	uint32_t trail;	// resolution
	uint64_t leading;	// Multiple of 4 KiB, length
	uint64_t offset;	// Multiple of 4 KiB
	uint64_t sequence;
} VDHX_LOG_DESC;

typedef struct {
	uint32_t magic;
	union {
		uint8_t cdata[4096];	// Cluster
		struct {
			uint32_t sequenceh;
			union {
				uint8_t data[4084]; // as per doc
				struct {
					uint64_t lead;
					uint8_t rdata[4068];	// The real data
					uint64_t trail;
				};
			};
			uint32_t sequencel;
		};
	};
} VHDX_LOG_DATA;

typedef struct {
	uint64_t magic;
	uint16_t res;
	uint16_t count;
	uint8_t  res2[20];
} VHDX_METADATA_HDR;

typedef struct {
	// File Parameters	CAA16737-FA36-4D43-B3B6-33F0AA44E76B
	// Virtual Disk Size	2FA54224-CD1B-4876-B211-5DBED83BF4B8
	// "Page 83 Data"	BECA12AB-B2E6-4523-93EF-C309E000C746
	// Logical Sector Size	8141BF1D-A96F-4709-BA47-F233A8FAAB5F
	// Logical Sector Size	CDA348C7-445D-4471-9CC9-E9885251C556
	// Parent Locator	A8D35F2D-B30B-454D-ABF7-D3D84834AB0C
	UID      type;	// itemID GUID
	uint32_t offset;
	uint32_t length;
	uint32_t flags;	// ...plus 2 bits? what the hell?
} VHDX_METADATA_ENTRY;

typedef struct {
	
} VHDX_INTERNALS;

typedef struct {
	VHDX_HDR hdr;
	VHDX_HEADER1 v1;
	VHDX_HEADER1 v1_2;
	VHDX_REGION_HDR reg;
	VHDX_REGION_HDR reg2;
	VHDX_LOG_HDR log;
	VHDX_METADATA_HDR meta;
	VHDX_INTERNALS in;
} VHDX_META;

static const uint32_t VHDX_META_ALLOC = sizeof(VHDX_META);

struct VDISK;

int vdisk_vhdx_open(struct VDISK *vd, uint32_t flags, uint32_t internal);
