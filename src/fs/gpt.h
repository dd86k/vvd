#pragma once

#include <stdint.h>
#include "uid.h"

// GNU GRUB "Hah!IdontNeedEFI"

#define EFI_SIG	0x5452415020494645	// "EFI PART"
#define EFI_SIG_LOW	0x20494645
#define EFI_SIG_HIGH	0x54524150
#define EFI_PART_NAME_LENGTH	36

//
// GPT flags
//
// Sections
// * EFI flags
// * Google flags
// * Microsoft flags
//
// Bits   Description
// 2:0    EFI/General flags
// 47:3   Reserved
// 63:48  OS specifics (Google, Microsoft)
//

/**
 * The computing platform requires this partition to function properly.
 */
static const uint64_t GPT_FLAG_PLATFORM_REQUIRED	= 1;
/**
 * The EFI firmware should ignore this partition and its data, and avoid
 * reading from it.
 */
static const uint64_t GPT_FLAG_EFI_FIRMWARE_IGNORE	= 2;
/**
 * Indicates that a legacy BIOS may boot from this partition.
 */
static const uint64_t GPT_FLAG_LEGACY_BIOS_BOOTABLE	= 4;

//
// Google flags
//

/**
 * Google flag for ChromeOS
 * 
 * Likely set when the OS booted successfully from the partition.
 */
static const uint64_t GPT_FLAG_SUCCESSFUL_BOOT = 0x100000000000000;
/**
 * Google flag mask for ChromeOS
 * 
 * Likely the number of tries remaining to boot from the partition.
 */
static const uint64_t GPT_FLAG_TRIES_REMAINING_MASK  = 0x100000000000000;
static const uint64_t GPT_FLAG_TRIES_REMAINING_SHIFT = 52;
/**
 * Google flag mask for ChromeOS
 * 
 * Likely denotes a priority, 15 being the high, 1 lowest, and 0 not bootable.
 */
static const uint64_t GPT_FLAG_PRIORITY_MASK  = 0x100000000000000;
static const uint64_t GPT_FLAG_PRIORITY_SHIFT = 0x100000000000000;

//
// Microsoft flags
//
// Source: https://docs.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-partition_information_gpt
//

/**
 * Microsoft flag: GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY
 * 
 * If this attribute is set, the partition is read-only.
 * 
 * Writes to the partition will fail. IOCTL_DISK_IS_WRITABLE will fail with the
 * ERROR_WRITE_PROTECT Win32 error code, which causes the file system to mount
 * as read only, if a file system is present.
 * 
 * VSS uses this attribute.
 * 
 * Do not set this attribute for dynamic disks. Setting it can cause I/O errors
 * and prevent the file system from mounting properly.
 */
static const uint64_t GPT_FLAG_READ_ONLY = 0x1000000000000000;
/**
 * Microsoft flag: GPT_BASIC_DATA_ATTRIBUTE_SHADOW_COPY
 * 
 * If this attribute is set, the partition is a shadow copy of another partition.
 * 
 * VSS uses this attribute. This attribute is an indication for file system
 * filter driver-based software (such as antivirus programs) to avoid
 * attaching to the volume.
 * 
 * An application can use the attribute to differentiate a shadow copy volume
 * from a production volume. An application that does a fast recovery, for
 * example, will break a shadow copy LUN and clear the read-only and hidden
 * attributes and this attribute. This attribute is set when the shadow copy is
 * created and cleared when the shadow copy is broken.
 * 
 * Despite its name, this attribute can be set for basic and dynamic disks.
 * 
 * Windows Server 2003: This attribute is not supported before Windows Server
 * 2003 with SP1.
 */
static const uint64_t GPT_FLAG_SHADOW_COPY = 0x2000000000000000;
/**
 * Microsoft flag: GPT_BASIC_DATA_ATTRIBUTE_HIDDEN
 * 
 * If this attribute is set, the partition is not detected by the Mount Manager.
 * 
 * As a result, the partition does not receive a drive letter, does not receive
 * a volume GUID path, does not host mounted folders (also called volume mount
 * points), and is not enumerated by calls to FindFirstVolume and
 * FindNextVolume. This ensures that applications such as Disk Defragmenter do
 * not access the partition. The Volume Shadow Copy Service (VSS) uses this
 * attribute.
 * 
 * Despite its name, this attribute can be set for basic and dynamic disks.
 */
static const uint64_t GPT_FLAG_HIDDEN = 0x4000000000000000;
/**
 * Microsoft flag: GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER
 * 
 * If this attribute is set, the partition does not receive a drive letter by
 * default when the disk is moved to another computer or when the disk is seen
 * for the first time by a computer. 
 */
static const uint64_t GPT_FLAG_NO_DRIVE_LETTER = 0x8000000000000000;

//
// Structures
//

typedef struct LBA64 {
	union {
		uint64_t lba;
		struct {
			uint32_t lowlba, highlba;
		};
	};
} LBA64;

// Protective MBRs have type of EEH, and the header is usually found at LBA 1
typedef struct { // v1.0
	union {
		uint64_t sig; // "EFI PART"
		struct {
			uint32_t siglow, sighigh;
		};
	};
	uint16_t minorver;
	uint16_t majorver;
	uint32_t headersize;	// usually 92 bytes (v1.0)
	uint32_t headercrc32;
	uint32_t reserved;	// reserved
	LBA64    current;
	LBA64    backup;	// Backup LBA index
	LBA64    first;	// First LBA index
	LBA64    last;	// Last LBA index
	UID      guid;	// Unique disk GUID
	LBA64    pt_location;	// (partition table) location
	uint32_t pt_entries;	// (partition table) maximum number of entries
	uint32_t pt_esize;	// (partition entry) structure size
	uint32_t pt_crc32;	// (partition table) CRC32
	uint8_t  pad[420];
} GPT;

// GPT entry structure
typedef struct {
	UID      type;	// Partition type GUID
	UID      part;	// Unique partition GUID
	LBA64    first;
	LBA64    last;
	uint64_t flags;
	uint16_t partname[36];	// 72 bytes, 36 UTF-16LE characters
	uint8_t  pad[384];
} GPT_ENTRY;

const char* gpt_part_type_str(UID *uid);
