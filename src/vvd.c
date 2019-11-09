/**
 * Main vvd operations
 */
#include <assert.h>
#include <string.h> // memcpy
#include <inttypes.h>
#include "vvd.h"
#include "utils.h"
#include "fs/mbr.h"

//
// vvd_info
//

int vvd_info(VDISK *vd) {
	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		vdi_info(vd);
		break;
	case VDISK_FORMAT_VMDK:
		vmdk_info(vd);
		break;
	case VDISK_FORMAT_VHD:
		vhd_info(vd);
		break;
	case VDISK_FORMAT_RAW: break; // No header info
	default:
		fputs("vvd_info: Format not supported\n", stderr);
		return ECLIFORMAT;
	}

	mbr_info_auto(vd);
	return EVDOK;
}

//
// vvd_map
//

int vvd_map(VDISK *vd) { //TODO: Add column width
	char bsizestr[BIN_FLENGTH]; // If used
	uint64_t blocksize;
	uint32_t blocksn = vd->u32nblocks;
	uint32_t *blocks = vd->u32blocks;

	switch (vd->format) {
	case VDISK_FORMAT_VHD:
		if (vd->vhd.type != VHD_DISK_DYN) {
			fputs("vvd_map: VHD is not dynamic\n", stderr);
			return EVDTYPE;
		}
		blocksize = vd->vhddyn.blocksize;
		break;
	case VDISK_FORMAT_VDI:
		blocksize = vd->vdi.blocksize;
		blocksn = vd->vdi.totalblocks;
		break;
	default:
		fputs("vvd_map: Unsupported format\n", stderr);
		return EVDFORMAT;
	}

	fbins(blocksize, bsizestr);
	printf(
	"Allocation map (%u blocks at %s)\n"
	"          |        0 |        1 |        2 |        3 |"
	           "        4 |        5 |        6 |        7 |\n"
	"----------+----------+----------+----------+----------+"
	           "----------+----------+----------+----------+\n",
	blocksn, bsizestr
	);
	size_t i = 0;
	size_t bn = blocksn - 8;
	for (; i < bn; i += 8) {
		printf(
		" %8zu | %8X | %8X | %8X | %8X | %8X | %8X | %8X | %8X |\n",
		i,
		blocks[i],     blocks[i + 1],
		blocks[i + 2], blocks[i + 3],
		blocks[i + 4], blocks[i + 5],
		blocks[i + 6], blocks[i + 7]
		);
	}
	if (blocksn - i > 0) { // Left over
		printf(" %8zu |", i);
		for (; i < blocksn; ++i)
			printf(" %8X |", blocks[i]);
		putchar('\n');
	}
	return ECLIOK;
}

//
// vvd_new
//

int vvd_new(VDISK *vd, uint64_t vsize) {
	uint8_t *buffer;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		vd->u32nblocks = vsize / vd->vdi.blocksize;
		if (vd->u32nblocks == 0)
			vd->u32nblocks = 1;
		uint32_t bsize = vd->u32nblocks << 2;
		if ((vd->u32blocks = malloc(bsize)) == NULL)
			return ECLIALLOC;
		vd->vdi.totalblocks = vd->u32nblocks;
		vd->vdi.disksize = vsize;
		switch (vd->vdi.type) {
		case VDI_DISK_DYN:
			for (size_t i = 0; i < vd->vdi.totalblocks; ++i)
				vd->u32blocks[i] = VDI_BLOCK_UNALLOCATED;
			break;
		case VDI_DISK_FIXED:
			if ((buffer = malloc(vd->vdi.blocksize)) == NULL)
				return ECLIALLOC;
			os_seek(vd->fd, vd->vdi.offData, SEEK_SET);
			for (size_t i = 0; i < vd->vdi.totalblocks; ++i) {
				vd->u32blocks[i] = VDI_BLOCK_FREE;
				os_write(vd->fd, buffer, vd->vdi.blocksize);
			}
			break;
		default:
			fputs("vvd_new: Type currently unsupported", stderr);
			return EVDTYPE;
		}
		break;
	default:
		fputs("vvd_new: Format currently unsupported", stderr);
		return EVDFORMAT;
	}

L_DISKEMPTY:
	if (vdisk_update_headers(vd)) {
		fputs("vvd_new: Could not write headers\n", stderr);
		return 1;
	}
	return 0;
}

//
// vvd_compact
//

int vvd_compact(VDISK *vd) {
	if (vd->u32nblocks == 0) {
		fputs("vvd_compact: No allocated blocks\n", stderr);
		return EVDMISC;
	}

	switch (vd->format) {
	case VDISK_FORMAT_VDI: return vdi_compact(vd);
	case VDISK_FORMAT_RAW:
		fputs("vvd_compact: RAW files/devices are not supported\n",
			stderr);
		return EVDFORMAT;
	default:
		fputs("vvd_compact: VDISK format not supported\n",
			stderr);
		return EVDFORMAT;
	}

	return 0;
}

//
// vvd_write (file)
//

int vvd_write(VDISK *vd, __OSFILE *fd) {
	
	
	return 0;
}
