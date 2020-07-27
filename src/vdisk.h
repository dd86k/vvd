#pragma once

#include "os/os.h"
#include "utils.h"
#include "vdisk/vdi.h"
#include "vdisk/vmdk.h"
#include "vdisk/vhd.h"
#include "vdisk/vhdx.h"

#define __LINE_BEFORE__ (__LINE__ - 1)
#define DEFAULT_BLOCKSIZE 1048576

//
// Global variables
//

/**
 * (Internal) Last error number set by vdisk_* functions.
 */
int vdisk_errno;
/**
 * 
 */
int vdisk_errln;
/**
 * 
 */
const char *vdisk_func;

//
// Enumerations
//

enum {	// DISKFORMAT magical hints (LSB), used for VDISK.format
	VDISK_FORMAT_NONE	= 0,	// No formats has been specificied yet
	VDISK_FORMAT_RAW	= 0xAAAAAAAA,	// Files/Devices
	VDISK_FORMAT_VDI	= 0x203C3C3C,	// "<<< " VirtualBox
	VDISK_FORMAT_VMDK	= 0x564D444B,	// "VMDK" VMware
	VDISK_FORMAT_VHD	= 0x656E6F63,	// "cone" VirtualPC/Hyper-V
	VDISK_FORMAT_VHDX	= 0x78646876,	// "vhdx" Hyper-V
	VDISK_FORMAT_QED	= 0x00444551,	// "QED\0" QEMU Enhanced Disk
	VDISK_FORMAT_QCOW	= 0xFB494651,	// "QFI\xFB" QEMU Copy-On-Write, v1/v2
//	VDISK_FORMAT_DMG	= 0x,	// "" Apple DMG
//	VDISK_FORMAT_PARAHDD	= 0x,	// "" Parallels HDD
//	VDISK_FORMAT_CUE	= 0x,	// "" Cue/Bin, Disk metadata
};

enum {	// VDISK flags for vdisk_open
	VDISK_RAW	= 0x1,	// Open or create vdisk as raw
	VDISK_CREATE	= 0x2,	// Create a vdisk if it doesn't exist
	VDISK_CREATE_TEMP	= 0x4,	// Create a temporary (random) vdisk file
	VDISK_CREATE_INIT	= 0x8,	// Init disk when creating vdisk

	VDISK_OPEN_VDI_ONLY	= 0x100,	// Open/create if VDISK is VDI
	VDISK_OPEN_VMDK_ONLY	= 0x200,	// Open/create if VDISK is VMDK
	VDISK_OPEN_VHD_ONLY	= 0x300,	// Open/create if VDISK is VHD
	VDISK_OPEN_VHDX_ONLY	= 0x400,	// Open/create if VDISK is VHDX
	VDISK_OPEN_QED_ONLY	= 0x500,	// Open/create if VDISK is QED
	VDISK_OPEN_QCOW_ONLY	= 0x600,	// Open/create if VDISK is QCOW

	VDISK_CREATE_DYN	= 0x1000,	// Create a dynamic type VDISK
	VDISK_CREATE_FIXED	= 0x2000,	// Create a fixed type VDISK
};

enum {	// VDISK error codes
	VVD_EVDOK	= 0,	// VDISK OK
	VVD_EVDOPEN	= -1,	// VDISK could not be opened nor created
	VVD_EVDREAD	= -10,	// Error reading VDISK
	VVD_EVDSEEK	= -11,	// Error seeking VDISK
	VVD_EVDWRITE	= -12,	// Error seeking VDISK
	VVD_EVDFORMAT	= -13,	// Invalid VDISK format
	VVD_EVDMAGIC	= -14,	// Invalid VDISK magic signature
	VVD_EVDVERSION	= -15,	// Unsupported VDISK version (major)
	VVD_EVDTYPE	= -16,	// Unsupported VDISK type
	VVD_EVDFULL	= -17,	// VDISK is full and no more data can be allocated
	VVD_EVDUNALLOC	= -18,	// Block is unallocated
	VVD_EVDBOUND	= -19,	// Index was out of block index bounds
	VVD_EVDALLOC	= -20,	// Could not allocate memory
	VVD_EVDMISC	= -255,	// Unknown
};

//
// Structure definitions
//

typedef struct VDISK {
	uint32_t format;	// See VDISKFORMAT, used by vdisk_open
	uint32_t flags;	// See VDISK_FLAG
	uint32_t offset;	// Calculated absolute data offset on disk
	uint64_t nextblock;	// (Internal) Location of new allocation block
	// VHDX:
	//uint64_t vsize;	// Calculated capacity, virtual size
	__OSFILE fd;	// File descriptor or handle
	union {
		uint64_t *u64blocks;	// 64-bit allocation blocks
		uint32_t *u32blocks;	// 32-bit allocation blocks
	};
	union {
		uint64_t u64nblocks;	// Total amount of allocated blocks
		uint32_t u32nblocks;	// Total amount of allocated blocks
	};
	//void (*read_lba)(VDISK *, void *, uint64_t);
	//void (*write_lba)(VDISK *, void *, uint64_t);
	//void (*read_seq)(VDISK *, void *);
	//void (*write_seq)(VDISK *, void *);
	// To avoid wasting memory space, and since a VDISK can only hold one
	// format at a time, all structures are unionized. Version translation
	// and header/format validity are done in vdisk_open.
	union {
		struct { // VDI
			VDI_HDR vdihdr;
			VDIHEADER1 vdi;
		};
			// VMDK
		struct VMDK_HDR vmdk;
		struct { // VHD
			VHD_HDR vhd;
			VHD_DYN_HDR vhddyn;
		};
		struct { // VHDX
			VHDX_HDR vhdx;
			VHDX_HEADER1 vhdxhdr;
			VHDX_REGION_HDR vhdxreg;
		};
			// QED
			// QCOW
	};
} VDISK;

//
// Functions
//

/**
 * Open a VDISK.
 * 
 * When opening a file, this function verifies the file path, VDISK format,
 * header structure, version, and other fields.
 * 
 * When creating a file, the specified file at the file path is overwritten.
 * An empty, unallocated VDISK is created. If VDISK_CREATE_TEMP is defined,
 * path parameter can be NULL, since the function will create a random
 * filename (OS). The fields are NOT populated, to 
 * 
 * \returns Error code if non-zero.
 */
int vdisk_open(const _vchar *path, VDISK *vd, uint16_t flags);

/**
 * Create a VDISK.
 */
int vdisk_create(const _vchar *path, VDISK *vd, uint16_t flags);

/**
 * Initiate VDISK with default/empty structure values.
 */
int vdisk_default(VDISK *vd);

/**
 * 
 */
char *vdisk_str(VDISK *vd);

/**
 * Update header information and allocation tables into file or device.
 */
int vdisk_update(VDISK *vd);

/**
 * Seek and read a sector-size (512 bytes) of data from a sector index (LBA).
 * 
 * This function checks if sector exists on dynamic type disks, and index
 * tables such as the BAT on VHDs.
 * 
 * Returns error code. Non-zero being an error.
 */
int vdisk_read_lba(VDISK *vd, void *buffer, uint64_t lba);

/**
 * Seek to a block index and read it. The size of the block depends on the size
 * speicified in the VDISK structure. Only certain VDISK types are supported,
 * notably dynamic types. If unsupported, returns EVDFORMAT or EVDTYPE.
 */
int vdisk_read_block(VDISK *vd, void *buffer, uint64_t index);

/**
 * 
 */
int vdisk_write_lba(VDISK *vd, void *buffer, uint64_t lba);

/**
 * 
 */
int vdisk_write_block(VDISK *vd, void *buffer, uint64_t index);

/**
 * 
 */
int vdisk_write_block_at(VDISK *vd, void *buffer, uint64_t bindex, uint64_t dindex);

//
// Error handling
//

/**
 * 
 */
char* vdisk_error();

/**
 * 
 */
void vdisk_perror(const char *func);

/**
 * 
 */
int vdisk_last_errno();
