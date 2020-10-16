#include "vdisk/vdisk.h"
#include "utils/bin.h"
#include "utils/platform.h"
#include <assert.h>

int vdisk_qed_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	if ((vd->meta = malloc(QED_META_ALLOC)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);

	if (os_fread(vd->fd, &vd->qed->hdr, sizeof(QED_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	if (vd->qed->hdr.cluster_size < QED_CLUSTER_MIN ||
		vd->qed->hdr.cluster_size > QED_CLUSTER_MAX ||
		pow2(vd->qed->hdr.cluster_size) == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);
	if (vd->qed->hdr.table_size < QED_TABLE_MIN ||
		vd->qed->hdr.table_size > QED_TABLE_MAX ||
		pow2(vd->qed->hdr.table_size) == 0)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	if (vd->qed->hdr.features > QED_FEATS)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

	// assert(header.image_size <= TABLE_NOFFSETS * TABLE_NOFFSETS * header.cluster_size)

	uint32_t table_size = vd->qed->in.tablesize =
		vd->qed->hdr.cluster_size * vd->qed->hdr.table_size;
	uint32_t table_entries = vd->qed->in.entries = table_size / sizeof(uint64_t);
	uint32_t clusterbits = fpow2(vd->qed->hdr.cluster_size);
	uint32_t tablebits = fpow2(table_entries);

	if ((vd->qed->in.L1.offsets = malloc(table_size)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);
	if ((vd->qed->in.L2.offsets = malloc(table_size)) == NULL)
		return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);
	if (os_fseek(vd->fd, vd->qed->hdr.l1_offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, vd->qed->in.L1.offsets, table_size))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	// assert(clusterbits + (2 * tablebits) <= 64);

	vd->qed->in.mask	= vd->qed->hdr.cluster_size - 1;
	vd->qed->in.L2.mask 	= (table_entries - 1) << clusterbits;
	vd->qed->in.L2.shift	= clusterbits;
	vd->qed->in.L1.mask 	= (table_entries - 1) << (clusterbits + tablebits);
	vd->qed->in.L1.mask	= clusterbits + tablebits;

	vd->capacity = vd->qed->hdr.capacity;

	vd->cb.lba_read = vdisk_qed_read_sector;

	return 0;
}

int vdisk_qed_L2_load(VDISK *vd, uint64_t offset) {
	if (vd->qed->in.L2.current == offset) // L2 already loaded
		return 0;

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, vd->qed->in.L2.offsets, vd->qed->in.tablesize))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	vd->qed->in.L2.current = offset;

	return 0;
}

int vdisk_qed_read_sector(VDISK *vd, void *buffer, uint64_t index) {
	uint64_t offset = SECTOR_TO_BYTE(index);

	uint32_t l1 = (offset >> vd->qed->in.L1.shift) & vd->qed->in.L1.mask;
	uint32_t l2 = (offset >> vd->qed->in.L2.shift) & vd->qed->in.L2.mask;

	if (l1 >= vd->qed->in.entries || l2 >= vd->qed->in.entries)
		return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);
	if (vdisk_qed_L2_load(vd, vd->qed->in.L1.offsets[l1]))
		return vd->err.num;

	offset = (uint64_t)vd->qed->in.L2.offsets[l2] + (offset & vd->qed->in.mask);

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	return 0;
}
