#include "../vdisk.h"
#include "../utils.h"
#include "../platform.h"
#include "../utils.h"
#ifdef TRACE
#include <stdio.h>
#include <inttypes.h>
#endif

//
// vdisk_vhd_open
//

int vdisk_vhd_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	vd->errfunc = __func__;

	if (internal & 2)
		goto L_VHD_MAGICOK;

	if (os_fread(vd->fd, &vd->vhd, sizeof(VHD_HDR)))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (vd->vhd.magic != VHD_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, LINE_BEFORE);

L_VHD_MAGICOK:
#if ENDIAN_LITTLE
	vd->vhd.major = bswap16(vd->vhd.major);
#endif
	if (vd->vhd.major != 1)
		return vdisk_i_err(vd, VVD_EVDVERSION, LINE_BEFORE);
		
#if ENDIAN_LITTLE
	vd->vhd.type = bswap32(vd->vhd.type);
#endif
	switch (vd->vhd.type) {
	case VHD_DISK_DIFF:
	case VHD_DISK_DYN:
	case VHD_DISK_FIXED: break;
	default:
		return vdisk_i_err(vd, VVD_EVDTYPE, LINE_BEFORE);
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
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		if (os_fread(vd->fd, &vd->vhddyn, sizeof(VHD_DYN_HDR)))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		if (vd->vhddyn.magic != VHD_DYN_MAGIC)
			return vdisk_i_err(vd, VVD_EVDMAGIC, LINE_BEFORE);

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

		if (pow2(vd->vhddyn.blocksize) == 0 ||
			vd->vhddyn.max_entries != vd->vhd.size_original / vd->vhddyn.blocksize)
			return vdisk_i_err(vd, VVD_EVDMISC, LINE_BEFORE);

		vd->u32blockcount = vd->vhddyn.max_entries;
		vd->vhd_blockmask = vd->vhddyn.blocksize - 1;
		vd->vhd_blockshift = fpow2(vd->vhddyn.blocksize);

		if (vd->u32blockcount == 0)
			return vdisk_i_err(vd, VVD_EVDMAGIC, LINE_BEFORE);
		if (os_fseek(vd->fd, vd->vhddyn.table_offset, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

		int batsize = vd->u32blockcount << 2; // "* 4"
		if ((vd->u32block = malloc(batsize)) == NULL)
			return vdisk_i_err(vd, VVD_EALLOC, LINE_BEFORE);
		if (os_fread(vd->fd, vd->u32block, batsize))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
#if ENDIAN_LITTLE
		for (size_t i = 0; i < vd->u32blockcount; ++i)
			vd->u32block[i] = bswap32(vd->u32block[i]);
#endif
		vd->read_lba = vdisk_vhd_dyn_read_lba;
	} else { // Fixed
		vd->read_lba = vdisk_vhd_fixed_read_lba;
	}
	vd->offset = 0;

	vd->capacity = vd->vhd.size_original;
	return 0;
}

//
// vdisk_vhd_fixed_read_lba
//

int vdisk_vhd_fixed_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	return 0;
}

//
// vdisk_vhd_dyn_read_lba
//

int vdisk_vhd_dyn_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint64_t offset = SECTOR_TO_BYTE(index);
	uint32_t bi = (uint32_t)(offset >> vd->vhd_blockshift);
#ifdef TRACE
	printf("%s: bi=%u\n", __func__, bi);
#endif
	if (bi >= vd->u32blockcount)
		return vdisk_i_err(vd, VVD_EVDBOUND, LINE_BEFORE);

	uint32_t block = vd->u32block[bi];
	if (block == VHD_BLOCK_UNALLOC) // Unallocated
		return vdisk_i_err(vd, VVD_EVDUNALLOC, LINE_BEFORE);

	uint64_t base = SECTOR_TO_BYTE(block) + 512;
	offset = base + (offset & vd->vhd_blockmask);
#ifdef TRACE
	printf("%s: block=%u  offset=%" PRIu64 "\n", __func__, block, offset);
#endif
	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	return 0;
}
