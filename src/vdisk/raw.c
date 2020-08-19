#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"

int vdisk_raw_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint64_t offset = SECTOR_TO_BYTE(index);

	if (offset >= vd->capacity)
		return vdisk_i_err(vd, VVD_EVDBOUND, LINE_BEFORE);

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	return 0;
}