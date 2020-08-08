#pragma once

#include "os.h"
#include "utils.h"
#include "vdisk/vdi.h"
#include "vdisk/vmdk.h"
#include "vdisk/vhd.h"
#include "vdisk/vhdx.h"
#include "vdisk/qed.h"
#include "vdisk/qcow.h"
#include "vdisk/phdd.h"

#define __LINE_BEFORE__ (__LINE__ - 1)

//TODO: Define function declarations for warnings and specific vdisk stuff

//
// Enumerations
//

enum {	// DISKFORMAT magical hints (LSB), used for VDISK.format
	VDISK_FORMAT_NONE	= 0,	// No formats has been specificied yet
	VDISK_FORMAT_RAW	= 0xAAAAAAAA,	// Raw files and devices
	VDISK_FORMAT_VDI	= 0x203C3C3C,	// "<<< " VirtualBox
	VDISK_FORMAT_VMDK	= 0x564D444B,	// "VMDK" VMware
	VDISK_FORMAT_VHD	= 0x656E6F63,	// "cone" VirtualPC/Hyper-V
	VDISK_FORMAT_VHDX	= 0x78646876,	// "vhdx" Hyper-V
	VDISK_FORMAT_QED	= 0x00444551,	// "QED\0" QEMU Enhanced Disk
	VDISK_FORMAT_QCOW	= 0xFB494651,	// "QFI\xFB" QEMU Copy-On-Write, v1/v2
	VDISK_FORMAT_PHDD	= 0x68746957,	// "With" Parallels HDD
//	VDISK_FORMAT_DMG	= 0x,	// "" Apple DMG
};

enum {	// VDISK flags, the open/create flags may intertwine in values
	VDISK_RAW	= 0x1,	// Open or create vdisk as raw

	//
	// vdisk_open flags
	//

	VDISK_OPEN_VDI_ONLY	= 0x1000,	// Only open successfully if VDISK is VDI
	VDISK_OPEN_VMDK_ONLY	= 0x2000,	// Only open successfully if VDISK is VMDK
	VDISK_OPEN_VHD_ONLY	= 0x3000,	// Only open successfully if VDISK is VHD
	VDISK_OPEN_VHDX_ONLY	= 0x4000,	// Only open successfully if VDISK is VHDX
	VDISK_OPEN_QED_ONLY	= 0x5000,	// Only open successfully if VDISK is QED
	VDISK_OPEN_QCOW_ONLY	= 0x6000,	// Only open successfully if VDISK is QCOW

	//
	// vdisk_create flags
	//

	VDISK_CREATE_TEMP	= 0x0100,	// Create a temporary (random) vdisk file
	VDISK_CREATE_DYN	= 0x1000,	// Create a dynamic type VDISK
	VDISK_CREATE_FIXED	= 0x2000,	// Create a fixed type VDISK
};

enum {	// VDISK error codes
	VVD_EOK	= 0,	// VDISK OK
	VVD_EOS	= -2,	// OS/CRT related error
	VVD_ENULL	= -3,	// Input pointer is NULL
	VVD_EALLOC	= -4,	// Could not allocate memory
	VVD_EVDFORMAT	= -10,	// Invalid VDISK format
	VVD_EVDMAGIC	= -11,	// Invalid VDISK magic signature
	VVD_EVDVERSION	= -12,	// Unsupported VDISK version (major)
	VVD_EVDTYPE	= -13,	// Unsupported VDISK type
	VVD_EVDFULL	= -14,	// VDISK is full and no more data can be allocated
	VVD_EVDUNALLOC	= -15,	// Block is unallocated
	VVD_EVDBOUND	= -16,	// Index was out of block index bounds
	VVD_EVDTODO	= -254,	// Currently unimplemented
	VVD_EVDMISC	= -255,	// Unknown
};

//
// Structure definitions
//

// Defines a virtual disk.
// All fields are more or less internal.
typedef struct VDISK {
	// Defines the virtual disk format (e.g. VDI, VMDK, etc.).
	// See VDISKFORMAT enumeration.
	uint32_t format;
	// Flags. See VDISK_FLAG enumeration.
	uint32_t flags;
	// Calculated absolute offset to data.
	uint32_t offset;
	// (Internal) Location of new allocation block
	uint64_t nextblock;
	// Virtual disk capacity in bytes. For RAW files, it's the file size. For
	// RAW devices, it's the disk size.
	uint64_t capacity;
	// (Posix) File descriptor (Windows) File HANDLE
	__OSFILE fd;
	//
	// Error members
	//
	int errcode;	// Error number
	int errline;	// Error line
	const char *errfunc;	// Function name
	//
	// Function pointers
	//
	// Return absolute file position from an LBA index
	//int (*poslba)(struct VDISK*, uint64_t*);
	// Return absolute file position from a block index
	//int (*posblock)(struct VDISK*, uint64_t*);
	//int (*read_lba)(struct VDISK*, void*, uint64_t);
	//int (*read_block)(struct VDISK*, void*, uint64_t);
	//int (*write_lba)(struct VDISK*, void*, uint64_t);
	//int (*write_block)(struct VDISK*, void*, uint64_t);
	//
	// BATs
	//
	union {
		// Allocation table using 64-bit indexes
		uint64_t *u64block;
		// Allocation table using 32-bit indexes
		uint32_t *u32block;
	};
	union {
		// Total amount of allocated blocks
		uint64_t u64blockcount;
		// Total amount of allocated blocks
		uint32_t u32blockcount;
	};
	// To avoid wasting memory space, and since a VDISK can only hold one
	// format at a time, all structures are unionized. Version translation
	// and header/format validity are done in vdisk_open.
	//TODO: Consider making these pointers and only allocate required structures
	union {
		struct {
			VDI_HDR vdihdr;
			VDIHEADER1 vdi;
			// Dynamic disk block mask for remaining offset to LBA.
			uint32_t vdi_blockmask;
			// Dynamic disk block index shift number. Populated by fpow2.
			uint32_t vdi_blockshift;
		};
		VMDK_HDR vmdk;
		struct {
			VHD_HDR vhd;
			VHD_DYN_HDR vhddyn;
		};
		struct {
			VHDX_HDR vhdx;
			VHDX_HEADER1 vhdxhdr;
			VHDX_REGION_HDR vhdxreg;
		};
		struct {
			QED_HDR qed;
			uint64_t qed_offset_mask;
			uint64_t qed_L2_mask;
			uint32_t qed_L2_shift;
			uint64_t qed_L1_mask;
			uint32_t qed_L1_shift;
			uint64_t *qed_L1; // L1 table
			uint64_t *qed_L2; // L2 table
		};
		QCOW_HDR qcow;
		PHDD_HDR phdd;
	};
} VDISK;

//
// Functions
//

/**
 * (Internal) Set errcode and errline.
 * 
 * \returns errcode
 */
int vdisk_i_err(VDISK *vd, int e, int l);

/**
 * Open a VDISK.
 * 
 * When opening a file, this function verifies the file path, VDISK format,
 * header structure, version, and other fields.
 * 
 * When creating a file, the specified file at the file path is overwritten.
 * An empty, unallocated VDISK is created. If VDISK_CREATE_TEMP is defined,
 * path parameter can be NULL, since the function will create a random
 * filename (OS).
 * 
 * \param vd VDISK structure
 * \param path OS string path
 * \param flags Flags
 * 
 * \returns Exit status
 */
int vdisk_open(VDISK *vd, const _oschar *path, uint32_t flags);

/**
 * Create a VDISK.
 * 
 * \param vd VDISK structure
 * \param path OS string path
 * \param format Virtual disk format
 * \param capacity Virtual disk capacity
 * \param flags Flags
 * 
 * \returns Exit status
 */
int vdisk_create(VDISK *vd, const _oschar *path, int format, uint64_t capacity, uint16_t flags);

/**
 * 
 */
const char *vdisk_str(VDISK *vd);

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
int vdisk_read_sector(VDISK *vd, void *buffer, uint64_t lba);

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
 * Returns an error message depending on the last value of vdisk_errno.
 */
const char* vdisk_error(VDISK *vd);

/**
 * Print to stdout, with the name of the function, a message with the last
 * value set to vdisk_errno.
 */
void vdisk_perror(VDISK *vd);
