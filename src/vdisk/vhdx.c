#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"
#include <assert.h>

int vdisk_vhdx_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	vd->errfunc = __func__;
	assert(0);

	//TODO: Check both headers and regions before doing an error

	//
	// Headers
	//

	if (os_fread(vd->fd, &vd->vhdx, sizeof(VHDX_HDR)))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (vd->vhdx.magic != VHDX_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, LINE_BEFORE);

	if (os_fseek(vd->fd, VHDX_HEADER1_LOC, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fread(vd->fd, &vd->vhdxhdr, sizeof(VHDX_HEADER1)))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (vd->vhdxhdr.magic != VHDX_HDR1_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, LINE_BEFORE);

	//
	// Regions
	//

	if (os_fseek(vd->fd, VHDX_REGION1_LOC, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fread(vd->fd, &vd->vhdxreg, sizeof(VHDX_REGION_HDR)))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (vd->vhdxreg.magic != VHDX_REGION_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, LINE_BEFORE);

	//
	//TODO: Log
	//

	//
	// BAT
	//
	
	// Chunk ratio
	//(8388608 * ) / // 8 KiB * 512
	
	return 0;
}
