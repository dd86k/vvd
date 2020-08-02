#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include "utils.h"
#include "vdisk.h"

// Internal: Set errcode and errline
int vdisk_i_err(VDISK *vd, int e, int l) {
	vd->errline = l;
	return (vd->errcode = e);
}

//
// vdisk_open
//

int vdisk_open(VDISK *vd, const _oschar *path, uint16_t flags) {
	vd->errfunc = __func__;

	if ((vd->fd = os_open(path)) == 0)
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	if (flags & VDISK_RAW) {
		vd->format = VDISK_FORMAT_RAW;
		vd->offset = 0;
		if (os_fsize(vd->fd, &vd->capacity))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		return (vd->errcode = VVD_EOK);
	}

	//
	// Format detection
	//
	// This hints the function to a format and tests both reading and
	// seeking capabilities on the file or device.
	//

	if (os_fread(vd->fd, &vd->format, 4))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (os_fseek(vd->fd, 0, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	//
	// Disk detection and loading
	//

	switch (vd->format) {
	//
	// VDI
	//
	case VDISK_FORMAT_VDI:
		if (os_fseek(vd->fd, 64, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (os_fread(vd->fd, &vd->vdihdr, sizeof(VDI_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (vd->vdihdr.magic != VDI_HEADER_MAGIC)
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

		switch (vd->vdihdr.majorv) { // Use latest major version natively
		case 1: // Includes all minor releases
			if (os_fread(vd->fd, &vd->vdi, sizeof(VDIHEADER1)))
				return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
			break;
		case 0: { // Or else, translate header
			VDIHEADER0 vd0;
			if (os_fread(vd->fd, &vd0, sizeof(VDIHEADER0)))
				return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
			vd->vdi.disksize = vd0.disksize;
			vd->vdi.type = vd0.type;
			vd->vdi.offBlocks = sizeof(VDI_HDR) + sizeof(VDIHEADER0);
			vd->vdi.offData = sizeof(VDI_HDR) + sizeof(VDIHEADER0) +
				(vd0.totalblocks << 2); // sizeof(uint32_t) -> "* 4" -> "<< 2"
			memcpy(&vd->vdi.uuidCreate, &vd0.uuidCreate, 16);
			memcpy(&vd->vdi.uuidModify, &vd0.uuidModify, 16);
			memcpy(&vd->vdi.uuidLinkage, &vd0.uuidLinkage, 16);
			memcpy(&vd->vdi.LegacyGeometry, &vd0.LegacyGeometry, sizeof(VDIDISKGEOMETRY));
			break;
		}
		default:
			return vdisk_i_err(vd, VVD_EVDVERSION, __LINE_BEFORE__);
		}

		switch (vd->vdi.type) {
		case VDI_DISK_DYN:
		case VDI_DISK_FIXED: break;
		default:
			return vdisk_i_err(vd, VVD_EVDTYPE, __LINE_BEFORE__);
		}

		// allocation table
		//TODO: Consider if this is an error (or warning)
		if (vd->vdi.blocksize == 0)
			vd->vdi.blocksize = VDI_BLOCKSIZE;
		if (os_fseek(vd->fd, vd->vdi.offBlocks, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		int bsize = vd->vdi.totalblocks << 2; // * sizeof(u32)
		if ((vd->u32blocks = malloc(bsize)) == NULL)
			return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
		if (os_fread(vd->fd, vd->u32blocks, bsize))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		vd->offset = vd->vdi.offData;
		vd->u32nblocks = vd->vdi.totalblocks;
		vd->capacity = vd->vdi.disksize;
		break; // VDISK_FORMAT_VDI
	//
	// VMDK
	//
	case VDISK_FORMAT_VMDK:
		if (os_fread(vd->fd, &vd->vmdk, sizeof(VMDK_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (vd->vmdk.version != 1)
			return vdisk_i_err(vd, VVD_EVDVERSION, __LINE_BEFORE__);
		if (vd->vmdk.grainSize < 1 || vd->vmdk.grainSize > 128 || pow2(vd->vmdk.grainSize) == 0)
			return vdisk_i_err(vd, VVD_EVDMISC, __LINE_BEFORE__);
		vd->offset = SECTOR_TO_BYTE(vd->vmdk.overHead);
		vd->capacity = SECTOR_TO_BYTE(vd->vmdk.capacity);
		break; // VDISK_FORMAT_VMDK
	//
	// VHD
	//
	case VDISK_FORMAT_VHD:
		if (os_fread(vd->fd, &vd->vhd, sizeof(VHD_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (vd->vhd.magic != VHD_MAGIC)
			return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE_BEFORE__);
L_VHD_MAGICOK:
		vd->vhd.major = bswap16(vd->vhd.major);
		if (vd->vhd.major != 1)
			return vdisk_i_err(vd, VVD_EVDVERSION, __LINE_BEFORE__);
		vd->vhd.type = bswap32(vd->vhd.type);
		switch (vd->vhd.type) {
		case VHD_DISK_DIFF:
		case VHD_DISK_DYN:
		case VHD_DISK_FIXED: break;
		default:
			return vdisk_i_err(vd, VVD_EVDTYPE, __LINE_BEFORE__);
		}
		vd->vhd.features = bswap32(vd->vhd.features);
		vd->vhd.minor = bswap16(vd->vhd.minor);
		vd->vhd.offset = bswap64(vd->vhd.offset);
		vd->vhd.timestamp = bswap32(vd->vhd.timestamp);
		vd->vhd.creator_major = bswap16(vd->vhd.creator_major);
		vd->vhd.creator_minor = bswap16(vd->vhd.creator_minor);
//		vd->vhd.creator_os = bswap32(vd->vhd.creator_os);
		vd->vhd.size_original = bswap64(vd->vhd.size_original);
		vd->vhd.size_current = bswap64(vd->vhd.size_current);
		vd->vhd.cylinders = bswap16(vd->vhd.cylinders);
		vd->vhd.checksum = bswap32(vd->vhd.checksum);
		uid_swap(&vd->vhd.uuid);
		if (vd->vhd.type != VHD_DISK_FIXED) {
			if (os_fseek(vd->fd, vd->vhd.offset, SEEK_SET))
				return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
			if (os_fread(vd->fd, &vd->vhddyn, sizeof(VHD_DYN_HDR)))
				return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
			if (vd->vhddyn.magic != VHD_DYN_MAGIC)
				return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE_BEFORE__);
			vd->vhddyn.data_offset = bswap64(vd->vhddyn.data_offset);
			vd->vhddyn.table_offset = bswap64(vd->vhddyn.table_offset);
			vd->vhddyn.minor = bswap16(vd->vhddyn.minor);
			vd->vhddyn.major = bswap16(vd->vhddyn.major);
			vd->vhddyn.max_entries = bswap32(vd->vhddyn.max_entries);
			vd->vhddyn.blocksize = bswap32(vd->vhddyn.blocksize);
			vd->vhddyn.checksum = bswap32(vd->vhddyn.checksum);
			uid_swap(&vd->vhddyn.parent_uuid);
			vd->vhddyn.parent_timestamp = bswap32(vd->vhddyn.parent_timestamp);
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
			vd->u32nblocks = vd->vhd.size_original / vd->vhddyn.blocksize;
			if (vd->u32nblocks <= 0)
				return vdisk_i_err(vd, VVD_EVDMAGIC, __LINE_BEFORE__);
			if (os_fseek(vd->fd, vd->vhddyn.table_offset, SEEK_SET))
				return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
			int batsize = vd->u32nblocks << 2; // "* sizeof(u32)"
			if ((vd->u32blocks = malloc(batsize)) == NULL)
				return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
			if (os_fread(vd->fd, vd->u32blocks, batsize))
				return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
			for (size_t i = 0; i < vd->u32nblocks; ++i)
				vd->u32blocks[i] = bswap32(vd->u32blocks[i]);
			vd->offset = SECTOR_TO_BYTE(vd->u32blocks[0]) + 512;
		} else {
			vd->offset = 0;
		}
		vd->capacity = vd->vhd.size_original;
		break; // VDISK_FORMAT_VHD
	/*case VDISK_FORMAT_VHDX:
		// HDR
		if (os_fread(vd->fd, &vd->vhdx, sizeof(VHDX_HDR))) {
			vd->errline = __LINE_BEFORE__;
			return (vd->errcode = VVD_EOS);
		}
		if (vd->vhdx.magic != VHDX_MAGIC) {
			vd->errline = __LINE_BEFORE__;
			return (vd->errcode = VVD_EVDMAGIC);
		}
		// HEADER1
		if (os_seek(vd->fd, VHDX_HEADER1_LOC, SEEK_SET)) {
			vd->errline = __LINE_BEFORE__;
			return (vd->errcode = VVD_EOS);
		}
		if (os_fread(vd->fd, &vd->vhdxhdr, sizeof(VHDX_HEADER1))) {
			vd->errline = __LINE_BEFORE__;
			return (vd->errcode = VVD_EOS)
		};
		if (vd->vhdxhdr.magic != VHDX_HDR1_MAGIC) {
			vd->errline = __LINE_BEFORE__;
			return (vd->errcode = VVD_EVDMAGIC);
		}
		// REGION
		if (os_seek(vd->fd, VHDX_REGION1_LOC, SEEK_SET)) {
			vd->errline = __LINE_BEFORE__;
			return (vd->errcode = VVD_EOS);
		}
		if (os_fread(vd->fd, &vd->vhdxreg, sizeof(VHDX_REGION_HDR))) {
			vd->errline = __LINE_BEFORE__;
			return (vd->errcode = VVD_EOS)
		};
		if (vd->vhdxreg.magic != VHDX_REGION_MAGIC) {
			vd->errline = __LINE_BEFORE__;
			return (vd->errcode = VVD_EVDMAGIC);
		}
		// LOG
		// unsupported
		// BAT
		
		// Chunk ratio
		//(8388608 * ) / // 8 KiB * 512
		break; // VDISK_FORMAT_VHDX*/
	default:
		// Attempt at different offsets

		// Fixed VHD: 512 bytes before EOF
		if (os_fseek(vd->fd, -512, SEEK_END))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (os_fread(vd->fd, &vd->vhd, sizeof(VHD_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (vd->vhd.magic == VHD_MAGIC) {
			vd->format = VDISK_FORMAT_VHD;
			goto L_VHD_MAGICOK;
		}

		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE_BEFORE__);
	}

	return (vd->errcode = VVD_EOK);
}

//
// vdisk_create
//

int vdisk_create(VDISK *vd, const _oschar *path, int format, uint64_t capacity, uint16_t flags) {
	vd->errfunc = __func__;

	if (flags & VDISK_CREATE_TEMP) {
		//TODO: Attach random number
#ifdef _WIN32
		path = L"vdisk.tmp";
#else
		path = "vdisk.tmp";
#endif
	}

	if (path == NULL)
		return vdisk_i_err(vd, VVD_ENULL, __LINE_BEFORE__);
	if (capacity == 0)
		return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);

	if ((vd->fd = os_create(path)) == 0)
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	if (flags & VDISK_RAW) {
		vd->format = VDISK_FORMAT_RAW;
		if (os_falloc(vd->fd, capacity))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		return (vd->errcode = VVD_EOK);
	}

	uint8_t *buffer;
	int vdisk_fixed = flags & VDISK_CREATE_FIXED;

	switch (format) {
	case VDISK_FORMAT_VDI:
		vd->format = VDISK_FORMAT_VDI;
		vd->vdi.type = vdisk_fixed ? VDI_DISK_FIXED : VDI_DISK_DYN;
		// Default values
		// hdr
		vd->vdihdr.magic = VDI_HEADER_MAGIC;
		vd->vdihdr.majorv = 1;
		vd->vdihdr.minorv = 1;
		// struct
		vd->vdi.blocksalloc = 0;
		vd->vdi.blocksextra = 0;
		vd->vdi.blocksize = VDI_BLOCKSIZE;
		vd->vdi.cbSector = vd->vdi.LegacyGeometry.cbSector = 512;
		vd->vdi.cCylinders = vd->vdi.cHeads = vd->vdi.cSectors =
			vd->vdi.LegacyGeometry.cCylinders =
			vd->vdi.LegacyGeometry.cHeads =
			vd->vdi.LegacyGeometry.cSectors = 0;
		vd->vdi.disksize = 0;
		vd->vdi.fFlags = 0;
		vd->vdi.hdrsize = (uint32_t)sizeof(VDIHEADER1);
		vd->vdi.offBlocks = VDI_BLOCKSIZE;
		vd->vdi.offData = 2 * VDI_BLOCKSIZE;
		vd->vdi.totalblocks = 0;
		vd->vdi.type = VDI_DISK_DYN;
		vd->vdi.u32Dummy = 0; // Always
		memset(vd->vdi.szComment, 0, VDI_COMMENT_SIZE);
		memset(&vd->vdi.uuidCreate, 0, 16);
		memset(&vd->vdi.uuidLinkage, 0, 16);
		memset(&vd->vdi.uuidModify, 0, 16);
		memset(&vd->vdi.uuidParentModify, 0, 16);
		// Data
		vd->u32nblocks = capacity / vd->vdi.blocksize;
		if (vd->u32nblocks == 0)
			vd->u32nblocks = 1;
		uint32_t bsize = vd->u32nblocks << 2;
		if ((vd->u32blocks = malloc(bsize)) == NULL)
			return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
		vd->vdi.totalblocks = vd->u32nblocks;
		vd->vdi.disksize = capacity;
		switch (vd->vdi.type) {
		case VDI_DISK_DYN:
			for (size_t i = 0; i < vd->vdi.totalblocks; ++i)
				vd->u32blocks[i] = VDI_BLOCK_UNALLOC;
			break;
		case VDI_DISK_FIXED:
			if ((buffer = malloc(vd->vdi.blocksize)) == NULL)
				return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
			os_fseek(vd->fd, vd->vdi.offData, SEEK_SET);
			for (size_t i = 0; i < vd->vdi.totalblocks; ++i) {
				vd->u32blocks[i] = VDI_BLOCK_FREE;
				os_fwrite(vd->fd, buffer, vd->vdi.blocksize);
			}
			break;
		default:
			return vdisk_i_err(vd, VVD_EVDTYPE, __LINE_BEFORE__);
		}
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE_BEFORE__);
	}

	return vdisk_update(vd);
}

//
// vdisk_str
//

const char* vdisk_str(VDISK *vd) {
	vd->errfunc = __func__;

	switch (vd->format) {
	case VDISK_FORMAT_RAW:	return "RAW";
	case VDISK_FORMAT_VDI:	return "VDI";
	case VDISK_FORMAT_VMDK:	return "VMDK";
	case VDISK_FORMAT_VHD:	return "VHD";
	case VDISK_FORMAT_VHDX:	return "VHDX";
	case VDISK_FORMAT_QED:	return "QED";
	case VDISK_FORMAT_QCOW:	return "QCOW";
	default:	return NULL; // Not opened, etc.
	}
}

//
// vdisk_update
//

int vdisk_update(VDISK *vd) {
	vd->errfunc = __func__;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		if (os_fseek(vd->fd, 0, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (os_fwrite(vd->fd, VDI_SIGNATURE, 40))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		// skip signature
		if (os_fseek(vd->fd, VDI_SIGNATURE_SIZE, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (os_fwrite(vd->fd, &vd->vdihdr, sizeof(VDI_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (os_fwrite(vd->fd, &vd->vdi, sizeof(VDIHEADER1)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		// blocks
		if (os_fseek(vd->fd, vd->vdi.offBlocks, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (os_fwrite(vd->fd, vd->u32blocks, vd->u32nblocks * 4))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		break;
	/*case VDISK_FORMAT_VMDK:
		assert(0);
		break;
	case VDISK_FORMAT_VHD:
		assert(0);
		break;*/
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE_BEFORE__);
	}

	return (vd->errcode = VVD_EOK);
}

//
// vdisk_read_lba
//

int vdisk_read_lba(VDISK *vd, void *buffer, uint64_t lba) {
	vd->errfunc = __func__;

	uint64_t base; // Base offset with dealing with blocks
	uint64_t offset = SECTOR_TO_BYTE(lba); // Byte offset
	size_t bi; // Block index

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		bi = offset / vd->vdi.blocksize;
		if (bi >= vd->vdi.totalblocks) // out of bounds
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		if (vd->u32blocks[bi] == VDI_BLOCK_UNALLOC)
			return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE_BEFORE__);
		base = vd->u32blocks[bi] * vd->vdi.blocksize;
		offset = vd->offset + base + (offset - base);
		break;
	case VDISK_FORMAT_VMDK:
		if (lba >= vd->vmdk.capacity)
			return vdisk_i_err(vd, VVD_EVDMISC, __LINE_BEFORE__);
		//bi = offset / SECTOR_TO_BYTE(vd->vmdk.grainSize);
		offset += vd->offset;
		break;
	case VDISK_FORMAT_VHD:
		switch (vd->vhd.type) {
		case VHD_DISK_FIXED: break;
		case VHD_DISK_DYN:
			bi = lba / vd->vhddyn.blocksize;
			if (bi >= vd->u32nblocks) // Over
				return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
			if (vd->u32blocks[bi] == VHD_BLOCK_UNALLOC) // Unallocated
				return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE_BEFORE__);
			base = SECTOR_TO_BYTE(vd->u32blocks[bi] * vd->vhddyn.blocksize);
			offset = vd->offset + base + (offset - base);
			break;
		default:
			return VDISK_ERR(VVD_EVDTYPE);
		}
		break;
	case VDISK_FORMAT_RAW:
		if (offset >= vd->capacity)
			return VDISK_ERR(VVD_EVDBOUND);
		break;
	default:
		return (vd->errcode = VVD_EVDFORMAT);
	}

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return (vd->errcode = VVD_EOS);
	if (os_fread(vd->fd, buffer, 512))
		return (vd->errcode = VVD_EOS);

	return (vd->errcode = VVD_EOK);
}

//
// vdisk_read_block
//

int vdisk_read_block(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint32_t readsize;
	uint64_t pos;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		if (index >= vd->u32nblocks)
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		readsize = vd->vdi.blocksize;
		if (vd->u32blocks[index] == VDI_BLOCK_UNALLOC)
			return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE_BEFORE__);
		pos = (vd->u32blocks[index] * vd->vdi.blocksize) + vd->vdi.offData;
		break;
	case VDISK_FORMAT_VMDK:
		return vdisk_i_err(vd, VVD_EVDTODO, __LINE_BEFORE__);
	case VDISK_FORMAT_VHD:
		if (vd->vhd.type != VHD_DISK_DYN)
			return vdisk_i_err(vd, VVD_EVDTYPE, __LINE_BEFORE__);
		readsize = vd->vhddyn.blocksize;
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE_BEFORE__);
	}

	if (os_fseek(vd->fd, pos, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (os_fread(vd->fd, buffer, readsize))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	return (vd->errcode = VVD_EOK);
}

//
// vdisk_write_lba
//

int vdisk_write_lba(VDISK *vd, void *buffer, uint64_t lba) {
	vd->errfunc = __func__;

	assert(0);

	return VVD_EOK;
}

//
// vdisk_write_block
//

int vdisk_write_block(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint64_t pos;
	uint64_t blocksize;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		//TODO: What should we do if we run out of allocation blocks?
		if (index >= vd->u32nblocks)
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		if (vd->u32blocks[index] == VDI_BLOCK_UNALLOC) {
			pos = vd->nextblock;
			vd->u32blocks[index] = ((pos - vd->offset) / vd->vdi.blocksize);
			vd->nextblock += vd->vdi.blocksize;
		} else {
			pos = (vd->u32blocks[index] * vd->vdi.blocksize) + vd->vdi.offData;
		}
		blocksize = vd->vdi.blocksize;
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE_BEFORE__);
	}

	if (os_fseek(vd->fd, pos, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (os_fwrite(vd->fd, buffer, blocksize))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	return (vd->errcode = VVD_EOK);
}

//
// vdisk_write_block_at
//

int vdisk_write_block_at(VDISK *vd, void *buffer, uint64_t bindex, uint64_t dindex) {
	vd->errfunc = __func__;

	uint64_t pos;
	uint64_t blocksize;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		if (dindex >= vd->u32nblocks)
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		pos = (dindex * vd->vdi.blocksize) + vd->offset;
		blocksize = vd->vdi.blocksize;
		vd->u32blocks[bindex] = dindex;
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE_BEFORE__);
	}

	if (os_fseek(vd->fd, pos, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
	if (os_fwrite(vd->fd, buffer, blocksize))
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	return (vd->errcode = VVD_EOK);
}

//
// vdisk_error
//


const char* vdisk_error(VDISK *vd) {
	switch (vd->errcode) {
	case VVD_EOK:
		return "last operation was successful";
	case VVD_ENULL:
		return "input pointer is null";
	case VVD_EVDFORMAT:
		return "unsupported vdisk format";
	case VVD_EVDMAGIC:
		return "invalid magic";
	case VVD_EVDVERSION:
		return "unsupported version";
	case VVD_EVDTYPE:
		return "invalid disk type for vdisk function";
	case VVD_EVDFULL:
		return "vdisk is full";
	case VVD_EVDUNALLOC:
		return "block is unallocated";
	case VVD_EVDBOUND:
		return "block index is out of bounds";
	case VVD_EVDTODO:
		return "currently unimplemented";
	case VVD_EVDMISC:
		return "unknown error happened";
	case VVD_EOS:
#if _WIN32
		// We're using the Win32 API, not the CRT functions
		static char _errmsgbuf[512];
		vd->errcode = GetLastError();
		int l = GetLocaleInfoEx( // Recommended over MAKELANGID
			LOCALE_NAME_USER_DEFAULT,
			LOCALE_ALL,
			0,
			0);
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			vd->errcode,
			l,
			_errmsgbuf,
			512,
			NULL);
		return _errmsgbuf;
#else
		return strerror(vd->errcode = errno);
#endif
	default:
		assert(0); return NULL;
	}
}

//
// vdisk_perror
//

#if _WIN32
	#define ERRFMT "%08X"
#else
	#define ERRFMT "%d"
#endif

void vdisk_perror(VDISK *vd) {
	fprintf(stderr, "%s@%u: (" ERRFMT ") %s\n",
		vd->errfunc, vd->errline, vd->errcode, vdisk_error(vd));
}
