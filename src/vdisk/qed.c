#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"
#include <assert.h>

int vdisk_qed_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	vd->errfunc = __func__;

	if (os_fread(vd->fd, &vd->qed, sizeof(QED_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	if (vd->qed.cluster_size < QED_CLUSTER_MIN ||
		vd->qed.cluster_size > QED_CLUSTER_MAX ||
		pow2(vd->qed.cluster_size) == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE_BEFORE__);
	if (vd->qed.table_size < QED_TABLE_MIN ||
		vd->qed.table_size > QED_TABLE_MAX ||
		pow2(vd->qed.table_size) == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE_BEFORE__);

	if (vd->qed.features > QED_FEATS)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE_BEFORE__);

	uint32_t table_size = vd->qed.cluster_size * vd->qed.table_size;
	uint32_t table_entries = table_size / sizeof(uint64_t);
	uint32_t clusterbits = fpow2(vd->qed.cluster_size);
	uint32_t tablebits = fpow2(table_entries);

	if ((vd->qed_L1 = malloc(table_size)) == NULL)
		return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
	if (os_fseek(vd->fd, vd->qed.cluster_size, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (os_fread(vd->fd, vd->qed_L1, table_size))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	// assert(clusterbits + (2 * tablebits) <= 64);

	vd->qed_offset_mask	= vd->qed.cluster_size - 1;
	vd->qed_L2_mask 	= (table_entries - 1) << clusterbits;
	vd->qed_L2_shift	= clusterbits;
	vd->qed_L1_mask 	= (table_entries - 1) << (clusterbits + tablebits);
	vd->qed_L1_shift	= clusterbits + tablebits;

	vd->capacity = vd->qed.capacity;
	vd->offset = vd->qed.cluster_size + table_size; // header + L1
	vd->u64block = vd->qed_L1;
	vd->u32blockcount = table_entries;

	return 0;
}
