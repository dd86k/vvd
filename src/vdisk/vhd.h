/**
 * VHD: (Connectix) Virtual Hard Disk
 */

#include <stdint.h>

#define VHDMAGIC "conectix"
#define VHD_MAGIC	0x78697463656E6F63	// "conectix"
#define VHD_DYN_MAGIC	0x6573726170737863	// "cxsparse"
#define VHD_OS_WIN	0x6B326957	// "Wi2k"
#define VHD_OS_MAC	0x2063614D	// "Mac "
enum {
	VHD_DISK_NONE	= 0,
	VHD_DISK_RES1	= 1,
	VHD_DISK_FIXED	= 2,
	VHD_DISK_DYN	= 3,
	VHD_DISK_DIFF	= 4,
	VHD_DISK_RES2	= 5,
	VHD_DISK_RES3	= 6
};
enum {
	VHD_BLOCK_UNALLOC	= -1,	// Block not allocated on disk
	VHD_FEAT_TEMP	= 1,
	VHD_FEAT_RES	= 2	// reserved, but always set
};

typedef struct { // v1
	uint64_t magic;	// "conectix"
	uint32_t features;
	uint16_t major;
	uint16_t minor;
	uint64_t offset;
	uint32_t timestamp;
	char     creator_app[4];
	uint16_t creator_major;
	uint16_t creator_minor;
	uint32_t creator_os;
	uint64_t size_original;	// Capacity in bytes
	uint64_t size_current;
	uint16_t cylinders;
	uint8_t  heads;
	uint8_t  sectors;
	uint32_t type;
	uint32_t checksum;
	UID      uuid;
	uint8_t  savedState;
	uint8_t  reserved[427];
} VHD_HDR;

typedef struct {
	uint32_t code;
	uint32_t dataspace;
	uint32_t datasize;
	uint32_t res;
	uint64_t offset;
} VHD_PARENT_LOCATOR;

typedef struct { // v1
	uint64_t magic;
	uint64_t data_offset;
	uint64_t table_offset;
	uint16_t minor;
	uint16_t major;
	uint32_t max_entries;	// For table
	uint32_t blocksize;	// In sectors
	uint32_t checksum;
	UID      parent_uuid;	// UUID
	uint32_t parent_timestamp;
	uint32_t res;
	uint16_t parent_name[256];	// UTF-16
	VHD_PARENT_LOCATOR parent_locator[8];
	uint8_t  res1[256];
} VHD_DYN_HDR;

struct VDISK;

int vdisk_vhd_open(struct VDISK *vd, uint32_t flags, uint32_t internal);

int vdisk_vhd_dyn_read_lba(struct VDISK *vd, void *buffer, uint64_t index);

int vdisk_vhd_fixed_read_lba(struct VDISK *vd, void *buffer, uint64_t index);
