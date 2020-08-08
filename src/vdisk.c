#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include "utils.h"
#include "vdisk.h"

//
// vdisk_i_err
//

int vdisk_i_err(VDISK *vd, int e, int l) {
	vd->errline = l;
	return (vd->errcode = e);
}

//
// vdisk_open
//

int vdisk_open(VDISK *vd, const _oschar *path, uint32_t flags) {
	vd->errfunc = __func__;

	if ((vd->fd = os_fopen(path)) == 0)
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

	int (*vdopen)(VDISK*,uint32_t,uint32_t); // vd open function pointer
	uint32_t internal = 0; // Internal flags

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		vdopen = vdisk_vdi_open;
		break;
	case VDISK_FORMAT_VMDK:
		vdopen = vdisk_vmdk_open;
		break;
	case VDISK_FORMAT_VHD:
		vdopen = vdisk_vhd_open;
		break;
	case VDISK_FORMAT_VHDX:
		vdopen = vdisk_vhdx_open;
		break;
	case VDISK_FORMAT_QED:
		vdopen = vdisk_qed_open;
		break;
	case VDISK_FORMAT_QCOW:
		vdopen = vdisk_qcow_open;
		break;
	case VDISK_FORMAT_PHDD:
		vdopen = vdisk_phdd_open;
		break;
	default: // Attempt at different offsets

		// Fixed VHD: 512 bytes before EOF
		if (os_fseek(vd->fd, -512, SEEK_END))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (os_fread(vd->fd, &vd->vhd, sizeof(VHD_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		if (vd->vhd.magic == VHD_MAGIC) {
			vdopen = vdisk_vhd_open;
			internal = 2;
			goto L_OPEN;
		}

		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE_BEFORE__);
	}

	//
	// Open
	//

L_OPEN:
	if (vdopen(vd, flags, internal))
		return vd->errcode;

	return 0;
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

	if ((vd->fd = os_fcreate(path)) == 0)
		return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);

	if (flags & VDISK_RAW) {
		vd->format = VDISK_FORMAT_RAW;
		if (os_falloc(vd->fd, capacity))
			return vdisk_i_err(vd, VVD_EOS, __LINE_BEFORE__);
		return (vd->errcode = VVD_EOK);
	}

	uint8_t *buffer;
	int vdisk_fixed = flags & VDISK_CREATE_FIXED;

	//TODO: vdisk_*_create internal functions

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
		vd->vdi.cCylinders =
			vd->vdi.cHeads =
			vd->vdi.cSectors =
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
		vd->u32blockcount = capacity / vd->vdi.blocksize;
		if (vd->u32blockcount == 0)
			vd->u32blockcount = 1;
		uint32_t bsize = vd->u32blockcount << 2;
		if ((vd->u32block = malloc(bsize)) == NULL)
			return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
		vd->vdi.totalblocks = vd->u32blockcount;
		vd->vdi.disksize = capacity;
		switch (vd->vdi.type) {
		case VDI_DISK_DYN:
			for (size_t i = 0; i < vd->vdi.totalblocks; ++i)
				vd->u32block[i] = VDI_BLOCK_ZERO;
			break;
		case VDI_DISK_FIXED:
			if ((buffer = malloc(vd->vdi.blocksize)) == NULL)
				return vdisk_i_err(vd, VVD_EALLOC, __LINE_BEFORE__);
			os_fseek(vd->fd, vd->vdi.offData, SEEK_SET);
			for (size_t i = 0; i < vd->vdi.totalblocks; ++i) {
				vd->u32block[i] = VDI_BLOCK_FREE;
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
	switch (vd->format) {
	case VDISK_FORMAT_VDI:	return "VDI";
	case VDISK_FORMAT_VMDK:	return "VMDK";
	case VDISK_FORMAT_VHD:	return "VHD";
	case VDISK_FORMAT_VHDX:	return "VHDX";
	case VDISK_FORMAT_QED:	return "QED";
	case VDISK_FORMAT_QCOW:	return "QCOW";
	case VDISK_FORMAT_PHDD:	return "Parallels";
	case VDISK_FORMAT_RAW:	return "RAW";
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
		if (os_fwrite(vd->fd, vd->u32block, vd->u32blockcount * 4))
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

	return 0;
}

//
//TODO: vdisk_flush(VDISK *vd)
//

//
// vdisk_read_sector
//

int vdisk_read_sector(VDISK *vd, void *buffer, uint64_t lba) {
	vd->errfunc = __func__;

	uint64_t offset = SECTOR_TO_BYTE(lba); // Byte offset
	uint64_t base; // Base offset with dealing with blocks
	size_t bi; // Block index

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		bi = offset >> vd->vdi_blockshift;
		if (bi >= vd->vdi.totalblocks) // out of bounds
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		switch (vd->u32block[bi]) {
		case VDI_BLOCK_ZERO:
			return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE_BEFORE__);
		case VDI_BLOCK_FREE:
			memset(buffer, 0, 512);
			return 0;
		}
		if (vd->u32block[bi] == VDI_BLOCK_ZERO)
			return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE_BEFORE__);
		base = vd->u32block[bi] * vd->vdi.blocksize;
		offset = vd->offset + base + (offset & vd->vdi_blockmask);
#if TRACE
		printf("* %s: lba=%" PRId64 " -> offset=0x%" PRIX64 "\n", __func__, lba, offset);
#endif
		break;
	case VDISK_FORMAT_VMDK:
		if (lba >= vd->vmdk.capacity)
			return vdisk_i_err(vd, VVD_EVDMISC, __LINE_BEFORE__);
		//bi = offset / SECTOR_TO_BYTE(vd->vmdk.grainSize);
		//TODO: Work with the grainSize
		offset += vd->offset;
		break;
	case VDISK_FORMAT_VHD:
		switch (vd->vhd.type) {
		case VHD_DISK_FIXED: break;
		case VHD_DISK_DYN:
			bi = lba / vd->vhddyn.blocksize;
			if (bi >= vd->u32blockcount) // Over
				return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
			if (vd->u32block[bi] == VHD_BLOCK_UNALLOC) // Unallocated
				return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE_BEFORE__);
			base = SECTOR_TO_BYTE(vd->u32block[bi] * vd->vhddyn.blocksize);
			offset = vd->offset + base + (offset - base);
			break;
		default:
			return vdisk_i_err(vd, VVD_EVDTYPE, __LINE_BEFORE__);
		}
		break;
//	case VDISK_FORMAT_VHDX:
	case VDISK_FORMAT_QED:
//		bi = offset >> vd->blockshift;
		
		break;
	case VDISK_FORMAT_RAW:
		if (offset >= vd->capacity)
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE_BEFORE__);
	}

	if (os_fseek(vd->fd, offset, SEEK_SET))
		return (vd->errcode = VVD_EOS);
	if (os_fread(vd->fd, buffer, 512))
		return (vd->errcode = VVD_EOS);

	return 0;
}

//
//TODO: Consider vdisk_read_sectors
//      Read multiple sectors at once
//

//
// vdisk_write_lba
//

int vdisk_write_lba(VDISK *vd, void *buffer, uint64_t lba) {
	vd->errfunc = __func__;

	assert(0);

	return VVD_EOK;
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
		if (index >= vd->u32blockcount)
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		if (vd->u32block[index] == VDI_BLOCK_ZERO)
			return vdisk_i_err(vd, VVD_EVDUNALLOC, __LINE_BEFORE__);
		readsize = vd->vdi.blocksize;
		pos = vd->offset + (vd->u32block[index] * vd->vdi.blocksize);
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
// vdisk_write_block
//

int vdisk_write_block(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint64_t pos;
	uint64_t blocksize;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		//TODO: What should we do if we run out of allocation blocks?
		if (index >= vd->u32blockcount)
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		if (vd->u32block[index] == VDI_BLOCK_ZERO) {
			pos = vd->nextblock;
			vd->u32block[index] = ((pos - vd->offset) / vd->vdi.blocksize);
			vd->nextblock += vd->vdi.blocksize;
		} else {
			pos = (vd->u32block[index] * vd->vdi.blocksize) + vd->vdi.offData;
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
		if (dindex >= vd->u32blockcount)
			return vdisk_i_err(vd, VVD_EVDBOUND, __LINE_BEFORE__);
		pos = (dindex * vd->vdi.blocksize) + vd->offset;
		blocksize = vd->vdi.blocksize;
		vd->u32block[bindex] = dindex;
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
