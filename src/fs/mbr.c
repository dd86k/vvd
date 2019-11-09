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
	if (mbr.pe1.partition < 0xEE || mbr.pe1.partition > 0xEF)
		return;
	if (vdisk_read_lba(vd, &mbr, 1)) return;
	if (gpt_check((GPT *)&mbr)) {
		gpt_info((GPT *)&mbr);
		gpt_list_pe_vd(vd, (GPT *)&mbr);
	}
}

void mbr_info(MBR *mbr) {
	char size[BIN_FLENGTH];
	uint64_t dtsize = SECTOR_TO_BYTE(
		(uint64_t)mbr->pe1.sectors + (uint64_t)mbr->pe2.sectors +
		(uint64_t)mbr->pe3.sectors + (uint64_t)mbr->pe4.sectors
	);
	fbins(dtsize, size);
	printf(
	"\n* MBR, SERIAL %08X, USED %s, TYPE %04u\n"
	"PARTITIONS  STATUS  TYPE        LBA       SIZE  C:H:S->C:H:S\n"
	"ENTRY 1       %3XH  %3XH  %9u  %9u  %u:%u:%u->%u:%u:%u\n"
	"ENTRY 2       %3XH  %3XH  %9u  %9u  %u:%u:%u->%u:%u:%u\n"
	"ENTRY 3       %3XH  %3XH  %9u  %9u  %u:%u:%u->%u:%u:%u\n"
	"ENTRY 4       %3XH  %3XH  %9u  %9u  %u:%u:%u->%u:%u:%u\n"
	,
	mbr->serial, size, mbr->type,
	// PE 1
	mbr->pe1.status,
	mbr->pe1.partition,
	mbr->pe1.lba,
	mbr->pe1.sectors,
	mbr->pe1.chsfirst.cylinder |
	((mbr->pe1.chsfirst.sector & 0xC0) << 2),
	mbr->pe1.chsfirst.head,
	mbr->pe1.chsfirst.sector & 0x3F,
	mbr->pe1.chslast.cylinder |
	((mbr->pe1.chslast.sector & 0xC0) << 2),
	mbr->pe1.chslast.head,
	mbr->pe1.chslast.sector & 0x3F,
	// PE 2
	mbr->pe2.status,
	mbr->pe2.partition,
	mbr->pe2.lba,
	mbr->pe2.sectors,
	mbr->pe2.chsfirst.cylinder |
	((mbr->pe2.chsfirst.sector & 0xC0) << 2),
	mbr->pe2.chsfirst.head,
	mbr->pe2.chsfirst.sector & 0x3F,
	mbr->pe2.chslast.cylinder |
	((mbr->pe2.chslast.sector & 0xC0) << 2),
	mbr->pe2.chslast.head,
	mbr->pe2.chslast.sector & 0x3F,
	// PE 3
	mbr->pe3.status,
	mbr->pe3.partition,
	mbr->pe3.lba,
	mbr->pe3.sectors,
	mbr->pe3.chsfirst.cylinder |
	((mbr->pe3.chsfirst.sector & 0xC0) << 2),
	mbr->pe3.chsfirst.head,
	mbr->pe3.chsfirst.sector & 0x3F,
	mbr->pe3.chslast.cylinder |
	((mbr->pe3.chslast.sector & 0xC0) << 2),
	mbr->pe3.chslast.head,
	mbr->pe3.chslast.sector & 0x3F,
	// PE 4
	mbr->pe4.status,
	mbr->pe4.partition,
	mbr->pe4.lba,
	mbr->pe4.sectors,
	mbr->pe4.chsfirst.cylinder |
	((mbr->pe4.chsfirst.sector & 0xC0) << 2),
	mbr->pe4.chsfirst.head,
	mbr->pe4.chsfirst.sector & 0x3F,
	mbr->pe4.chslast.cylinder |
	((mbr->pe4.chslast.sector & 0xC0) << 2),
	mbr->pe4.chslast.head,
	mbr->pe4.chslast.sector & 0x3F
	);
}
