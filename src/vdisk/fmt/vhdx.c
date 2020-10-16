#include "vdisk/vdisk.h"
#include "utils/bin.h"
#include "utils/platform.h"
#include <assert.h>

int vdisk_vhdx_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	assert(0); //TODO: Continue VHDX

	if ((vd->meta = malloc(VHDX_META_ALLOC)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);

	//TODO: Check both headers and regions before doing an error

	//
	// Headers
	//

	if (os_fread(vd->fd, &vd->vhdx->hdr, sizeof(VHDX_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (vd->vhdx->hdr.magic != VHDX_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE__, __func__);

	if (os_fseek(vd->fd, VHDX_HEADER1_LOC, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, &vd->vhdx->v1, sizeof(VHDX_HEADER1)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	if (os_fseek(vd->fd, VHDX_HEADER2_LOC, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, &vd->vhdx->v1_2, sizeof(VHDX_HEADER1)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	if (vd->vhdx->v1.magic != VHDX_HDR1_MAGIC || vd->vhdx->v1_2.magic != VHDX_HDR1_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE__, __func__);
	if (vd->vhdx->v1.version != 1 || vd->vhdx->v1_2.version != 1)
		return vdisk_i_err(vd, VVD_EVDVERSION, __LINE__, __func__);

	//
	// Regions
	//

	if (os_fseek(vd->fd, VHDX_REGION1_LOC, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, &vd->vhdx->reg, sizeof(VHDX_REGION_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (vd->vhdx->reg.magic != VHDX_REGION_MAGIC)
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
