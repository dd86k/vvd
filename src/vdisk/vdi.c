#include <string.h> // memcpy
#include "vdisk.h"
#include "utils.h"
#include "platform.h"
#ifdef TRACE
#include <stdio.h>
#include <inttypes.h>
#endif

//
// vdisk_vdi_open
//

int vdisk_vdi_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	
	if ((vd->buffer_headers = malloc(VDI_HDR_ALLOC)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);

	vd->vdi.hdr = vd->buffer_headers;

	if (os_fseek(vd->fd, 0, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, vd->vdi.hdr, sizeof(VDI_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (vd->vdi.hdr->magic != VDI_HEADER_MAGIC)
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	switch (vd->vdi.hdr->majorver) { // Use latest major version natively
	case 1: // Includes v1.1
		vd->vdi.v1 = (void*)vd->vdi.hdr + sizeof(VDI_HDR);
		if (os_fread(vd->fd, vd->vdi.v1, sizeof(VDI_HEADERv1)))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		vd->vdi.v0 = NULL;
		vd->vdi.in = (void*)vd->vdi.v1 + sizeof(VDI_HEADERv1);
		break;
	/*case 0:
		vd->vdi.v0 = vd->vdi.hdr + sizeof(VDI_HDR);
		if (os_fread(vd->fd, vd->vdi.v0, sizeof(VDI_HEADERv0)))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		vd->vdi.v1 = NULL;
		vd->vdi.in = vd->vdi.v0 + sizeof(VDI_HEADERv1);
		break;*/
	default:
		return vdisk_i_err(vd, VVD_EVDVERSION, __LINE__, __func__);
	}

	switch (vd->vdi.v1->type) {
	case VDI_DISK_DYN:
	case VDI_DISK_FIXED: break;
	default:
		return vdisk_i_err(vd, VVD_EVDTYPE, __LINE__, __func__);
	}

	// Allocation table

	//TODO: Consider if this is an error (or warning)
	if (vd->vdi.v1->blk_size == 0)
		vd->vdi.v1->blk_size = VDI_BLOCKSIZE;
	if (os_fseek(vd->fd, vd->vdi.v1->offBlocks, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	int bsize = vd->vdi.v1->blk_total << 2; // * sizeof(u32)
	if ((vd->vdi.in->offsets = malloc(bsize)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);
	if (os_fread(vd->fd, vd->vdi.in->offsets, bsize))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	// Internals / calculated values

	vd->offset = vd->vdi.v1->offData;
	vd->capacity = vd->vdi.v1->capacity;
	vd->vdi.in->bmask = vd->vdi.v1->blk_size - 1;
	vd->vdi.in->bshift = fpow2(vd->vdi.v1->blk_size);

	// Function pointers

	vd->cb.lba_read = vdisk_vdi_read_sector;

	return 0;
}

int vdisk_vdi_create(VDISK *vd, uint64_t capacity, uint32_t flags) {

	if ((vd->buffer_headers = malloc(VDI_HDR_ALLOC)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);

	if (capacity == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	vd->vdi.hdr = vd->buffer_headers;
	vd->vdi.v1 = (void*)vd->vdi.hdr + sizeof(VDI_HDR);
	vd->vdi.in = (void*)vd->vdi.v1 + sizeof(VDI_HEADERv1);

	uint32_t bcount = capacity / VDI_BLOCKSIZE;

	if ((vd->vdi.in->offsets = malloc(bcount << 2)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);

	vd->format = VDISK_FORMAT_VDI;

	// Pre-header

	strcpy(vd->vdi.hdr->signature, VDI_SIGNATURE);
	vd->vdi.hdr->magic = VDI_HEADER_MAGIC;
	vd->vdi.hdr->majorver = 1;
	vd->vdi.hdr->minorver = 1;

	// Header

	vd->vdi.v1->blk_alloc = 0;
	vd->vdi.v1->blk_extra = 0;
	vd->vdi.v1->blk_size = VDI_BLOCKSIZE;
	vd->vdi.v1->cbSector = vd->vdi.v1->LegacyGeometry.cbSector = 512;
	vd->vdi.v1->cCylinders =
		vd->vdi.v1->cHeads =
		vd->vdi.v1->cSectors =
		vd->vdi.v1->LegacyGeometry.cCylinders =
		vd->vdi.v1->LegacyGeometry.cHeads =
		vd->vdi.v1->LegacyGeometry.cSectors = 0;
	vd->vdi.v1->capacity = capacity;
	vd->vdi.v1->fFlags = 0;
	vd->vdi.v1->hdrsize = (uint32_t)sizeof(VDI_HEADERv1);
	vd->vdi.v1->offBlocks = VDI_BLOCKSIZE;
	vd->vdi.v1->offData = VDI_BLOCKSIZE * 2;
	vd->vdi.v1->blk_total = 0;
	vd->vdi.v1->type = VDI_DISK_DYN;
	vd->vdi.v1->u32Dummy = 0;	// Always
	memset(vd->vdi.v1->szComment, 0, VDI_COMMENT_SIZE);
	memset(&vd->vdi.v1->uuidCreate, 0, 16);
	memset(&vd->vdi.v1->uuidLinkage, 0, 16);
	memset(&vd->vdi.v1->uuidModify, 0, 16);
	memset(&vd->vdi.v1->uuidParentModify, 0, 16);

	// Data

	uint32_t *offsets = vd->vdi.in->offsets;
	uint8_t  *buffer;

	switch (flags & VDISK_CREATE_TYPE_MASK) {
	case VDISK_CREATE_TYPE_DYNAMIC:
		vd->vdi.v1->type = VDI_DISK_DYN;
		for (size_t i = 0; i < vd->vdi.v1->blk_total; ++i)
			offsets[i] = VDI_BLOCK_ZERO;
		break;
	case VDISK_CREATE_TYPE_FIXED:
		vd->vdi.v1->type = VDI_DISK_FIXED;
		if ((buffer = malloc(vd->vdi.v1->blk_size)) == NULL)
			return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);
		os_fseek(vd->fd, vd->vdi.v1->offData, SEEK_SET);
		for (size_t i = 0; i < vd->vdi.v1->blk_total; ++i) {
			offsets[i] = VDI_BLOCK_FREE;
			os_fwrite(vd->fd, buffer, vd->vdi.v1->blk_size);
		}
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDTYPE, __LINE__, __func__);
	}

	return 0;
}

//
// vdisk_vdi_read_sector
//

int vdisk_vdi_read_sector(VDISK *vd, void *buffer, uint64_t index) {
	
	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset
	size_t bi = offset >> vd->vdi.in->bshift;

	if (bi >= vd->vdi.v1->blk_total) // out of bounds
		return vdisk_i_err(vd, VVD_EVDBOUND, __LINE__, __func__);

	uint32_t block = vd->vdi.in->offsets[bi];
	switch (block) {
	case VDI_BLOCK_ZERO: //TODO: Should this be zero'd too?
		return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE__, __func__);
	case VDI_BLOCK_FREE:
		memset(buffer, 0, 512);
		return 0;
	}

	offset = vd->offset +
		((uint64_t)block * vd->vdi.v1->blk_size) +
		(offset & vd->vdi.in->bmask);

#ifdef TRACE
	printf("%s: lba=%" PRId64 " -> offset=0x%" PRIX64 "\n", __func__, index, offset);
#endif

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	return 0;
}

//
// vdisk_vdi_compact
//

int vdisk_vdi_compact(VDISK *vd, void(*cb)(uint32_t type, void *data)) {
	
	if (vd->vdi.v1->type != VDI_DISK_DYN)
		return vdisk_i_err(vd, VVD_EVDTYPE, __LINE__, __func__);

	uint32_t *blks2;	// back resolving array
	uint32_t bk_alloc;	// blocks allocated (alt)

	// 1. Allocate block array for back resolving.

	uint64_t fsize;
	if (os_fsize(vd->fd, &fsize))
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	// This verifies that there are actually data blocks available
	bk_alloc = (uint32_t)((fsize - vd->vdi.v1->offData - vd->vdi.v1->offBlocks) >> vd->vdi.in->bshift);
	if (bk_alloc == 0 || vd->vdi.v1->blk_alloc == 0)
		return 0;

	blks2 = malloc(bk_alloc << 2);
	if (blks2 == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);
	for (uint32_t i; i < bk_alloc; ++i)
		blks2[i] = VDI_BLOCK_FREE;

	uint32_t d = 0;
	uint32_t blk_index = 0;
	uint32_t *blks = vd->vdi.in->offsets;

	// 2. Check and fix allocation errors before compacting

	for (; blk_index < vd->vdi.v1->blk_total; ++blk_index) {
		uint32_t bi = blks[blk_index]; // block index
		if (bi >= VDI_BLOCK_FREE) {
			continue;
		}

		if (bi < vd->vdi.v1->blk_alloc) {
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

	uint32_t blk_count = vd->vdi.v1->blk_total;

	for (blk_index = 0; blk_index < blk_count; ++blk_index) {
		uint32_t bi = blks[blk_index]; // block index
		if (bi >= VDI_BLOCK_FREE) {
			continue;
		}
	}

	// 4. Fill bubbles with other data if available

//	for (i = 0; o < vd->vdi.blk_alloc

	// 5. Update fields in-memory and on-disk

	return 0;
}
