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
		if (os_fread(vd->fd, &vd->vdi, sizeof(VDIHEADER1)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		break;
	case 0: { // Or else, translate header
		VDIHEADER0 vd0;
		if (os_fread(vd->fd, &vd0, sizeof(VDIHEADER0)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		vd->vdi.disksize = vd0.disksize;
		vd->vdi.type = vd0.type;
		vd->vdi.offBlocks = sizeof(VDI_HDR) + sizeof(VDIHEADER0);
		vd->vdi.offData = sizeof(VDI_HDR) + sizeof(VDIHEADER0) +
			(vd0.totalblocks << 2); // sizeof(uint32_t) -> "* 4" -> "<< 2"
		memcpy(&vd->vdi.uuidCreate, &vd0.uuidCreate, 16);
		memcpy(&vd->vdi.uuidModify, &vd0.uuidModify, 16);
		memcpy(&vd->vdi.uuidLinkage, &vd0.uuidLinkage, 16);
		memcpy(&vd->vdi.LegacyGeometry, &vd0.LegacyGeometry, sizeof(VDIDISKGEOMETRY));
		break;
	}
	default:
		return vdisk_i_err(vd, VVD_EVDVERSION, __LINE_BEFORE__);
	}

	switch (vd->vdi.type) {
	case VDI_DISK_DYN:
	case VDI_DISK_FIXED: break;
	default:
		return vdisk_i_err(vd, VVD_EVDTYPE, __LINE_BEFORE__);
	}

	// allocation table
	//TODO: Consider if this is an error (or warning)
	if (vd->vdi.blocksize == 0)
		vd->vdi.blocksize = VDI_BLOCKSIZE;
	if (os_fseek(vd->fd, vd->vdi.offBlocks, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	int bsize = vd->vdi.totalblocks << 2; // * sizeof(u32)
	if ((vd->u32block = malloc(bsize)) == NULL)
		return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
	if (os_fread(vd->fd, vd->u32block, bsize))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	vd->offset = vd->vdi.offData;
	vd->u32blockcount = vd->vdi.totalblocks;
	vd->capacity = vd->vdi.disksize;
	vd->vdi_blockmask = vd->vdi.blocksize - 1;
	vd->vdi_blockshift = fpow2(vd->vdi.blocksize);

	return 0;
}
