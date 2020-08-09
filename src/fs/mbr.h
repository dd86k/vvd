#pragma once

#include <stdio.h>
#include <stdint.h>
#include "../vdisk.h"
#include "../platform.h"

#ifdef ENDIAN_LITTLE
enum {
	MBR_SIG = 0xAA55,	// MBR signature
};
#else
enum {
	MBR_SIG = 0x55AA,	// MBR signature
};
#endif

//TODO: Continue MBR_TYPE entries
// Sources:
// - https://www.win.tue.nl/~aeb/partitions/partition_types-1.html
// - https://en.wikipedia.org/wiki/Partition_type
enum {
	// Empty entry
	MBR_TYPE_00_EMPTY	= 0x00,
	// 12-bit FAT
	MBR_TYPE_01_FAT12	= 0x01,
	// XENIX root
	MBR_TYPE_02_XENIX_ROOT	= 0x02,
	// XENIX /usr
	MBR_TYPE_03_XENIX_USR	= 0x03,
	// FAT16 <32MB
	MBR_TYPE_04_FAT16	= 0x04,
	// Extended partition
	MBR_TYPE_05_EXT_PART	= 0x05,
	// FAT16B >32MB
	MBR_TYPE_06_FAT16B	= 0x06,
	// IFS, HPFS, NTFS, exFAT
	MBR_TYPE_07_NTFS	= 0x07,
	// AIX boot, SplitDrive, Commodore DOS, Dell
	MBR_TYPE_08_AIX_BOOT	= 0x08,
	// AIX data, Coherent FS, QNX ("qnz") 1.x/2.x
	MBR_TYPE_09_AIX_DATA	= 0x09,
	// OS/2 boot, Coherent FS swap, OPUS
	MBR_TYPE_0A_OS2_BOOT	= 0x0A,
	// Win95 OSR2 FAT32
	MBR_TYPE_0B_FAT32	= 0x0B,
	// Win92 OSR2 FAT32 (LBA-mapped)
	MBR_TYPE_0C_FAT32_LBA	= 0x0C,
	// SILICON SAFE
	MBR_TYPE_0D_SILICONSAFE	= 0x0D,
	// Win95 FAT16 (LBA-mapped)
	MBR_TYPE_0E_FAT16_LBA	= 0x0E,
	// Win95 Extended partition (LBA-mapped)
	MBR_TYPE_0F_EXT_PART_LBA	= 0x0F,
	// Hidden FAT12 partition by OS/2
	MBR_TYPE_11_FAT12_HIDDEN	= 0x11,
	// (Compaq) Configuration/diagnostics partition
	MBR_TYPE_12_CONFDIAG_PART	= 0x12,
	// Hidden FAT16 (<32MB) partition by OS/2
	MBR_TYPE_14_FAT16_HIDDEN	= 0x14,
	// Hidden FAT16 (>32MB) partition by OS/2
	MBR_TYPE_16_FAT16B_HIDDEN	= 0x16,
	// Hidden IFS (e.g., HPFS) partition by OS/2
	MBR_TYPE_17_IFS_HIDDEN	= 0x17,
	// AST SmartSleep Partition
	MBR_TYPE_18_AST_SS_PART	= 0x18,
	// Hidden Win95 FAT32
	MBR_TYPE_1B_FAT32_W95_HIDDEN	= 0x1B,
	// Hidden Win95 FAT32 (LBA-mapped)
	MBR_TYPE_1C_FAT32_LBA_W95_HIDDEN	= 0x1C,
	// Hidden Win95 FAT16
	MBR_TYPE_1E_FAT16_W95_HIDDEN	= 0x1E,
	// NEC DOS 3.x
	MBR_TYPE_24_NEC_DOS	= 0x24,
	// MirOS BSD, Windows RE hidden, RouterBOOT kernel, PQservice
	MBR_TYPE_27_MIROS	= 0x27,
	// AtheOS (AFS)
	MBR_TYPE_2A_ATHEOS	= 0x2A,
	// SyllableSecure (SylStor)
	MBR_TYPE_2B_SYLSTOR	= 0x2B,
	// NOS
	MBR_TYPE_32_NOS	= 0x32,
	// JFS on OS/2 or eCS
	MBR_TYPE_35_JFS	= 0x35,
	// THEOS 3.2 2GB Part
	MBR_TYPE_38_THEOS_2GB	= 0x38,
	// Plan9 Part, THEOS 4.0 spanned part
	MBR_TYPE_39_PLAN9	= 0x39,
	// THEOS 4.0 4GB part
	MBR_TYPE_3A_THEOS_4GB	= 0x3A,
	// THEOS 4.0 extended part
	MBR_TYPE_3B_THEOS_EXT	= 0x3B,
	// PartitionMagic recovery part
	MBR_TYPE_3C_PARTMAGIC	= 0x3C,
	// NetWare hidden part
	MBR_TYPE_3D_NETWARE_HIDDEN	= 0x3D,
	// PICK, Venix 80286
	MBR_TYPE_40_PICK	= 0x40,
	// Linux/MINIX (sharing with DRDOS), Personal RISC boot,
	// Power PC Reference Platform boot
	MBR_TYPE_41_DRDOS_LINUX	= 0x41,
	// Linux swap (sharing with DRDOS), SFS (Secure FileSystem),
	// Windows 200 dynamic extended partition marker
	MBR_TYPE_42_DRDOS_LINUX_SWAP	= 0x42,
	// Linux native (sharing with DRDOS)
	MBR_TYPE_43_DRDOS_LINUX_NATIVE	= 0x43,
	// GoBack part
	MBR_TYPE_44_GOBACK	= 0x44,
	// Boot-US boot manager, Priam (Powerquest), EUMEL/Elan
	MBR_TYPE_45_BOOTUS	= 0x45,
	// EUMEL/Elan
	MBR_TYPE_46_EUMEL_ELAN	= 0x46,
	// EUMEL/Elan
	MBR_TYPE_47_EUMEL_ELAN	= 0x47,
	// EUMEL/Elan
	MBR_TYPE_48_EUMEL_ELAN	= 0x48,
};

#define MBR_LBA_A_504_MB	0x1F800000
#define MBR_LBA_A_1008_MB	0x3F000000
#define MBR_LBA_A_2016_MB	0x7E000000
#define MBR_LBA_A_4032_MB	0xFC000000
// 8032.5 MiB
#define MBR_LBA_A_8032_MB	0x1F6080000ULL

typedef struct { // Cylinder-Head-Sector
	uint8_t head;	// HEAD[7:0]
	uint8_t sector;	// SECTOR[5:0], bits[7:6] for CYLINDER[9:8]
	uint8_t cylinder;	// CYLINDER[7:0]
} CHS;

typedef struct {
	uint8_t status;	// Partition status
	CHS chsfirst;	// CHS first absolute address
	uint8_t parttype;	// partition type
	CHS chslast;	// CHS last absolute address
	uint32_t lba;	// LBA of first absolute sector in partition
	uint32_t sectors;	// number of sectors for parition
} MBR_PARTITION;

typedef struct {
	union {
		uint8_t data[512];
		struct {
			uint8_t pad[440];
			uint32_t serial;	// WindowsNT 3.5+, Linux 2.6+
			// Usually 0000H
			// (Windows) 5A5AH if protected
			// (UEFI) AA55H if protective MBR
			uint16_t type;
			MBR_PARTITION pe[4];
			uint16_t sig;
		};
	};
} MBR;

/**
 * Obtain a LBA (sector index) from a CHS geometry. This is based on
 * `LBA = (C × HPC + H) × SPT + (S − 1)`.
 * 
 * \param chs CHS geometry structure
 * 
 * \returns LBA index
 */
uint32_t mbr_lba(CHS *chs);

/**
 * BIOS-assisted LBA translation from a CHS geometry with a disk size. Useful
 * for disks up to 8032.5 MiB.
 * 
 * ---
 * SIZE (MiB)	S/T	H	C
 * < 504	63	16	63 * H * 512
 * 504-1008	63	32	63 * H * 512
 * 1008-2016	63	64	63 * H * 512
 * 2016-4032	63	128	63 * H * 512
 * 4032-8032.5	63	255	63 * H * 512
 * ---
 * 
 * \param chs CHS geometry structure
 * \param dsize Disk size
 * 
 * \returns LBA index
 */
uint32_t mbr_lba_a(CHS *chs, uint64_t dsize);

/**
 * Print MBR information to stdout.
 */
void mbr_info_stdout(MBR *s);

/**
 * Get partition type string
 */
const char *mbr_part_type_str(uint8_t);
