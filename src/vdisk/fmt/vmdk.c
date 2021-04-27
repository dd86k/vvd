#include "vdisk/vdisk.h"
#include "utils/bin.h"
#include "utils/platform.h"

int vdisk_vmdk_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	if ((vd->vmdk = malloc(VMDK_META_ALLOC)) == NULL)
		return VDISK_ERROR(vd, VVD_ENOMEM);

	if (os_fread(vd->fd, &vd->vmdk->hdr, sizeof(VMDK_HDR)))
		return VDISK_ERROR(vd, VVD_EOS);
	if (vd->vmdk->hdr.version != 1)
		return VDISK_ERROR(vd, VVD_EVDVERSION);
	if (vd->vmdk->hdr.grainSize < 8 ||	// < 4KiB
		vd->vmdk->hdr.grainSize > 128 ||	// > 64KiB
		pow2(vd->vmdk->hdr.grainSize) == 0)
		return VDISK_ERROR(vd, VVD_EVDMISC);

	vd->capacity = SECTOR_TO_BYTE(vd->vmdk->hdr.capacity);

	vd->vmdk->in.mask = vd->vmdk->hdr.grainSize - 1;
	vd->vmdk->in.shift = fpow2((uint32_t)vd->vmdk->hdr.grainSize);
	vd->vmdk->in.overhead = SECTOR_TO_BYTE(vd->vmdk->hdr.overHead);

	vd->cb.lba_read = vdisk_vmdk_sparse_read_lba;

	return 0;
}

int vdisk_vmdk_sparse_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	
	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset

	if (offset >= vd->capacity)
		return VDISK_ERROR(vd, VVD_EVDMISC);

	//bi = offset / SECTOR_TO_BYTE(vd->vmdkold.grainSize);
	//TODO: Work with the grainSize
	offset += vd->vmdk->in.overhead;

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, buffer, 512))
		return VDISK_ERROR(vd, VVD_EOS);

	return 0;
}
