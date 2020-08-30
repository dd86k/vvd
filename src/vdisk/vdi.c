#include <string.h> // memcpy
#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"
#ifdef TRACE
#include <stdio.h>
#include <inttypes.h>
#endif

//
// vdisk_vdi_open
//

int vdisk_vdi_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	vd->errfunc = __func__;

	if (os_fseek(vd->fd, 64, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fread(vd->fd, &vd->vdihdr, sizeof(VDI_HDR)))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (vd->vdihdr.magic != VDI_HEADER_MAGIC)
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	switch (vd->vdihdr.majorv) { // Use latest major version natively
	case 1: // Includes all minor releases
		if (os_fread(vd->fd, &vd->vdiv1, sizeof(VDIHEADER1)))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		break;
	case 0: { // Or else, translate header
		//TODO: Don't tranlate header in case of in-file ops
		VDIHEADER0 vd0;
		if (os_fread(vd->fd, &vd0, sizeof(VDIHEADER0)))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		vd->vdiv1.disksize = vd0.disksize;
		vd->vdiv1.type = vd0.type;
		vd->vdiv1.offBlocks = sizeof(VDI_HDR) + sizeof(VDIHEADER0);
		vd->vdiv1.offData = sizeof(VDI_HDR) + sizeof(VDIHEADER0) +
			(vd0.totalblocks << 2); // sizeof(uint32_t) -> "* 4" -> "<< 2"
		memcpy(&vd->vdiv1.uuidCreate, &vd0.uuidCreate, 16);
		memcpy(&vd->vdiv1.uuidModify, &vd0.uuidModify, 16);
		memcpy(&vd->vdiv1.uuidLinkage, &vd0.uuidLinkage, 16);
		memcpy(&vd->vdiv1.LegacyGeometry, &vd0.LegacyGeometry, sizeof(VDIDISKGEOMETRY));
		break;
	}
	default:
		return vdisk_i_err(vd, VVD_EVDVERSION, LINE_BEFORE);
	}

	switch (vd->vdiv1.type) {
	case VDI_DISK_DYN:
	case VDI_DISK_FIXED: break;
	default:
		return vdisk_i_err(vd, VVD_EVDTYPE, LINE_BEFORE);
	}

	vd->read_lba = vdisk_vdi_read_sector;

	// allocation table
	//TODO: Consider if this is an error (or warning)
	if (vd->vdiv1.blocksize == 0)
		vd->vdiv1.blocksize = VDI_BLOCKSIZE;
	if (os_fseek(vd->fd, vd->vdiv1.offBlocks, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	int bsize = vd->vdiv1.totalblocks << 2; // * sizeof(u32)
	if ((vd->u32block = malloc(bsize)) == NULL)
		return vdisk_i_err(vd, VVD_EALLOC, LINE_BEFORE);
	if (os_fread(vd->fd, vd->u32block, bsize))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	vd->offset = vd->vdiv1.offData;
	vd->u32blockcount = vd->vdiv1.totalblocks;
	vd->capacity = vd->vdiv1.disksize;
	vd->vdi_blockmask = vd->vdiv1.blocksize - 1;
	vd->vdi_blockshift = fpow2(vd->vdiv1.blocksize);

	return 0;
}

//
// vdisk_vdi_read_sector
//

int vdisk_vdi_read_sector(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset
	size_t bi = offset >> vd->vdi_blockshift;

	if (bi >= vd->vdiv1.totalblocks) // out of bounds
		return vdisk_i_err(vd, VVD_EVDBOUND, LINE_BEFORE);

	uint32_t block = vd->u32block[bi];
	switch (block) {
	case VDI_BLOCK_ZERO: //TODO: Should this be zero'd too?
		return vdisk_i_err(vd, VVD_EVDUNALLOC, LINE_BEFORE);
	case VDI_BLOCK_FREE:
		memset(buffer, 0, 512);
		return 0;
	}

	offset = vd->offset +
		((uint64_t)block * vd->vdiv1.blocksize) +
		(offset & vd->vdi_blockmask);

#ifdef TRACE
	printf("%s: lba=%" PRId64 " -> offset=0x%" PRIX64 "\n", __func__, index, offset);
#endif

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	return 0;
}

//
// vdisk_vdi_compact
//

int vdisk_vdi_compact(VDISK *vd, void(*cb)(uint32_t type, void *data)) {
	if (vd->vdiv1.type != VDI_DISK_DYN)
		return vdisk_i_err(vd, VVD_EVDTYPE, LINE_BEFORE);

	if (vd->vdiv1.blocksalloc == 0)
		return 0;

	VDISK vdtmp;
	if (vdisk_create(&vdtmp, osstr("vdisk.tmp"), VDISK_FORMAT_VDI, vd->capacity, VDISK_CREATE_DYN))
		return vdtmp.errcode;

	if (cb) {
		cb(VVD_NOTIF_VDISK_CREATED_TYPE_NAME, (char*)vdisk_str(&vdtmp));
		cb(VVD_NOTIF_VDISK_TOTAL_BLOCKS, &vdtmp.vdiv1.totalblocks);
	}

	uint32_t stat_unalloc = 0;	// unallocated blocks
	uint32_t stat_occupied = 0;	// allocated blocks with data
	uint32_t stat_zero = 0;	// blocks with no data inside
	uint32_t stat_alloc = 0;	// allocated blocks

/*	char strbsize[BINSTR_LENGTH];
	bintostr(strbsize, vd->vdiv1.blocksize);
	printf("vvd_compact: writing (%s/block, %u checks/%u bytes)...\n",
		strbsize, (uint32_t)oblocksize, (uint32_t)sizeof(size_t));*/

	uint32_t d = 0;
	uint32_t i = 0;
	// check/fix allocation errors before compating
/*	for (; i < vd->u32blockcount; ++i) {
		uint32_t bi = vd->u32block[i]; // block index
		if (bi >= VDI_BLOCK_FREE) {
			++stat_unalloc;
			continue;
		}

		if (cb) cb(VVD_NOTIF_VDISK_CURRENT_BLOCK, &i);

		if (bi < vd->vdiv1.blocksalloc) {
			if (vdtmp.u32block[bi] == VDI_BLOCK_FREE) {
				vdtmp.u32block[bi] = i;
			} else {
				vd->u32block[bi] = VDI_BLOCK_FREE;
				//TODO: Update header once manipulating source
				//rc = vdiUpdateBlockInfo(pImage, i);
				vdisk_write_block_at(&vdtmp, buffer, i, d++);
			}
		} else {
			vd->u32block[bi] = VDI_BLOCK_FREE;
			//TODO: Update header once manipulating source
			//vd->u32block[bi] = VDI_BLOCK_FREE;
			vdisk_write_block_at(&vdtmp, buffer, i, d++);
		}
	}*/
	// Find redundant information and update the block pointers accordingly
	for (i = 0; i < vd->u32blockcount; ++i) {
		uint32_t bi = vd->u32block[i]; // block index
		if (bi >= VDI_BLOCK_FREE) {
			++stat_unalloc;
			continue;
		}
	}
	// Fill bubbles with other data if available
//		for (i = 0; o < vd->vdi.blocksalloc

	return 0;
}
