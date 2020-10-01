#include "vdisk.h"
#include "utils.h"
#include "platform.h"
#include "utils.h"
#ifdef TRACE
#include <stdio.h>
#include <inttypes.h>
#endif

//
// vdisk_vhd_open
//

int vdisk_vhd_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	
	if (internal & 2)
		goto L_VHD_MAGICOK;

	if (os_fread(vd->fd, &vd->vhdhdr, sizeof(VHD_HDR)))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (vd->vhdhdr.magic != VHD_MAGIC)
		return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE__, __func__);

L_VHD_MAGICOK:
#if ENDIAN_LITTLE
	vd->vhdhdr.major = bswap16(vd->vhdhdr.major);
#endif
	if (vd->vhdhdr.major != 1)
		return vdisk_i_err(vd, VVD_EVDVERSION, __LINE__, __func__);
		
#if ENDIAN_LITTLE
	vd->vhdhdr.type = bswap32(vd->vhdhdr.type);
#endif
	switch (vd->vhdhdr.type) {
	case VHD_DISK_DIFF:
	case VHD_DISK_DYN:
	case VHD_DISK_FIXED: break;
	default:
		return vdisk_i_err(vd, VVD_EVDTYPE, __LINE__, __func__);
	}

#if ENDIAN_LITTLE
	vd->vhdhdr.features = bswap32(vd->vhdhdr.features);
	vd->vhdhdr.minor = bswap16(vd->vhdhdr.minor);
	vd->vhdhdr.offset = bswap64(vd->vhdhdr.offset);
	vd->vhdhdr.timestamp = bswap32(vd->vhdhdr.timestamp);
	vd->vhdhdr.creator_major = bswap16(vd->vhdhdr.creator_major);
	vd->vhdhdr.creator_minor = bswap16(vd->vhdhdr.creator_minor);
//	vd->vhd.creator_os = bswap32(vd->vhd.creator_os);
	vd->vhdhdr.size_original = bswap64(vd->vhdhdr.size_original);
	vd->vhdhdr.size_current = bswap64(vd->vhdhdr.size_current);
	vd->vhdhdr.cylinders = bswap16(vd->vhdhdr.cylinders);
	vd->vhdhdr.checksum = bswap32(vd->vhdhdr.checksum);
	uid_swap(&vd->vhdhdr.uuid);
#endif

	if (vd->vhdhdr.type != VHD_DISK_FIXED) {
		if (os_fseek(vd->fd, vd->vhdhdr.offset, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		if (os_fread(vd->fd, &vd->vhddyn, sizeof(VHD_DYN_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		if (vd->vhddyn.magic != VHD_DYN_MAGIC)
			return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE__, __func__);

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
			vd->vhddyn.max_entries != vd->vhdhdr.size_original / vd->vhddyn.blocksize)
			return vdisk_i_err(vd, VVD_EVDMISC, __LINE__, __func__);

		vd->u32blockcount = vd->vhddyn.max_entries;
		vd->vhd_blockmask = vd->vhddyn.blocksize - 1;
		vd->vhd_blockshift = fpow2(vd->vhddyn.blocksize);

		if (vd->u32blockcount == 0)
			return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE__, __func__);
		if (os_fseek(vd->fd, vd->vhddyn.table_offset, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

		int batsize = vd->u32blockcount << 2; // "* 4"
		if ((vd->u32block = malloc(batsize)) == NULL)
			return vdisk_i_err(vd, VVD_ENOMEM, __LINE__, __func__);
		if (os_fread(vd->fd, vd->u32block, batsize))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
#if ENDIAN_LITTLE
		for (size_t i = 0; i < vd->u32blockcount; ++i)
			vd->u32block[i] = bswap32(vd->u32block[i]);
#endif
		vd->cb.lba_read = vdisk_vhd_dyn_read_lba;
	} else { // Fixed
		vd->cb.lba_read = vdisk_vhd_fixed_read_lba;
	}
	vd->offset = 0;

	vd->capacity = vd->vhdhdr.size_original;
	return 0;
}

//
// vdisk_vhd_fixed_read_lba
//

int vdisk_vhd_fixed_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	
	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	return 0;
}

//
// vdisk_vhd_dyn_read_lba
//

int vdisk_vhd_dyn_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	
	uint64_t offset = SECTOR_TO_BYTE(index);
	uint32_t bi = (uint32_t)(offset >> vd->vhd_blockshift);
#ifdef TRACE
	printf("%s: bi=%u\n", __func__, bi);
#endif
	if (bi >= vd->u32blockcount)
		return vdisk_i_err(vd, VVD_EVDBOUND, __LINE__, __func__);

	uint32_t block = vd->u32block[bi];
	if (block == VHD_BLOCK_UNALLOC) // Unallocated
		return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE__, __func__);

	uint64_t base = SECTOR_TO_BYTE(block) + 512;
	offset = base + (offset & vd->vhd_blockmask);
#ifdef TRACE
	printf("%s: block=%u  offset=%" PRIu64 "\n", __func__, block, offset);
#endif
	if (os_fseek(vd->fd, offset, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fread(vd->fd, buffer, 512))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	return 0;
}
