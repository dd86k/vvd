#include <string.h> // memcpy
#include "vdisk/vdisk.h"
#include "utils/bin.h"
#include "utils/platform.h"
#ifdef TRACE
#include <stdio.h>
#include <inttypes.h>
#endif

//
// vdisk_vdi_open
//

int vdisk_vdi_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	if ((vd->meta = malloc(VDI_META_ALLOC)) == NULL)
		return VDISK_ERROR(vd, VVD_ENOMEM);

	if (os_fseek(vd->fd, 0, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, &vd->vdi->hdr, sizeof(VDI_HDR)))
		return VDISK_ERROR(vd, VVD_EOS);
	if (vd->vdi->hdr.magic != VDI_HEADER_MAGIC)
		return VDISK_ERROR(vd, VVD_EOS);

	switch (vd->vdi->hdr.majorver) { // Use latest major version natively
	case 1: // v1.1
		if (os_fread(vd->fd, &vd->vdi->v1, sizeof(VDI_HEADERv1)))
			return VDISK_ERROR(vd, VVD_EOS);
		break;
	/*case 0:
		if (os_fread(vd->fd, &vd->vdi->v0, sizeof(VDI_HEADERv0)))
			return VDISK_ERROR(vd, VVD_EOS);
		break;*/
	default:
		return VDISK_ERROR(vd, VVD_EVDVERSION);
	}

	switch (vd->vdi->v1.type) {
	case VDI_DISK_DYN:
	case VDI_DISK_FIXED: break;
	default:
		return VDISK_ERROR(vd, VVD_EVDTYPE);
	}

	// Allocation table

	//TODO: Consider if this is an error (or warning)
	if (vd->vdi->v1.blk_size == 0)
		vd->vdi->v1.blk_size = VDI_BLOCKSIZE;
	if (os_fseek(vd->fd, vd->vdi->v1.offBlocks, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);

	int bsize = vd->vdi->v1.blk_total << 2; // * sizeof(u32)
	if ((vd->vdi->in.offsets = malloc(bsize)) == NULL)
		return VDISK_ERROR(vd, VVD_ENOMEM);
	if (os_fread(vd->fd, vd->vdi->in.offsets, bsize))
		return VDISK_ERROR(vd, VVD_EOS);

	// Internals / calculated values

	vd->capacity     = vd->vdi->v1.capacity;
	vd->vdi->in.mask  = vd->vdi->v1.blk_size - 1;
	vd->vdi->in.shift = fpow2(vd->vdi->v1.blk_size);

	// Function pointers

	vd->cb.lba_read = vdisk_vdi_read_sector;

	return 0;
}

int vdisk_vdi_create(VDISK *vd, uint64_t capacity, uint32_t flags) {
	if ((vd->meta = malloc(VDI_META_ALLOC)) == NULL)
		return VDISK_ERROR(vd, VVD_ENOMEM);

	if (capacity == 0)
		return VDISK_ERROR(vd, VVD_EVDMISC);

	uint32_t bcount = capacity / VDI_BLOCKSIZE;

	if ((vd->vdi->in.offsets = malloc(bcount << 2)) == NULL)
		return VDISK_ERROR(vd, VVD_ENOMEM);

	vd->format = VDISK_FORMAT_VDI;

	// Pre-header

	strcpy(vd->vdi->hdr.signature, VDI_SIGNATURE);
	vd->vdi->hdr.magic = VDI_HEADER_MAGIC;
	vd->vdi->hdr.majorver = 1;
	vd->vdi->hdr.minorver = 1;

	// Header

	vd->vdi->v1.blk_alloc = 0;
	vd->vdi->v1.blk_extra = 0;
	vd->vdi->v1.blk_size = VDI_BLOCKSIZE;
	vd->vdi->v1.cbSector = vd->vdi->v1.LegacyGeometry.cbSector = 512;
	vd->vdi->v1.cCylinders =
		vd->vdi->v1.cHeads =
		vd->vdi->v1.cSectors =
		vd->vdi->v1.LegacyGeometry.cCylinders =
		vd->vdi->v1.LegacyGeometry.cHeads =
		vd->vdi->v1.LegacyGeometry.cSectors = 0;
	vd->vdi->v1.capacity = capacity;
	vd->vdi->v1.fFlags = 0;
	vd->vdi->v1.hdrsize = (uint32_t)sizeof(VDI_HEADERv1);
	vd->vdi->v1.offBlocks = VDI_BLOCKSIZE;
	vd->vdi->v1.offData = VDI_BLOCKSIZE * 2;
	vd->vdi->v1.blk_total = 0;
	vd->vdi->v1.type = VDI_DISK_DYN;
	vd->vdi->v1.u32Dummy = 0;	// Always
	memset(vd->vdi->v1.szComment, 0, VDI_COMMENT_SIZE);
	memset(&vd->vdi->v1.uuidCreate, 0, 16);
	memset(&vd->vdi->v1.uuidLinkage, 0, 16);
	memset(&vd->vdi->v1.uuidModify, 0, 16);
	memset(&vd->vdi->v1.uuidParentModify, 0, 16);

	// Data

	uint32_t *offsets = vd->vdi->in.offsets;
	uint8_t  *buffer;
	uint32_t blk_total = vd->vdi->v1.blk_total;

	switch (flags & VDISK_CREATE_TYPE_MASK) {
	case VDISK_CREATE_TYPE_DYNAMIC:
		vd->vdi->v1.type = VDI_DISK_DYN;
		for (size_t i = 0; i < blk_total; ++i)
			offsets[i] = VDI_BLOCK_ZERO;
		break;
	case VDISK_CREATE_TYPE_FIXED:
		vd->vdi->v1.type = VDI_DISK_FIXED;
		if ((buffer = calloc(1, vd->vdi->v1.blk_size)) == NULL)
			return VDISK_ERROR(vd, VVD_ENOMEM);
		os_fseek(vd->fd, vd->vdi->v1.offData, SEEK_SET);
		for (size_t i = 0; i < blk_total; ++i) {
			offsets[i] = VDI_BLOCK_FREE;
			os_fwrite(vd->fd, buffer, vd->vdi->v1.blk_size);
		}
		break;
	default:
		return VDISK_ERROR(vd, VVD_EVDTYPE);
	}

	return 0;
}

//
// vdisk_vdi_read_sector
//

int vdisk_vdi_read_sector(VDISK *vd, void *buffer, uint64_t index) {
	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset
	size_t bi = offset >> vd->vdi->in.shift;

	if (bi >= vd->vdi->v1.blk_total) // out of bounds
		return VDISK_ERROR(vd, VVD_EVDBOUND);

	uint32_t block = vd->vdi->in.offsets[bi];
	switch (block) {
	case VDI_BLOCK_ZERO: //TODO: Should this be zero'd too?
		return VDISK_ERROR(vd, VVD_EVDUNALLOC);
	case VDI_BLOCK_FREE:
		memset(buffer, 0, 512);
		return 0;
	}

	offset = vd->vdi->v1.offData +
		((uint64_t)block * vd->vdi->v1.blk_size) +
		(offset & vd->vdi->in.mask);

#ifdef TRACE
	printf("%s: lba=%" PRId64 " -> offset=0x%" PRIX64 "\n", __func__, index, offset);
#endif

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, buffer, 512))
		return VDISK_ERROR(vd, VVD_EOS);

	return 0;
}

//
// vdisk_vdi_compact
//

int vdisk_vdi_compact(VDISK *vd) {
	if (vd->vdi->hdr.majorver != 1)
		return VDISK_ERROR(vd, VVD_EVDVERSION);
	if (vd->vdi->v1.type != VDI_DISK_DYN)
		return VDISK_ERROR(vd, VVD_EVDTYPE);
	
	return VDISK_ERROR(vd, VVD_EVDTODO);
	/*uint32_t *blks2;	// back resolving array
	uint32_t bk_alloc;	// blocks allocated (alt)

	// 1. Allocate block array for back resolving.

	uint64_t fsize;	// file size
	if (os_fsize(vd->fd, &fsize))
		return VDISK_ERROR(vd, VVD_EVDMISC);

	VDI_HEADERv1 vdi = vd->vdi->v1;
	VDI_INTERNALS in = vd->vdi->in;

	// This verifies that there are actually data blocks available
	bk_alloc = (uint32_t)((fsize - vdi.offData - vdi.offBlocks) >> in.shift);
	if (bk_alloc == 0 || vdi.blk_alloc == 0)
		return 0;

	blks2 = malloc(bk_alloc << 2);
	if (blks2 == NULL)
		return VDISK_ERROR(vd, VVD_ENOMEM);
	for (uint32_t i; i < bk_alloc; ++i)
		blks2[i] = VDI_BLOCK_FREE;

	uint32_t d = 0;
	uint32_t blk_index = 0;
	uint32_t *blks = in.offsets;

	// 2. Check and fix allocation errors before compacting

	for (; blk_index < vdi.blk_total; ++blk_index) {
		uint32_t bi = blks[blk_index]; // block index
		if (bi >= VDI_BLOCK_FREE) {
			continue;
		}

		if (bi < vdi.blk_alloc) {
			if (blks[bi] == VDI_BLOCK_FREE) {
				blks[bi] = blk_index;
			} else {
				blks[bi] = VDI_BLOCK_FREE;
				//TODO: Update header once manipulating source
				//rc = vdiUpdateBlockInfo(pImage, i);
				//vdisk_write_block_at(vd, buffer, i, d++);
			}
		} else {
			blks[bi] = VDI_BLOCK_FREE;
			//TODO: Update header once manipulating source
			//blocks[bi] = VDI_BLOCK_FREE;
			//vdisk_write_block_at(vd, buffer, i, d++);
		}
	}

	// 3. Find redundant information and update the block pointers accordingly

	

	// 4. Fill bubbles with other data if available

//	for (i = 0; o < vd->vdiold.blk_alloc

	// 5. Update fields in-memory and on-disk

	//return 0;*/
}
