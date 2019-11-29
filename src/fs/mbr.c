#include <stdio.h>
#include <stdint.h>
#include "mbr.h"
#include "gpt.h"
#include "../vdisk.h"

// Maps CHS geometry to an LBA (sector index)
// LBA = (C × HPC + H) × SPT + (S − 1)
// HPC	Max heads per cylinders, typically 16 (28-bit LBA)
// SPT	Max sectors per strack, typically 63 (28-bit LBA)
uint32_t mbr_lba(CHS_ENTRY *chs) {
	uint8_t sector = chs->sector & 0x3F;
	uint16_t cylinder = chs->cylinder | ((chs->sector & 0xC0) << 2);
	return (cylinder * 16 * chs->head) * 63 + (sector - 1);
}

// LBA-ASSISTED TRANSLATION for disks under 8032.5 MiB
// SIZE (MiB)	S/T	H	C
// < 504	63	16	63 * H * 512
// 504-1008	63	32	63 * H * 512
// 1008-2016	63	64	63 * H * 512
// 2016-4032	63	128	63 * H * 512
// 4032-8032.5	63	255	63 * H * 512
uint32_t mbr_lba_a(CHS_ENTRY *chs, uint64_t bsize) {
	uint8_t H;
	if (bsize <= 0x1F800000) // 504 MiB
		H = 16;
	else if (bsize <= 0x3F000000) // 1008 MiB
		H = 32;
	else if (bsize <= 0x7E000000) // 2016 MiB
		H = 64;
	else if (bsize <= 0xFC000000) // 4032 MiB
		H = 128;
	else if (bsize <= 0x1F6080000ULL) // 8032.5 MiB
		H = 255;
	else
		H = 16; //TODO: Verify H when >8.5G
	
	uint8_t sector = chs->sector & 0x3F;
	uint16_t cylinder = chs->cylinder | ((chs->sector & 0xC0) << 2);
	return (cylinder * H * chs->head) * 63 + (sector - 1);
}

// Checks for MBR signature
int mbr_check(MBR *mbr) {
	return mbr->sig == MBR_SIG;
}

// And also checks GPT automatically
void mbr_info_auto(VDISK *vd) {
	MBR mbr;
	if (vdisk_read_lba(vd, &mbr, 0)) return;
	if (mbr_check(&mbr) == 0) return;
	mbr_info(&mbr);
	// EFI GPT Protective or EFI System Partition
	if (mbr.pe[0].partition < 0xEE || mbr.pe[0].partition > 0xEF)
		return;
	if (vdisk_read_lba(vd, &mbr, 1)) return;
	if (gpt_check((GPT *)&mbr)) {
		gpt_info((GPT *)&mbr);
		gpt_list_pe_vd(vd, (GPT *)&mbr);
	}
}

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
