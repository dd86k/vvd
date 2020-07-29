#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include "utils.h"
#include "vdisk.h"

//
// vdisk_open
//

int vdisk_open(VDISK *vd, const _oschar *path, uint16_t flags) {
	vdisk_func = __func__;

	if ((vd->fd = os_open(path)) == 0) {
		vdisk_errln = __LINE_BEFORE__;
		return (vdisk_errno = VVD_EVDOPEN);
	}

	if (flags & VDISK_RAW) {
		vd->format = VDISK_FORMAT_RAW;
		vd->offset = 0;
		if (os_size(vd->fd, &vd->capacity))
			return (vdisk_errno = VVD_EVDMISC);
		return (vdisk_errno = VVD_EVDOK);
	}

	//
	// Format detection
	//
	// This hints the function to a format and tests both reading and
	// seeking capabilities on the file or device.
	//

	if (os_read(vd->fd, &vd->format, 4)) {
		vdisk_errln = __LINE_BEFORE__;
		return (vdisk_errno = VVD_EVDREAD);
	}
	if (os_seek(vd->fd, 0, SEEK_SET)) {
		vdisk_errln = __LINE_BEFORE__;
		return (vdisk_errno = VVD_EVDSEEK);
	}

	//
	// Disk detection and loading
	//

	switch (vd->format) {
	//
	// VDI
	//
	case VDISK_FORMAT_VDI:
		if (os_seek(vd->fd, 64, SEEK_SET)) {
			vdisk_errln = __LINE_BEFORE__;
			return (vdisk_errno = VVD_EVDSEEK);
		}
		if (os_read(vd->fd, &vd->vdihdr, sizeof(VDI_HDR))) {
			vdisk_errln = __LINE_BEFORE__;
			return (vdisk_errno = VVD_EVDREAD);
		}
		if (vd->vdihdr.magic != VDI_HEADER_MAGIC) {
			vdisk_errln = __LINE_BEFORE__;
			return (vdisk_errno = VVD_EVDMAGIC);
		}

		switch (vd->vdihdr.majorv) { // Use latest major version natively
		case 1: // Includes all minor releases
			if (os_read(vd->fd, &vd->vdi, sizeof(VDIHEADER1))) {
				vdisk_errln = __LINE_BEFORE__;
				return (vdisk_errno = VVD_EVDREAD);
			}
			break;
		case 0: { // Or else, translate header
			VDIHEADER0 vd0;
			if (os_read(vd->fd, &vd0, sizeof(VDIHEADER0))) {
				vdisk_errln = __LINE_BEFORE__;
				return (vdisk_errno = VVD_EVDREAD);
			}
			vd->vdi.disksize = vd0.disksize;
			vd->vdi.type = vd0.type;
			vd->vdi.offBlocks = sizeof(VDI_HDR) + sizeof(VDIHEADER0);
			vd->vdi.offData = sizeof(VDI_HDR) + sizeof(VDIHEADER0) +
				(vd0.totalblocks << 2); // sizeof(uint32_t) -> "* 4" -> "<< 2"
			memcpy(&vd->vdi.uuidCreate, &vd0.uuidCreate, 16);
			memcpy(&vd->vdi.uuidModify, &vd0.uuidModify, 16);
			memcpy(&vd->vdi.uuidLinkage, &vd0.uuidLinkage, 16);
			memcpy(&vd->vdi.LegacyGeometry, &vd0.LegacyGeometry,
				sizeof(VDIDISKGEOMETRY));
			break;
		}
		default:
			return (vdisk_errno = VVD_EVDVERSION);
		}

		switch (vd->vdi.type) {
		case VDI_DISK_DYN:
		case VDI_DISK_FIXED: break;
		default:
			return (vdisk_errno = VVD_EVDTYPE);
		}

		// allocation table
		if (vd->vdi.blocksize == 0) {
			vdisk_errln = __LINE_BEFORE__;
			vd->vdi.blocksize = VDI_BLOCKSIZE;
		}
		//TODO: Consider warning if blocksize != offBlocks?
		if (os_seek(vd->fd, vd->vdi.offBlocks, SEEK_SET)) {
			vdisk_errln = __LINE_BEFORE__;
			return (vdisk_errno = VVD_EVDSEEK);
		}
		int bsize = vd->vdi.totalblocks << 2; // * sizeof(u32)
		if ((vd->u32blocks = malloc(bsize)) == NULL) {
			vdisk_errln = __LINE_BEFORE__;
			return (vdisk_errno = VVD_EVDMISC);
		}
		if (os_read(vd->fd, vd->u32blocks, bsize)) {
			vdisk_errln = __LINE_BEFORE__;
			return (vdisk_errno = VVD_EVDREAD);
		}
		vd->offset = vd->vdi.offData;
		vd->u32nblocks = vd->vdi.totalblocks;
		vd->capacity = vd->vdi.disksize;
		break; // VDISK_FORMAT_VDI
	//
	// VMDK
	//
	case VDISK_FORMAT_VMDK:
		if (os_read(vd->fd, &vd->vmdk, sizeof(VMDK_HDR)))
			return (vdisk_errno = VVD_EVDREAD);
		if (vd->vmdk.version != 1)
			return (vdisk_errno = VVD_EVDVERSION);
		if (vd->vmdk.grainSize < 1 || vd->vmdk.grainSize > 128 || pow2(vd->vmdk.grainSize) == 0)
			return (vdisk_errno = VVD_EVDMISC);
		vd->offset = SECTOR_TO_BYTE(vd->vmdk.overHead);
		vd->capacity = SECTOR_TO_BYTE(vd->vmdk.capacity);
		break; // VDISK_FORMAT_VMDK
	//
	// VHD
	//
	case VDISK_FORMAT_VHD:
		if (os_read(vd->fd, &vd->vhd, sizeof(VHD_HDR)))
			return (vdisk_errno = VVD_EVDREAD);
		if (vd->vhd.magic != VHD_MAGIC)
			return (vdisk_errno = VVD_EVDMAGIC);
L_VHD_MAGICOK:
		vd->vhd.major = bswap16(vd->vhd.major);
		if (vd->vhd.major != 1)
			return (vdisk_errno = VVD_EVDVERSION);
		vd->vhd.type = bswap32(vd->vhd.type);
		switch (vd->vhd.type) {
		case VHD_DISK_DIFF:
		case VHD_DISK_DYN:
		case VHD_DISK_FIXED: break;
		default:
			return (vdisk_errno = VVD_EVDTYPE);
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
			if (os_seek(vd->fd, vd->vhd.offset, SEEK_SET))
				return (vdisk_errno = VVD_EVDSEEK);
			if (os_read(vd->fd, &vd->vhddyn, sizeof(VHD_DYN_HDR)))
				return (vdisk_errno = VVD_EVDREAD);
			if (vd->vhddyn.magic != VHD_DYN_MAGIC)
				return (vdisk_errno = VVD_EVDMAGIC);
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
				return (vdisk_errno = VVD_EVDMISC);
			if (os_seek(vd->fd, vd->vhddyn.table_offset, SEEK_SET))
				return (vdisk_errno = VVD_EVDSEEK);
			int batsize = vd->u32nblocks << 2; // "* sizeof(u32)"
			if ((vd->u32blocks = malloc(batsize)) == NULL)
				return (vdisk_errno = VVD_EVDMISC);
			if (os_read(vd->fd, vd->u32blocks, batsize))
				return (vdisk_errno = VVD_EVDREAD);
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
		if (os_read(vd->fd, &vd->vhdx, sizeof(VHDX_HDR)))
			return (vdisk_errno = VVD_EVDREAD);
		if (vd->vhdx.magic != VHDX_MAGIC)
			return (vdisk_errno = VVD_EVDMAGIC);
		// HEADER1
		if (os_seek(vd->fd, VHDX_HEADER1_LOC, SEEK_SET))
			return (vdisk_errno = VVD_EVDSEEK);
		if (os_read(vd->fd, &vd->vhdxhdr, sizeof(VHDX_HEADER1)))
			return (vdisk_errno = VVD_EVDREAD);
		if (vd->vhdxhdr.magic != VHDX_HDR1_MAGIC)
			return (vdisk_errno = VVD_EVDMAGIC);
		// REGION
		if (os_seek(vd->fd, VHDX_REGION1_LOC, SEEK_SET))
			return (vdisk_errno = VVD_EVDSEEK);
		if (os_read(vd->fd, &vd->vhdxreg, sizeof(VHDX_REGION_HDR)))
			return (vdisk_errno = VVD_EVDREAD);
		if (vd->vhdxreg.magic != VHDX_REGION_MAGIC)
			return (vdisk_errno = VVD_EVDMAGIC);
		// LOG
		// unsupported
		// BAT
		
		// Chunk ratio
		//(8388608 * ) / // 8 KiB * 512
		break; // VDISK_FORMAT_VHDX*/
	default:
		// Attempt at different offsets

		// Fixed VHD: 512 bytes before EOF
		if (os_seek(vd->fd, -512, SEEK_END))
			return (vdisk_errno = VVD_EVDSEEK);
		if (os_read(vd->fd, &vd->vhd, sizeof(VHD_HDR)))
			return (vdisk_errno = VVD_EVDREAD);
		if (vd->vhd.magic == VHD_MAGIC) {
			vd->format = VDISK_FORMAT_VHD;
			goto L_VHD_MAGICOK;
		}

		return (vdisk_errno = VVD_EVDFORMAT);
	}

	return (vdisk_errno = VVD_EVDOK);
}

//
// vdisk_create
//

int vdisk_create(VDISK *vd, const _oschar *path, int format, uint64_t capacity, uint16_t flags) {
	vdisk_func = __func__;

	if (flags & VDISK_CREATE_TEMP) {
		//TODO: Attach random number
#ifdef _WIN32
		path = L"vdisk.tmp";
#else
		path = "vdisk.tmp";
#endif
	}

	if (path == NULL)
		return (vdisk_errno = VVD_EVDMISC);
	if (capacity == 0)
		return (vdisk_errno = VVD_EVDBOUND);

	if ((vd->fd = os_create(path)) == 0)
		return (vdisk_errno = VVD_EVDOPEN);

	if (flags & VDISK_RAW) {
		vd->format = VDISK_FORMAT_RAW;
		if (os_seek(vd->fd, capacity - 1, SEEK_SET))
			return (vdisk_errno = VVD_EVDSEEK);
		uint8_t __n = 0;
		if (os_write(vd->fd, &__n, 1))
			return (vdisk_errno = VVD_EVDWRITE);
		return (vdisk_errno = VVD_EVDOK);
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
		// Work
		vd->u32nblocks = capacity / vd->vdi.blocksize;
		if (vd->u32nblocks == 0)
			vd->u32nblocks = 1;
		uint32_t bsize = vd->u32nblocks << 2;
		if ((vd->u32blocks = malloc(bsize)) == NULL)
			return VVD_EVDALLOC;
		vd->vdi.totalblocks = vd->u32nblocks;
		vd->vdi.disksize = capacity;
		switch (vd->vdi.type) {
		case VDI_DISK_DYN:
			for (size_t i = 0; i < vd->vdi.totalblocks; ++i)
				vd->u32blocks[i] = VDI_BLOCK_UNALLOCATED;
			break;
		case VDI_DISK_FIXED:
			if ((buffer = malloc(vd->vdi.blocksize)) == NULL)
				return VVD_EVDALLOC;
			os_seek(vd->fd, vd->vdi.offData, SEEK_SET);
			for (size_t i = 0; i < vd->vdi.totalblocks; ++i) {
				vd->u32blocks[i] = VDI_BLOCK_FREE;
				os_write(vd->fd, buffer, vd->vdi.blocksize);
			}
			break;
		default:
			return (vdisk_errno = VVD_EVDTYPE);
		}
		break;
	default:
		return (vdisk_errno = VVD_EVDFORMAT);
	}

	return vdisk_update(vd);
}

//
// vdisk_str
//

char* vdisk_str(VDISK *vd) {
	vdisk_func = __func__;

	switch (vd->format) {
	case VDISK_FORMAT_RAW:	return "RAW";
	case VDISK_FORMAT_VDI:	return "VDI";
	case VDISK_FORMAT_VMDK:	return "VMDK";
	case VDISK_FORMAT_VHD:	return "VHD";
	case VDISK_FORMAT_VHDX:	return "VHDX";
	case VDISK_FORMAT_QED:	return "QED";
	case VDISK_FORMAT_QCOW:	return "QCOW";
	default: assert(0);	return NULL;
	}
}

//
// vdisk_update
//

int vdisk_update(VDISK *vd) {
	vdisk_func = __func__;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		os_seek(vd->fd, 0, SEEK_SET);
		os_write(vd->fd, VDI_SIGNATURE, 40);
		// skip signature
		os_seek(vd->fd, VDI_SIGNATURE_SIZE, SEEK_SET);
		os_write(vd->fd, &vd->vdihdr, sizeof(VDI_HDR));
		os_write(vd->fd, &vd->vdi, sizeof(VDIHEADER1));
		// blocks
		os_seek(vd->fd, vd->vdi.offBlocks, SEEK_SET);
		os_write(vd->fd, vd->u32blocks, vd->u32nblocks * 4);
		break;
	/*case VDISK_FORMAT_VMDK:
		assert(0);
		break;
	case VDISK_FORMAT_VHD:
		assert(0);
		break;*/
	default:
		return (vdisk_errno = VVD_EVDFORMAT);
	}

	return (vdisk_errno = VVD_EVDOK);
}

//
// vdisk_read_lba
//

int vdisk_read_lba(VDISK *vd, void *buffer, uint64_t lba) {
	vdisk_func = __func__;

	uint64_t fpos; // New file position
	uint64_t boff = SECTOR_TO_BYTE(lba); // Byte offset
	size_t bi; // Block index

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		bi = boff / vd->vdi.blocksize;
		if (bi >= vd->vdi.totalblocks) // out of bounds
			return (vdisk_errno = VVD_EVDBOUND);
		if (vd->u32blocks[bi] == VDI_BLOCK_UNALLOCATED ||
			vd->u32blocks[bi] == VDI_BLOCK_FREE)
			//TODO: Consider memset'ing buffer instead
			return (vdisk_errno = VVD_EVDMISC);
		fpos = vd->offset + (vd->u32blocks[bi] * vd->vdi.blocksize) + boff;
		break;
	case VDISK_FORMAT_VMDK:
		if (lba >= vd->vmdk.capacity)
			return (vdisk_errno = VVD_EVDMISC);
		//bi = boff / SECTOR_TO_BYTE(vd->vmdk.grainSize);
		fpos = vd->offset + boff;
		break;
	case VDISK_FORMAT_VHD:
		switch (vd->vhd.type) {
		case VHD_DISK_FIXED:
			fpos = boff;
			break;
		case VHD_DISK_DYN:
			// selected index
			bi = boff / vd->vhddyn.blocksize;
			if (bi >= vd->u32nblocks) // Over
				return VVD_EVDMISC;
			if (vd->u32blocks[bi] == 0xFFFFFFFF) // Unallocated
				return VVD_EVDMISC;
			fpos = SECTOR_TO_BYTE(vd->u32blocks[bi]) + boff + 512;
			break;
		default:
			return (vdisk_errno = VVD_EVDTYPE);
		}
		break;
	case VDISK_FORMAT_RAW:
		fpos = boff;
		break;
	default:
		return (vdisk_errno = VVD_EVDFORMAT);
	}

	if (os_seek(vd->fd, fpos, SEEK_SET))
		return (vdisk_errno = VVD_EVDSEEK);
	if (os_read(vd->fd, buffer, 512))
		return (vdisk_errno = VVD_EVDREAD);

	return (vdisk_errno = VVD_EVDOK);
}

//
// vdisk_read_block
//

int vdisk_read_block(VDISK *vd, void *buffer, uint64_t index) {
	vdisk_func = __func__;

	uint32_t readsize;
	uint64_t pos;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		if (index >= vd->u32nblocks)
			return (vdisk_errno = VVD_EVDMISC);
		readsize = vd->vdi.blocksize;
		if (vd->u32blocks[index] == VDI_BLOCK_UNALLOCATED)
			return (vdisk_errno = VVD_EVDUNALLOC);
		pos = (vd->u32blocks[index] * vd->vdi.blocksize) + vd->vdi.offData;
		break;
	case VDISK_FORMAT_VMDK:
		return (vdisk_errno = VVD_EVDTODO);
	case VDISK_FORMAT_VHD:
		if (vd->vhd.type != VHD_DISK_DYN)
			return (vdisk_errno = VVD_EVDTYPE);
		readsize = vd->vhddyn.blocksize;
		break;
	default:
		return (vdisk_errno = VVD_EVDFORMAT);
	}

	if (os_seek(vd->fd, pos, SEEK_SET))
		return (vdisk_errno = VVD_EVDSEEK);
	if (os_read(vd->fd, buffer, readsize))
		return (vdisk_errno = VVD_EVDREAD);

	return (vdisk_errno = VVD_EVDOK);
}

//
// vdisk_write_lba
//

int vdisk_write_lba(VDISK *vd, void *buffer, uint64_t lba) {
	vdisk_func = __func__;

	return VVD_EVDOK;
}

//
// vdisk_write_block
//

int vdisk_write_block(VDISK *vd, void *buffer, uint64_t index) {
	vdisk_func = __func__;

	uint64_t pos;
	uint64_t blocksize;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		//TODO: What should we do if we run out of allocation blocks?
		if (index >= vd->u32nblocks)
			return (vdisk_errno = VVD_EVDMISC);
		if (vd->u32blocks[index] == VDI_BLOCK_UNALLOCATED) {
			pos = vd->nextblock;
			vd->u32blocks[index] = ((pos - vd->offset) / vd->vdi.blocksize);
			vd->nextblock += vd->vdi.blocksize;
		} else {
			pos = (vd->u32blocks[index] * vd->vdi.blocksize) + vd->vdi.offData;
		}
		blocksize = vd->vdi.blocksize;
		break;
	default:
		return (vdisk_errno = VVD_EVDFORMAT);
	}

	if (os_seek(vd->fd, pos, SEEK_SET))
		return (vdisk_errno = VVD_EVDSEEK);
	if (os_write(vd->fd, buffer, blocksize))
		return (vdisk_errno = VVD_EVDWRITE);

	return (vdisk_errno = VVD_EVDOK);
}

//
// vdisk_write_block_at
//

int vdisk_write_block_at(VDISK *vd, void *buffer, uint64_t bindex, uint64_t dindex) {
	vdisk_func = __func__;

	uint64_t pos;
	uint64_t blocksize;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		if (dindex >= vd->u32nblocks)
			return (vdisk_errno = VVD_EVDMISC);
		pos = (dindex * vd->vdi.blocksize) + vd->offset;
		blocksize = vd->vdi.blocksize;
		vd->u32blocks[bindex] = dindex;
		break;
	default:
		return (vdisk_errno = VVD_EVDFORMAT);
	}

	if (os_seek(vd->fd, pos, SEEK_SET)) {
		return (vdisk_errno = VVD_EVDSEEK);
	}
	if (os_write(vd->fd, buffer, blocksize))
		return (vdisk_errno = VVD_EVDWRITE);

	return (vdisk_errno = VVD_EVDOK);
}

//
// vdisk_error
//

const char* vdisk_error() {
	switch (vdisk_errno) {
	case VVD_EVDOK:
		return "last operation was successful";
	case VVD_EVDOPEN:
		return "could not open vdisk";
	case VVD_EVDREAD:
		return "could not read vdisk";
	case VVD_EVDSEEK:
		return "could not seek vdisk";
	case VVD_EVDWRITE:
		return "could not write vdisk";
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
	default:
		assert(0); return NULL;
	}
}

//
// vdisk_perror
//

void vdisk_perror(const char *func) {
	fprintf(stderr, "[%s] %s L%u: (%d) %s\n",
		func, vdisk_func, vdisk_errln, vdisk_errno, vdisk_error());
}

//
// vdisk_last_errno
//

int vdisk_last_errno() {
	return vdisk_errno;
}
