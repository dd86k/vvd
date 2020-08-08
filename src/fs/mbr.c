#include <stdio.h>
#include <stdint.h>
#include "mbr.h"
#include <assert.h>

//
// mbr_lba
//

uint32_t mbr_lba(CHS *chs) {
	// LBA = (C * HPC + H) * SPT + (S − 1)
	// HPC	Max heads per cylinders, typically 16 (28-bit LBA)
	// SPT	Max sectors per strack, typically 63 (28-bit LBA)
	uint8_t sector = chs->sector & 0x3F;
	uint16_t cylinder = chs->cylinder | ((chs->sector & 0xC0) << 2);
	return (cylinder * 16 * chs->head) * 63 + (sector - 1);
}

//
// mbr_lba_a
//

uint32_t mbr_lba_a(CHS *chs, uint64_t dsize) {
	uint8_t HPC;

	if (dsize <= MBR_LBA_A_504_MB)
		HPC = 16;
	else if (dsize <= MBR_LBA_A_1008_MB)
		HPC = 32;
	else if (dsize <= MBR_LBA_A_2016_MB)
		HPC = 64;
	else if (dsize <= MBR_LBA_A_4032_MB)
		HPC = 128;
	else if (dsize <= MBR_LBA_A_8032_MB)
		HPC = 255;
	else
		HPC = 16; //TODO: Verify HPC value when disksize >8.5 GiB

	uint8_t sector = chs->sector & 0x3F;
	uint16_t cylinder = chs->cylinder | ((chs->sector & 0xC0) << 2);
	return (cylinder * HPC * chs->head) * 63 + (sector - 1);
}

//
// mbr_info_stdout
//

void mbr_info_stdout(MBR *mbr) {
	uint64_t dtsize = SECTOR_TO_BYTE(
		(uint64_t)mbr->pe[0].sectors + (uint64_t)mbr->pe[1].sectors +
		(uint64_t)mbr->pe[2].sectors + (uint64_t)mbr->pe[3].sectors
	);
	char size[BIN_FLENGTH];
	fbins(dtsize, size);

	printf(
	"\nMBR: SERIAL %08X, %s USED, TYPE %04u\n"
	"   STAT  TYPE        LBA        SIZE  C:H:S start-end\n",
	mbr->serial, size, mbr->type
	);

	for (unsigned int i = 0; i < 4; ++i) {
		MBR_PARTITION pe = mbr->pe[i];
		printf(
		"%u.  %2XH   %2XH %10u  %10u  %4u:%3u:%2u-%4u:%3u:%2u\n",
		i, pe.status, pe.parttype, pe.lba, pe.sectors,
		// CHS start
		pe.chsfirst.cylinder | ((pe.chsfirst.sector & 0xC0) << 2),
		pe.chsfirst.head, pe.chsfirst.sector & 0x3F,
		// CHS end
		pe.chslast.cylinder | ((pe.chslast.sector & 0xC0) << 2),
		pe.chslast.head, pe.chslast.sector & 0x3F
		);
	}
}

//
// mbr_part_type_str
//

const char *mbr_part_type_str(uint8_t type) {
	assert(0);
	return NULL;
}
