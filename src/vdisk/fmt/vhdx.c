#include "vdisk/vdisk.h"
#include "utils/bin.h"
#include "utils/platform.h"
#include <assert.h>

int vdisk_vhdx_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	assert(0); //TODO: Continue VHDX

	if ((vd->meta = malloc(VHDX_META_ALLOC)) == NULL)
		return VDISK_ERROR(vd, VVD_ENOMEM);

	//TODO: Check both headers and regions before doing an error

	//
	// Headers
	//

	if (os_fread(vd->fd, &vd->vhdx->hdr, sizeof(VHDX_HDR)))
		return VDISK_ERROR(vd, VVD_EOS);
	if (vd->vhdx->hdr.magic != VHDX_MAGIC)
		return VDISK_ERROR(vd, VVD_EVDMAGIC);

	if (os_fseek(vd->fd, VHDX_HEADER1_LOC, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, &vd->vhdx->v1, sizeof(VHDX_HEADER1)))
		return VDISK_ERROR(vd, VVD_EOS);

	if (os_fseek(vd->fd, VHDX_HEADER2_LOC, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, &vd->vhdx->v1_2, sizeof(VHDX_HEADER1)))
		return VDISK_ERROR(vd, VVD_EOS);

	if (vd->vhdx->v1.magic != VHDX_HDR1_MAGIC || vd->vhdx->v1_2.magic != VHDX_HDR1_MAGIC)
		return VDISK_ERROR(vd, VVD_EVDMAGIC);
	if (vd->vhdx->v1.version != 1 || vd->vhdx->v1_2.version != 1)
		return VDISK_ERROR(vd, VVD_EVDVERSION);

	//
	// Regions
	//

	if (os_fseek(vd->fd, VHDX_REGION1_LOC, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, &vd->vhdx->reg, sizeof(VHDX_REGION_HDR)))
		return VDISK_ERROR(vd, VVD_EOS);
	if (vd->vhdx->reg.magic != VHDX_REGION_MAGIC)
		return VDISK_ERROR(vd, VVD_EVDMAGIC);

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
