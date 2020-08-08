#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"

int vdisk_vmdk_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	vd->errfunc = __func__;

	if (os_fread(vd->fd, &vd->vmdk, sizeof(VMDK_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (vd->vmdk.version != 1)
		return vdisk_i_err(vd, VVD_EVDVERSION, __LINE_BEFORE__);
	if (vd->vmdk.grainSize < 1 || vd->vmdk.grainSize > 128 || pow2(vd->vmdk.grainSize) == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE_BEFORE__);
	vd->offset = SECTOR_TO_BYTE(vd->vmdk.overHead);
	vd->capacity = SECTOR_TO_BYTE(vd->vmdk.capacity);
	return 0;
}
