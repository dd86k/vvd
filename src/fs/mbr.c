#include <stdio.h>
#include <stdint.h>
#include "mbr.h"
#include "gpt.h"
#include "../vdisk.h"

//
// mbr_lba
//

uint32_t mbr_lba(CHS_ENTRY *chs) {
	// LBA = (C × HPC + H) × SPT + (S − 1)
	// HPC	Max heads per cylinders, typically 16 (28-bit LBA)
	// SPT	Max sectors per strack, typically 63 (28-bit LBA)
	uint8_t sector = chs->sector & 0x3F;
	uint16_t cylinder = chs->cylinder | ((chs->sector & 0xC0) << 2);
	return (cylinder * 16 * chs->head) * 63 + (sector - 1);
}

//
// mbr_lba_a
//

uint32_t mbr_lba_a(CHS_ENTRY *chs, uint64_t dsize) {
	uint8_t H;
	if (dsize <= 0x1F800000) // 504 MiB
		H = 16;
	else if (dsize <= 0x3F000000) // 1008 MiB
		H = 32;
	else if (dsize <= 0x7E000000) // 2016 MiB
		H = 64;
	else if (dsize <= 0xFC000000) // 4032 MiB
		H = 128;
	else if (dsize <= 0x1F6080000ULL) // 8032.5 MiB
		H = 255;
	else
		H = 16; //TODO: Verify H when >8.5 GiB
	
	uint8_t sector = chs->sector & 0x3F;
	uint16_t cylinder = chs->cylinder | ((chs->sector & 0xC0) << 2);
	return (cylinder * H * chs->head) * 63 + (sector - 1);
}

//
// mbr_info
//

void mbr_info(MBR *mbr) {
	uint64_t dtsize = SECTOR_TO_BYTE(
		(uint64_t)mbr->pe[0].sectors + (uint64_t)mbr->pe[1].sectors +
		(uint64_t)mbr->pe[2].sectors + (uint64_t)mbr->pe[3].sectors
	);
	char size[BIN_FLENGTH];
	fbins(dtsize, size);
	
	printf(
	"\n* MBR, SERIAL %08X, USED %s, TYPE %04u\n"
	"PARTITIONS  STATUS  TYPE        LBA        SIZE  C:H:S->C:H:S\n",
	mbr->serial, size, mbr->type
	);
	for (unsigned int i = 0; i < 4; ++i)
		printf(
		"ENTRY %u       %3XH  %3XH  %9u  %10u  %u:%u:%u->%u:%u:%u\n",
		i,
		mbr->pe[i].status,
		mbr->pe[i].partition,
		mbr->pe[i].lba,
		mbr->pe[i].sectors,
		mbr->pe[i].chsfirst.cylinder |
		((mbr->pe[i].chsfirst.sector & 0xC0) << 2),
		mbr->pe[i].chsfirst.head,
		mbr->pe[i].chsfirst.sector & 0x3F,
		mbr->pe[i].chslast.cylinder |
		((mbr->pe[i].chslast.sector & 0xC0) << 2),
		mbr->pe[i].chslast.head,
		mbr->pe[i].chslast.sector & 0x3F
		);
}
