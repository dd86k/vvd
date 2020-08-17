#include <string.h> // memcpy
#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"

int vdisk_vdi_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	vd->errfunc = __func__;

	if (os_fseek(vd->fd, 64, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (os_fread(vd->fd, &vd->vdihdr, sizeof(VDI_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (vd->vdihdr.magic != VDI_HEADER_MAGIC)
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	switch (vd->vdihdr.majorv) { // Use latest major version natively
	case 1: // Includes all minor releases
		if (os_fread(vd->fd, &vd->vdiv1, sizeof(VDIHEADER1)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		break;
	case 0: { // Or else, translate header
		//TODO: Don't tranlate header in case of in-file ops
		VDIHEADER0 vd0;
		if (os_fread(vd->fd, &vd0, sizeof(VDIHEADER0)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
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
		return vdisk_i_err(vd, VVD_EVDVERSION, __LINE_BEFORE__);
	}

	switch (vd->vdiv1.type) {
	case VDI_DISK_DYN:
	case VDI_DISK_FIXED: break;
	default:
		return vdisk_i_err(vd, VVD_EVDTYPE, __LINE_BEFORE__);
	}

	vd->read_lba = vdisk_vdi_read_sector;

	// allocation table
	//TODO: Consider if this is an error (or warning)
	if (vd->vdiv1.blocksize == 0)
		vd->vdiv1.blocksize = VDI_BLOCKSIZE;
	if (os_fseek(vd->fd, vd->vdiv1.offBlocks, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	int bsize = vd->vdiv1.totalblocks << 2; // * sizeof(u32)
	if ((vd->u32block = malloc(bsize)) == NULL)
		return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
	if (os_fread(vd->fd, vd->u32block, bsize))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	vd->offset = vd->vdiv1.offData;
	vd->u32blockcount = vd->vdiv1.totalblocks;
	vd->capacity = vd->vdiv1.disksize;
	vd->vdi_blockmask = vd->vdiv1.blocksize - 1;
	vd->vdi_blockshift = fpow2(vd->vdiv1.blocksize);

	return 0;
}

int vdisk_vdi_read_sector(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset
	size_t bi = offset >> vd->vdi_blockshift;

	if (bi >= vd->vdiv1.totalblocks) // out of bounds
		return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);

	uint32_t block = vd->u32block[bi];
	switch (block) {
	case VDI_BLOCK_ZERO: //TODO: Should this be zero'd too?
		return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE_BEFORE__);
	case VDI_BLOCK_FREE:
		memset(buffer, 0, 512);
		return 0;
	}

	offset = vd->offset +
		((uint64_t)block * vd->vdiv1.blocksize) +
		(offset & vd->vdi_blockmask);

#if TRACE
	printf("* %s: lba=%" PRId64 " -> offset=0x%" PRIX64 "\n", __func__, index, offset);
#endif

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	return 0;
}
