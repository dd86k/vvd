#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"
#include <assert.h>

int vdisk_vhdx_open(VDISK *vd, uint32_t flags, uint32_t internal) {
		assert(0);

	//TODO: Check both headers and regions before doing an error

	//
	// Headers
	//

	if (os_fread(vd->fd, &vd->vhdxhdr, sizeof(VHDX_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (vd->vhdxhdr.magic != VHDX_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE__, __func__);

	if (os_fseek(vd->fd, VHDX_HEADER1_LOC, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, &vd->vhdxhdr, sizeof(VHDX_HEADER1)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (vd->vhdxhdr.magic != VHDX_HDR1_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE__, __func__);

	//
	// Regions
	//

	if (os_fseek(vd->fd, VHDX_REGION1_LOC, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, &vd->vhdxreg, sizeof(VHDX_REGION_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (vd->vhdxreg.magic != VHDX_REGION_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE__, __func__);

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
