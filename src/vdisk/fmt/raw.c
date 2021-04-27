#include "vdisk/vdisk.h"
#include "utils/bin.h"
#include "utils/platform.h"

int vdisk_raw_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	if (os_fsize(vd->fd, &vd->capacity))
		return VDISK_ERROR(vd, VVD_EOS);
	vd->format = VDISK_FORMAT_RAW;
	vd->cb.lba_read = vdisk_raw_read_lba;
	return 0;
}

int vdisk_raw_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	uint64_t offset = SECTOR_TO_BYTE(index);

	if (offset >= vd->capacity)
		return VDISK_ERROR(vd, VVD_EVDBOUND);

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, buffer, 512))
		return VDISK_ERROR(vd, VVD_EOS);

	return 0;
}