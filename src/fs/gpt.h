#pragma once

#include <stdint.h>
#include "../uid.h"
#include "../vdisk.h"

// GNU GRUB "Hah!IdontNeedEFI"

#define EFI_SIG	0x5452415020494645	// "EFI PART"
#define EFI_SIG_LOW	0x20494645
#define EFI_SIG_HIGH	0x54524150
#define EFI_PART_NAME_LENGTH	36

//
// EFI Parition Entry flags (GPT_ENTRY::flags)
//

#define EFI_PE_PLATFORM_REQUIRED	1	// bit 0, preserve as-is
#define EFI_PE_EFI_FIRMWARE_IGNORE	2	// bit 1, ignore content of partition
#define EFI_PE_LEGACY_BIOS_BOOTABLE	4	// bit 2

//
// EFI Parition Entry flags (GPT_ENTRY::resflags)
//

//
// EFI Parition Entry flags (GPT_ENTRY::partflags)
//

// Google Chrome OS
// priority[3:0]
// tries remiaining[7:4]
#define EFI_PE_SUCCESSFUL_BOOT	0x10	// bit 8

// Microsoft
#define EFI_PE_READ_ONLY	0x1000	// bit 12
#define EFI_PE_SHADOW_COPY	0x2000	// bit 13
#define EFI_PE_HIDDEN	0x4000	// bit 14
#define EFI_PE_NO_DRIVE_LETTER	0x8000	// bit 15

typedef struct LBA64 {
	union {
		uint64_t lba;
		struct {
			uint32_t low, high;
		};
	};
} LBA64;

// Protective MBRs have type of EEH, and the header is usually found at LBA 1
typedef struct GPT { // v1.0
	union {
		uint64_t sig; // "EFI PART"
		struct {
			uint32_t siglow, sighigh;
		};
	};
	uint16_t minorver;
	uint16_t majorver;
	uint32_t headersize;	// usually 92 bytes (v1.0)
	uint32_t crc32;
	uint32_t reserved;	// reserved
	LBA64    current;
	LBA64    backup;
	LBA64    firstlba;
	LBA64    lastlba;
	UID      guid;	// Disk GUID
	LBA64    pt_location;	// (partition table) location
	uint32_t pt_entries;	// (partition table) number of entries
	uint32_t pt_esize;	// (parition entry) structure size
	uint32_t pt_crc32;	// (partition table) CRC32
	uint8_t  pad[420];
} GPT;

// GPT entry structure
typedef struct GPT_ENTRY {
	// Unused entry        : 00000000-0000-0000-0000-000000000000
	// EFI System Partition: C12A7328-F81F-11D2-BA4B-00A0C93EC93B
	// Contains legacy MBR : 024DEE41-33E7-11D3-9D69-0008C781F39F
	UID      type;	// Parition type GUID
	UID      part;	// Unique partition GUID
	LBA64    firstlba;
	LBA64    lastlba;
	union {
		uint64_t flagsraw;
		struct {
			// Bit 0 - Required for platform
			// Bit 1 - If on, do not produce EFI_BLOCK_IO_PROTOCOL
			// Bit 2 - Legacy PC-AT BIOS bootable 
			uint32_t flags;	// GPT entry flags
			uint16_t resflags;	// Reserved
			// (Chrome OS) Bit 0:3 - Priority (0: non-boot, 1:lowest, 15:highest)
			// (Chrome OS) Bit 4:7 - Tries remaining
			// (Chrome OS) Bit 8 - Successful boot
			// (Windows) Bit 12 - Read-only
			// (Windows) Bit 13 - Shadow partition copy
			// (Windows) Bit 14 - Hidden
			// (Windows) Bit 15 - No drive letter (no automount)
			uint16_t partflags;	// Partition-defined flags
		};
	};
	uint16_t partname[36];	// 72 bytes, 32 UTF-16LE characters
	uint8_t  pad[384];
} GPT_ENTRY;

struct VDISK;

/**
 * Prints GPT information to stdout.
 */
void gpt_info_stdout(GPT *);
/**
 * Run through the list of GPT_ENTRY from a VDISK.
 */
void gpt_info_entries_stdout(VDISK *vd, GPT *gpt, uint32_t lba);
