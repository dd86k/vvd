#include "vdisk/vdisk.h"
#include "utils/bin.h"
#include "utils/platform.h"
#include "utils/bin.h"
#ifdef TRACE
#include <stdio.h>
#include <inttypes.h>
#endif

//
// vdisk_vhd_open
//

int vdisk_vhd_open(VDISK *vd, uint32_t flags, uint32_t internal) {
	if ((vd->vmdk = malloc(VHD_META_ALLOC)) == NULL)
		return VDISK_ERROR(vd, VVD_ENOMEM);

	if (internal & 2) {
		if (os_fseek(vd->fd, -512, SEEK_END))
			return VDISK_ERROR(vd, VVD_EOS);
	}

	if (os_fread(vd->fd, &vd->vhd->hdr, sizeof(VHD_HDR)))
		return VDISK_ERROR(vd, VVD_EOS);
	if (vd->vhd->hdr.magic != VHD_MAGIC)
		return VDISK_ERROR(vd, VVD_EVDMAGIC);

#if ENDIAN_LITTLE
	vd->vhd->hdr.major = bswap16(vd->vhd->hdr.major);
#endif

	if (vd->vhd->hdr.major != 1)
		return VDISK_ERROR(vd, VVD_EVDVERSION);
	
#if ENDIAN_LITTLE
	vd->vhd->hdr.type = bswap32(vd->vhd->hdr.type);
#endif

	switch (vd->vhd->hdr.type) {
	case VHD_DISK_DIFF:
	case VHD_DISK_DYN:
	case VHD_DISK_FIXED: break;
	default:
		return VDISK_ERROR(vd, VVD_EVDTYPE);
	}

#if ENDIAN_LITTLE
	vd->vhd->hdr.features = bswap32(vd->vhd->hdr.features);
	vd->vhd->hdr.minor = bswap16(vd->vhd->hdr.minor);
	vd->vhd->hdr.offset = bswap64(vd->vhd->hdr.offset);
	vd->vhd->hdr.timestamp = bswap32(vd->vhd->hdr.timestamp);
	vd->vhd->hdr.creator_major = bswap16(vd->vhd->hdr.creator_major);
	vd->vhd->hdr.creator_minor = bswap16(vd->vhd->hdr.creator_minor);
//	vd->vhd->creator_os = bswap32(vd->vhd->creator_os);
	vd->vhd->hdr.size_original = bswap64(vd->vhd->hdr.size_original);
	vd->vhd->hdr.size_current = bswap64(vd->vhd->hdr.size_current);
	vd->vhd->hdr.cylinders = bswap16(vd->vhd->hdr.cylinders);
	vd->vhd->hdr.checksum = bswap32(vd->vhd->hdr.checksum);
	uid_swap(&vd->vhd->hdr.uuid);
#endif

	if (vd->vhd->hdr.type != VHD_DISK_FIXED) {
		if (os_fseek(vd->fd, vd->vhd->hdr.offset, SEEK_SET))
			return VDISK_ERROR(vd, VVD_EOS);
		if (os_fread(vd->fd, &vd->vhd->dyn, sizeof(VHD_DYN_HDR)))
			return VDISK_ERROR(vd, VVD_EOS);
		if (vd->vhd->dyn.magic != VHD_DYN_MAGIC)
			return VDISK_ERROR(vd, VVD_EVDMAGIC);

#if ENDIAN_LITTLE
		vd->vhd->dyn.data_offset = bswap64(vd->vhd->dyn.data_offset);
		vd->vhd->dyn.table_offset = bswap64(vd->vhd->dyn.table_offset);
		vd->vhd->dyn.minor = bswap16(vd->vhd->dyn.minor);
		vd->vhd->dyn.major = bswap16(vd->vhd->dyn.major);
		vd->vhd->dyn.max_entries = bswap32(vd->vhd->dyn.max_entries);
		vd->vhd->dyn.blocksize = bswap32(vd->vhd->dyn.blocksize);
		vd->vhd->dyn.checksum = bswap32(vd->vhd->dyn.checksum);
		vd->vhd->dyn.parent_timestamp = bswap32(vd->vhd->dyn.parent_timestamp);
		uid_swap(&vd->vhd->dyn.parent_uuid);

		for (size_t i = 0; i < 8; ++i) {
			vd->vhd->dyn.parent_locator[i].code =
				bswap32(vd->vhd->dyn.parent_locator[i].code);
			vd->vhd->dyn.parent_locator[i].datasize =
				bswap32(vd->vhd->dyn.parent_locator[i].datasize);
			vd->vhd->dyn.parent_locator[i].dataspace =
				bswap32(vd->vhd->dyn.parent_locator[i].dataspace);
			vd->vhd->dyn.parent_locator[i].offset =
				bswap64(vd->vhd->dyn.parent_locator[i].offset);
		}
#endif

		if (pow2(vd->vhd->dyn.blocksize) == 0 ||
			vd->vhd->dyn.max_entries != vd->vhd->hdr.size_original / vd->vhd->dyn.blocksize)
			return VDISK_ERROR(vd, VVD_EVDMISC);

		vd->vhd->in.mask  = vd->vhd->dyn.blocksize - 1;
		vd->vhd->in.shift = fpow2(vd->vhd->dyn.blocksize);

		if (vd->vhd->dyn.max_entries == 0)
			return VDISK_ERROR(vd, VVD_EVDMAGIC);
		if (os_fseek(vd->fd, vd->vhd->dyn.table_offset, SEEK_SET))
			return VDISK_ERROR(vd, VVD_EOS);

		int batsize = vd->vhd->dyn.max_entries << 2; // "* 4"
		if ((vd->vhd->in.offsets = malloc(batsize)) == NULL)
			return VDISK_ERROR(vd, VVD_ENOMEM);
		if (os_fread(vd->fd, vd->vhd->in.offsets, batsize))
			return VDISK_ERROR(vd, VVD_EOS);
#if ENDIAN_LITTLE
		for (size_t i = 0; i < vd->vhd->dyn.max_entries; ++i)
			vd->vhd->in.offsets[i] = bswap32(vd->vhd->in.offsets[i]);
#endif
		vd->cb.lba_read = vdisk_vhd_dyn_read_lba;
	} else { // Fixed
		vd->cb.lba_read = vdisk_vhd_fixed_read_lba;
	}

	vd->capacity = vd->vhd->hdr.size_original;
	return 0;
}

//
// vdisk_vhd_fixed_read_lba
//

int vdisk_vhd_fixed_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	uint64_t offset = SECTOR_TO_BYTE(index); // Byte offset

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, buffer, 512))
		return VDISK_ERROR(vd, VVD_EOS);

	return 0;
}

//
// vdisk_vhd_dyn_read_lba
//

int vdisk_vhd_dyn_read_lba(VDISK *vd, void *buffer, uint64_t index) {
	uint64_t offset = SECTOR_TO_BYTE(index);
	uint32_t bi = (uint32_t)(offset >> vd->vhd->in.shift);
#ifdef TRACE
	printf("%s: bi=%u\n", __func__, bi);
#endif
	if (bi >= vd->vhd->dyn.max_entries)
		return VDISK_ERROR(vd, VVD_EVDBOUND);

	uint32_t block = vd->vhd->in.offsets[bi];
	if (block == VHD_BLOCK_UNALLOC) // Unallocated
		return VDISK_ERROR(vd, VVD_EVDUNALLOC);

	uint64_t base = SECTOR_TO_BYTE(block) + 512;
	offset = base + (offset & vd->vhd->in.mask);
#ifdef TRACE
	printf("%s: block=%u  offset=%" PRIu64 "\n", __func__, block, offset);
#endif
	if (os_fseek(vd->fd, offset, SEEK_SET))
		return VDISK_ERROR(vd, VVD_EOS);
	if (os_fread(vd->fd, buffer, 512))
		return VDISK_ERROR(vd, VVD_EOS);

	return 0;
}
