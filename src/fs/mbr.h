#pragma once

#include <stdio.h>
#include <stdint.h>
#include "../vdisk.h"

enum {
	MBR_SIG = 0xAA55, // MBR signature, LSB
};

typedef struct CHS_ENTRY { // Cylinder-Head-Sector
	uint8_t head;	// HEAD[7:0]
	uint8_t sector;	// SECTOR[5:0], bits[7:6] for CYLINDER[9:8]
	uint8_t cylinder;	// CYLINDER[7:0]
} CHS_ENTRY;

typedef struct MBR_PARTITION_ENTRY {
	uint8_t status;
	struct CHS_ENTRY chsfirst;	// CHS first absolute address
	uint8_t partition;	// partition type
	struct CHS_ENTRY chslast;	// CHS last absolute address
	uint32_t lba;	// LBA of first absolute sector in partition
	uint32_t sectors;	// number of sectors for parition
} MBR_PARTITION_ENTRY;

typedef struct MBR {
	union {
		uint8_t data[512];
		struct {
			uint8_t pad[440];
			uint32_t serial;	// WindowsNT 3.5+, Linux 2.6+
			// Usually 0000H
			// (Windows) 5A5AH if protected
			// (UEFI) AA55H if protective MBR
			uint16_t type;
			MBR_PARTITION_ENTRY pe[4];
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
uint32_t mbr_lba(CHS_ENTRY *chs);

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
uint32_t mbr_lba_a(CHS_ENTRY *chs, uint64_t dsize);

/**
 * Print MBR information to stdout.
 */
void mbr_info(MBR *s);
