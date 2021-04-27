#pragma once

#include "utils/os.h"
#include "utils/bin.h"
#include "vdisk/fmt/raw.h"
#include "vdisk/fmt/vdi.h"
#include "vdisk/fmt/vmdk.h"
#include "vdisk/fmt/vhd.h"
#include "vdisk/fmt/vhdx.h"
#include "vdisk/fmt/qed.h"
#include "vdisk/fmt/qcow.h"
#include "vdisk/fmt/phdd.h"

#define VDISK_ERROR(vd,ERR)	vdisk_i_err(vd,ERR,__LINE__,__func__)

//
// Constants
//

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
enum {	// DISKFORMAT magical hints (LSB), used for VDISK.format
	VDISK_FORMAT_NONE	= 0,	// No formats has been specificied yet
	VDISK_FORMAT_RAW	= 0xAAAAAAAA,	// Raw files and storage devices
	VDISK_FORMAT_VDI	= 0x203C3C3C,	// "<<< " VirtualBox
	VDISK_FORMAT_VMDK	= 0x564D444B,	// "VMDK" VMware
	VDISK_FORMAT_VMDK_COW	= 0x44574F43,	// "COWD" VMware EXSi COW disk
	VDISK_FORMAT_VHD	= 0x656E6F63,	// "cone" VirtualPC/Hyper-V
	VDISK_FORMAT_VHDX	= 0x78646876,	// "vhdx" Hyper-V
	VDISK_FORMAT_QED	= 0x00444551,	// "QED\0" QEMU Enhanced Disk
	VDISK_FORMAT_QCOW	= 0xFB494651,	// "QFI\xFB" QEMU Copy-On-Write, v1/v2
	VDISK_FORMAT_PHDD	= 0x68746957,	// "With" Parallels HDD
	VDISK_FORMAT_BOCHS	= 0x68636F42,	// "Boch" Bochs Virtual HD Image
//	VDISK_FORMAT_DMG	= 0x,	// "" Apple DMG
};
#else	// Big-endian
enum {	// DISKFORMAT magical hints (MSB), used for VDISK.format
	VDISK_FORMAT_NONE	= 0,	// No formats has been specificied yet
	VDISK_FORMAT_RAW	= 0xAAAAAAAA,	// Raw files and storage devices
	VDISK_FORMAT_VDI	= 0x3C3C3C20,	// "<<< " VirtualBox
	VDISK_FORMAT_VMDK	= 0x4B444D56,	// "VMDK" VMware
	VDISK_FORMAT_VMDK_COW	= 0x434F5744,	// "COWD" VMware EXSi COW disk
	VDISK_FORMAT_VHD	= 0x636F6E65,	// "cone" VirtualPC/Hyper-V
	VDISK_FORMAT_VHDX	= 0x76686478,	// "vhdx" Hyper-V
	VDISK_FORMAT_QED	= 0x51454400,	// "QED\0" QEMU Enhanced Disk
	VDISK_FORMAT_QCOW	= 0x514649FB,	// "QFI\xFB" QEMU Copy-On-Write, v1/v2
	VDISK_FORMAT_PHDD	= 0x57697468,	// "With" Parallels HDD
	VDISK_FORMAT_BOCHS	= 0x426F6368,	// "Boch" Bochs Virtual HD Image
//	VDISK_FORMAT_DMG	= 0x,	// "" Apple DMG
};
#endif

enum {	// VDISK flags, the open/create flags may overlap
	VDISK_RAW	= 0x1,	// Open or create vdisk as raw

	//
	// vdisk_open flags
	//

	VDISK_OPEN_VDI_ONLY	= 0x1000,	//TODO: Only open successfully if VDISK is VDI
	VDISK_OPEN_VMDK_ONLY	= 0x2000,	//TODO: Only open successfully if VDISK is VMDK
	VDISK_OPEN_VHD_ONLY	= 0x3000,	//TODO: Only open successfully if VDISK is VHD
	VDISK_OPEN_VHDX_ONLY	= 0x4000,	//TODO: Only open successfully if VDISK is VHDX
	VDISK_OPEN_QED_ONLY	= 0x5000,	//TODO: Only open successfully if VDISK is QED
	VDISK_OPEN_QCOW_ONLY	= 0x6000,	//TODO: Only open successfully if VDISK is QCOW
	VDISK_OPEN_PHDD_ONLY	= 0x7000,	//TODO: Only open successfully if VDISK is Parallels HDD
	VDISK_OPEN_BOCHS_ONLY	= 0x8000,	//TODO: Only open successfully if VDISK is Parallels HDD

	//
	// vdisk_create flags
	//

	VDISK_CREATE_TEMP	= 0x0100,	//TODO: Create a temporary (random) vdisk file

	VDISK_CREATE_TYPE_DYNAMIC	= 0x1000,	//TODO: Create a dynamic type VDISK
	VDISK_CREATE_TYPE_FIXED	= 0x2000,	//TODO: Create a fixed type VDISK
	VDISK_CREATE_TYPE_PARENT	= 0x3000,	//TODO: Create a parent of the VDISK
	VDISK_CREATE_TYPE_SNAPSHOT	= 0x4000,	//TODO: Create a snapshot of the VDISK
	VDISK_CREATE_TYPE_MASK	= 0x7000,	// Type mask used internally
};

enum {	// VDISK error codes
	VVD_EOK	= 0,	// VDISK OK
	VVD_EOS	= -2,	// OS/CRT related error
	VVD_ENULL	= -3,	// Input pointer is NULL
	VVD_ENOMEM	= -4,	// Could not allocate memory
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

enum {
	// Operation has completed successfully.
	// Parameter: NULL
	VVD_NOTIF_DONE,
	// VDISK was created with type
	// Parameter: const char*
	VVD_NOTIF_VDISK_CREATED_TYPE_NAME,
	// Total amount of blocks before processing
	// Parameter: uint32_t
	VVD_NOTIF_VDISK_TOTAL_BLOCKS,
	// Total amount of blocks before processing (64-bit indexes)
	// Parameter: uint64_t
	VVD_NOTIF_VDISK_TOTAL_BLOCKS64,
	// 
	// Parameter: uint32_t
	VVD_NOTIF_VDISK_CURRENT_BLOCK,
	// 
	// Parameter: uint64_t
	VVD_NOTIF_VDISK_CURRENT_BLOCK64,
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
	// Reserved.
	uint32_t cookie;
	// Virtual disk capacity in bytes. For RAW files, it's the file size. For
	// RAW devices, it's the disk size. This is populated automatically.
	uint64_t capacity;
	// OS file handle
	__OSFILE fd;
	// Error structure
	struct {
		int num;	// Error number
		int line;	// Source file line number
		const char *func;	// Function name
	} err;
	// Implementation functions
	struct {
		// Read from a disk sector with a LBA index
		int (*lba_read)(struct VDISK*, void*, uint64_t);
		// Write to a disk sector with a LBA index
		int (*lba_write)(struct VDISK*, void*, uint64_t);
		// Read a dynamic block with a block index
		int (*blk_read)(struct VDISK*, void*, uint64_t);
		// Read a sector with a LBA index
		int (*blk_write)(struct VDISK*, void*, uint64_t);
	} cb;
	// Meta union
	union {
		void *meta;
		VDI_META *vdi;
		VMDK_META *vmdk;
		VHD_META *vhd;
		QED_META *qed;
		VHDX_META *vhdx;
	};
} VDISK;

//
// SECTION Internal functions
//

/**
 * (Internal) Set errcode and errline.
 * 
 * \returns errcode
 */
int vdisk_i_err(VDISK *vd, int e, int l, const char *f);

//
// SECTION Functions
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
 * filename (OS).
 * 
 * \param vd VDISK structure
 * \param path OS string path
 * \param flags Opening flags
 * 
 * \returns Exit status
 */
int vdisk_open(VDISK *vd, const oschar *path, uint32_t flags);

/**
 * Create a VDISK.
 * 
 * \param vd VDISK structure
 * \param path OS string path
 * \param format Virtual disk format
 * \param capacity Virtual disk capacity
 * \param flags Creation flags
 * 
 * \returns Exit status
 */
int vdisk_create(VDISK *vd, const oschar *path, int format, uint64_t capacity, uint16_t flags);

/**
 * Returns a string representation of the loaded virtual disk. If a format was
 * not found, a null pointer is returned.
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
// SECTION
//

/**
 * 
 */
int vdisk_op_compact(VDISK *vd, void(*cb)(uint32_t, void*));

/**
 * 
 */
//int vdisk_op_resize(VDISK *vd, void(*cb_progress)(uint64_t block));

//
// SECTION Error handling
//

/**
 * Returns an error message depending on the last value of vdisk_errno. If the
 * error is set to VVD_EOS, the error message will come from the OS (or CRT).
 */
const char* vdisk_error(VDISK *vd);

/**
 * Print to stdout, with the name of the function, a message with the last
 * value set to vdisk_errno.
 */
void vdisk_perror(VDISK *vd);
