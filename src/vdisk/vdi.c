#include <stdio.h>
#include <string.h> // memcpy
#include "../utils.h"
#include "../vdisk.h"

void vdi_info(VDISK *vd) {
	char *type; // vdisk type
	switch (vd->vdi.type) {
	case VDI_DISK_DYN:	type = "dynamic"; break;
	case VDI_DISK_FIXED:	type = "fixed"; break;
	case VDI_DISK_UNDO:	type = "undo"; break;
	case VDI_DISK_DIFF:	type = "diff"; break;
	default:	type = "type?";
	}

	char disksize[BIN_FLENGTH];
	char bsize[BIN_FLENGTH];	// block size
	char create_uuid[GUID_TEXT_SIZE], modify_uuid[GUID_TEXT_SIZE],
		link_uuid[GUID_TEXT_SIZE], parent_uuid[GUID_TEXT_SIZE];

	fbins(vd->vdi.disksize, disksize);
	fbins(vd->vdi.blocksize, bsize);
	uuid_tostr(create_uuid, &vd->vdi.uuidCreate);
	uuid_tostr(modify_uuid, &vd->vdi.uuidModify);
	uuid_tostr(link_uuid, &vd->vdi.uuidLinkage);
	uuid_tostr(parent_uuid, &vd->vdi.uuidParentModify);

	printf(
	"VDI, VirtualBox %s vdisk v%u.%u, %s\n"
	"Header size: %u, Flags: %XH, Dummy: %u\n"
	"Blocks: %u (allocated: %u, extra: %u), %s size\n"
	"Offset to data: %Xh, to alloc blocks: %Xh\n"
	"Cylinders: %u (legacy: %u)\n"
	"Heads: %u (legacy: %u)\n"
	"Sectors: %u (legacy: %u)\n"
	"Sector size: %u (legacy: %u)\n"
	"Create UUID : %s\n"
	"Modifiy UUID: %s\n"
	"Linkage UUID: %s\n"
	"Parent UUID : %s\n",
	type, vd->vdihdr.majorv, vd->vdihdr.minorv, disksize,
	vd->vdi.hdrsize, vd->vdi.fFlags, vd->vdi.u32Dummy,
	vd->vdi.totalblocks, vd->vdi.blocksalloc, vd->vdi.blocksextra, bsize,
	vd->vdi.offData, vd->vdi.offBlocks,
	vd->vdi.cCylinders, vd->vdi.LegacyGeometry.cCylinders,
	vd->vdi.cHeads, vd->vdi.LegacyGeometry.cHeads,
	vd->vdi.cSectors, vd->vdi.LegacyGeometry.cSectors,
	vd->vdi.cbSector, vd->vdi.LegacyGeometry.cbSector,
	create_uuid, modify_uuid, link_uuid, parent_uuid
	);
}

int vdi_compact(VDISK *vd) {
	if (vd->format != VDISK_FORMAT_VDI) {
		fputs("vdi_compact: vdisk not VDI\n", stderr);
		return EVDFORMAT;
	}
	if (vd->vdi.type != VDI_DISK_DYN) {
		fputs("vdi_compact: vdisk not dynamic\n", stderr);
		return EVDTYPE;
	}

	uint8_t *buffer;	// Block buffer

	//
	// Block buffer for transfer
	//

	if ((buffer = malloc(vd->vdi.blocksize)) == NULL)
		return EVDALLOC;

	//
	// Temporary VDISK
	//
	// Also assigns the same attributes from the source vdisk
	//

	VDISK vdtmp;
	if ((vdtmp.u32blocks = malloc(vd->u32nblocks << 2)) == NULL)
		return EVDALLOC;
	vdtmp.offset = vd->offset;
	vdtmp.format = vd->format;
	vdtmp.u32nblocks = vd->u32nblocks;
	for (size_t i = 0; i < vdtmp.u32nblocks; ++i)
		vdtmp.u32blocks[i] = VDI_BLOCK_UNALLOCATED;
	memcpy(&vdtmp.vdihdr, &vd->vdihdr, sizeof(VDI_HDR));
	memcpy(&vdtmp.vdi, &vd->vdi, sizeof(VDIHEADER1));
	if (vdisk_open(NULL, &vdtmp, VDISK_CREATE_TEMP)) {
		vdisk_perror(__func__);
		return vdisk_errno;
	}
	printf("vdi_compact: %s disk created\n", vdisk_str(&vdtmp));

	//
	// Block tranfer
	//
	// Status progress
	//	Original:
	//		12,803,112,960 Bytes
	//	vboxmanage --compact:
	//		11,011,096,576 Bytes
	// 1.	Initial (vdisk_write_block_at):
	//	11,917,066,240 Bytes
	// 2.	Direct write (os_write)
	//	11,914,969,088 Bytes
	// 3.	
	//	 Bytes
	// 4.	
	//	 Bytes
	// 5.	
	//	 Bytes
	// 6.	
	//	 Bytes
	// 7.	
	//	 Bytes
	// 8.	
	//	 Bytes
	// 9.	
	//	 Bytes
	//

/*

Original:
| 0 | 1 | 4 | - | 3 | <- Block index
  |   |   |   /---+
  |   |   +---|---\
  v   v       v   v
|   |   | - |   |   | <- Block data (1 MiB)

Compacted:
| 0 | 1 | 2 | - | 3 |
  |   |   |   +---+
  v   v   v   v
|   |   |   |   | - |

*/

	// "Optimized" buffer size
	size_t oblocksize = vd->vdi.blocksize / sizeof(size_t);
	// "Optimized" buffer pointer
	size_t *obuffer = (size_t*)buffer;
	uint32_t stat_unalloc = 0;	// unallocated blocks
	uint32_t stat_occupied = 0;	// allocated blocks with data
	uint32_t stat_zero = 0;	// blocks with no data inside
	uint32_t stat_alloc = 0;	// allocated blocks
	char strbsize[BIN_FLENGTH];
	fbins(vd->vdi.blocksize, strbsize);
	printf("vdi_compact: Writing (%s blocks, %u checks/%u bytes)...\n",
		strbsize, (uint32_t)oblocksize, (uint32_t)sizeof(size_t));
	uint64_t d = 0;	// disk block index
	os_seek(vd->fd, vd->offset, SEEK_SET);
	for (size_t i = 0; i < vd->u32nblocks; ++i) {
		if (vd->u32blocks[i] == VDI_BLOCK_UNALLOCATED ||
			vd->u32blocks[i] == VDI_BLOCK_FREE) {
			vdtmp.u32blocks[i] = VDI_BLOCK_UNALLOCATED;
			++stat_unalloc;
			continue;
		}
		int re = vdisk_read_block(vd, buffer, i);
		if (re == EVDREAD || re == EVDSEEK) {
			fprintf(stderr, "vdi_compact: Couldn't %s disk\n",
				re == EVDSEEK ? "seek" : "read");
			return re;
		}
		// Check if block has data, if so, write the block into tmp VDISK
		++stat_alloc;
		for (size_t b = 0; b < oblocksize; ++b) {
			if (obuffer[b])
				goto L_HASDATA;
		}
		++stat_zero;
		continue;
L_HASDATA:
		//if (vdisk_write_block_at(&vdtmp, buffer, i, d)) {
		//	fputs("vdi_compact: Couldn't write to disk\n", stderr);
		//	return EVDWRITE;
		//}
		vdtmp.u32blocks[i] = i;
		os_write(vdtmp.fd, buffer, vd->vdi.blocksize);
		++stat_occupied;
		++d;
	}
	vdtmp.vdi.blocksalloc = stat_occupied;
	vdisk_update_headers(&vdtmp);
	printf(
		"vdi_compact: %u/%u blocks written, %u unallocated, %u zero, %u total\n",
		stat_occupied, stat_alloc, stat_unalloc, stat_zero, vd->u32nblocks
	);
	return 0;
}
