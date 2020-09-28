#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"

int vdisk_vmdk_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	
	if (os_fread(vd->fd, &vd->vmdkhdr, sizeof(VMDK_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (vd->vmdkhdr.version != 1)
		return vdisk_i_err(vd, VVD_EVDVERSION, __LINE__, __func__);
	if (vd->vmdkhdr.grainSize < 1 || vd->vmdkhdr.grainSize > 128 || pow2(vd->vmdkhdr.grainSize) == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	vd->offset = SECTOR_TO_BYTE(vd->vmdkhdr.overHead);
	vd->capacity = SECTOR_TO_BYTE(vd->vmdkhdr.capacity);

	vd->vmdk_blockmask = vd->vmdkhdr.grainSize - 1;
	vd->vmdk_blockshift = fpow2((uint32_t)vd->vmdkhdr.grainSize);

	vd->cb.lba_read = vdisk_vmdk_sparse_read_lba;

	return 0;
}

int vdisk_vmdk_sparse_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	
	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset

	if (offset >= vd->vmdkhdr.capacity)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	//bi = offset / SECTOR_TO_BYTE(vd->vmdk.grainSize);
	//TODO: Work with the grainSize
	offset += vd->offset;

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	return 0;
}
