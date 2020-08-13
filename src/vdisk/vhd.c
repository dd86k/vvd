#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"

int vdisk_vhd_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	vd->errfunc = __func__;

	if (internal & 2)
		goto L_VHD_MAGICOK;

	if (os_fread(vd->fd, &vd->vhd, sizeof(VHD_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (vd->vhd.magic != VHD_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE_BEFORE__);

L_VHD_MAGICOK:
#if ENDIAN_LITTLE
	vd->vhd.major = bswap16(vd->vhd.major);
#endif
	if (vd->vhd.major != 1)
		return vdisk_i_err(vd, VVD_EVDVERSION, __LINE_BEFORE__);
		
#if ENDIAN_LITTLE
	vd->vhd.type = bswap32(vd->vhd.type);
#endif
	switch (vd->vhd.type) {
	case VHD_DISK_DIFF:
	case VHD_DISK_DYN:
	case VHD_DISK_FIXED: break;
	default:
		return vdisk_i_err(vd, VVD_EVDTYPE, __LINE_BEFORE__);
	}

#if ENDIAN_LITTLE
	vd->vhd.features = bswap32(vd->vhd.features);
	vd->vhd.minor = bswap16(vd->vhd.minor);
	vd->vhd.offset = bswap64(vd->vhd.offset);
	vd->vhd.timestamp = bswap32(vd->vhd.timestamp);
	vd->vhd.creator_major = bswap16(vd->vhd.creator_major);
	vd->vhd.creator_minor = bswap16(vd->vhd.creator_minor);
//	vd->vhd.creator_os = bswap32(vd->vhd.creator_os);
	vd->vhd.size_original = bswap64(vd->vhd.size_original);
	vd->vhd.size_current = bswap64(vd->vhd.size_current);
	vd->vhd.cylinders = bswap16(vd->vhd.cylinders);
	vd->vhd.checksum = bswap32(vd->vhd.checksum);
	uid_swap(&vd->vhd.uuid);
#endif

	if (vd->vhd.type != VHD_DISK_FIXED) {
		if (os_fseek(vd->fd, vd->vhd.offset, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (os_fread(vd->fd, &vd->vhddyn, sizeof(VHD_DYN_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (vd->vhddyn.magic != VHD_DYN_MAGIC)
			return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE_BEFORE__);

#if ENDIAN_LITTLE
		vd->vhddyn.data_offset = bswap64(vd->vhddyn.data_offset);
		vd->vhddyn.table_offset = bswap64(vd->vhddyn.table_offset);
		vd->vhddyn.minor = bswap16(vd->vhddyn.minor);
		vd->vhddyn.major = bswap16(vd->vhddyn.major);
		vd->vhddyn.max_entries = bswap32(vd->vhddyn.max_entries);
		vd->vhddyn.blocksize = bswap32(vd->vhddyn.blocksize);
		vd->vhddyn.checksum = bswap32(vd->vhddyn.checksum);
		vd->vhddyn.parent_timestamp = bswap32(vd->vhddyn.parent_timestamp);
		uid_swap(&vd->vhddyn.parent_uuid);

		for (size_t i = 0; i < 8; ++i) {
			vd->vhddyn.parent_locator[i].code = bswap32(
				vd->vhddyn.parent_locator[i].code);
			vd->vhddyn.parent_locator[i].datasize = bswap32(
				vd->vhddyn.parent_locator[i].datasize);
			vd->vhddyn.parent_locator[i].dataspace = bswap32(
				vd->vhddyn.parent_locator[i].dataspace);
			vd->vhddyn.parent_locator[i].offset = bswap64(
				vd->vhddyn.parent_locator[i].offset);
		}
#endif

		vd->u32blockcount = vd->vhd.size_original / vd->vhddyn.blocksize;
//		vd->blockmask = vd->vhddyn.blocksize - 1;
//		vd->blockshift = fpow2(vd->vhddyn.blocksize);

		if (vd->u32blockcount <= 0)
			return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE_BEFORE__);
		if (os_fseek(vd->fd, vd->vhddyn.table_offset, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

		int batsize = vd->u32blockcount << 2; // "* sizeof(u32)"
		if ((vd->u32block = malloc(batsize)) == NULL)
			return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
		if (os_fread(vd->fd, vd->u32block, batsize))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
#if ENDIAN_LITTLE
		for (size_t i = 0; i < vd->u32blockcount; ++i)
			vd->u32block[i] = bswap32(vd->u32block[i]);
#endif
		vd->offset = SECTOR_TO_BYTE(vd->u32block[0]) + 512;
	} else {
		vd->offset = 0;
	}

	vd->capacity = vd->vhd.size_original;
	return 0;
}
