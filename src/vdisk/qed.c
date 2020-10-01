#include "vdisk.h"
#include "utils.h"
#include "platform.h"
#include <assert.h>

int vdisk_qed_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	
	if (os_fread(vd->fd, &vd->qedhdr, sizeof(QED_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	if (vd->qedhdr.cluster_size < QED_CLUSTER_MIN ||
		vd->qedhdr.cluster_size > QED_CLUSTER_MAX ||
		pow2(vd->qedhdr.cluster_size) == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);
	if (vd->qedhdr.table_size < QED_TABLE_MIN ||
		vd->qedhdr.table_size > QED_TABLE_MAX ||
		pow2(vd->qedhdr.table_size) == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	if (vd->qedhdr.features > QED_FEATS)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	// assert(header.image_size <= TABLE_NOFFSETS * TABLE_NOFFSETS * header.cluster_size)

	uint32_t table_size = vd->qed_L2.tablesize =
		vd->qedhdr.cluster_size * vd->qedhdr.table_size;
	uint32_t table_entries = vd->qed_L2.entries =
		table_size / sizeof(uint64_t);
	uint32_t clusterbits = fpow2(vd->qedhdr.cluster_size);
	uint32_t tablebits = fpow2(table_entries);

	if ((vd->qed_L1 = malloc(table_size)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);
	if ((vd->qed_L2.offsets = malloc(table_size)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);
	if (os_fseek(vd->fd, vd->qedhdr.cluster_size, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, vd->qed_L1, table_size))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	// assert(clusterbits + (2 * tablebits) <= 64);

	vd->qed_offset_mask	= vd->qedhdr.cluster_size - 1;
	vd->qed_L2_mask 	= (table_entries - 1) << clusterbits;
	vd->qed_L2_shift	= clusterbits;
	vd->qed_L1_mask 	= (table_entries - 1) << (clusterbits + tablebits);
	vd->qed_L1_shift	= clusterbits + tablebits;

	vd->capacity = vd->qedhdr.capacity;
	vd->offset = vd->qedhdr.cluster_size + table_size; // header + L1
	vd->u64block = vd->qed_L1;
	vd->u32blockcount = table_entries;

	vd->cb.lba_read = vdisk_qed_read_sector;

	return 0;
}

int vdisk_qed_L2_load(VDISK *vd, uint64_t offset) {
	
	if (vd->qed_L2.offset == offset) // L2 already loaded
		return 0;

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, vd->qed_L2.offsets, vd->qed_L2.tablesize))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	vd->qed_L2.offset = offset;

	return 0;
}

int vdisk_qed_read_sector(VDISK *vd, void *buffer, uint64_t index) {
	
	uint64_t offset = SECTOR_TO_BYTE(index);

	uint32_t l1 = (offset >> vd->qed_L1_shift) & vd->qed_L1_mask;
	uint32_t l2 = (offset >> vd->qed_L2_shift) & vd->qed_L2_mask;

	if (l1 >= vd->qed_L2.entries || l2 >= vd->qed_L2.entries)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);
	if (vdisk_qed_L2_load(vd, vd->qed_L1[l1]))
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	offset = (uint64_t)vd->qed_L2.offsets[l2] + (offset & vd->qed_offset_mask);

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	return 0;
}
