#include <stdio.h>
#include <inttypes.h> // PRIxxx
#include "../vdisk.h"
#include "../fs/mbr.h"
#include "../utils.h"

void vmdk_info(VDISK *vd) {
	char *comp; // compression
	//if (h.flags & COMPRESSED)
	switch (vd->vmdk.compressAlgorithm) {
	case 0:  comp = "no"; break;
	case 1:  comp = "DEFLATE"; break;
	default: comp = "?";
	}

	char size[BIN_FLENGTH];
	fbins(SECTOR_TO_BYTE(vd->vmdk.capacity), size);
	printf(
		"VMDK, VMware vdisk v%u, %s compression, %s\n"
		"\nCapacity: %"PRIu64" Sectors\n"
		"Overhead: %"PRIu64" Sectors\n"
		"Grain size (Raw): %"PRIu64" Sectors\n",
		vd->vmdk.version, comp, size,
		vd->vmdk.capacity, vd->vmdk.overHead, vd->vmdk.grainSize
	);

	if (vd->vmdk.uncleanShutdown)
		printf("+ Unclean shutdown");
}
