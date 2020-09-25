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

void vdisk_i_pre_init(VDISK *vd) {
	// Until all implementations are done, this allows to catch
	// non-implemented functions during operation
	vd->read_lba = vd->write_lba =
		vd->read_block = vd->write_block = NULL;
}

//
// vdisk_open
//

int vdisk_open(VDISK *vd, const oschar *path, uint32_t flags) {
	vd->errfunc = __func__;

	if ((vd->fd = os_fopen(path)) == 0)
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	vdisk_i_pre_init(vd);

	if (flags & VDISK_RAW) {
		if (os_fsize(vd->fd, &vd->capacity))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		vd->format = VDISK_FORMAT_RAW;
		vd->offset = 0;
		vd->read_lba = vdisk_raw_read_lba;
		return 0;
	}

	//
	// Format detection
	//
	// This hints the function to a format and tests both reading and
	// seeking capabilities on the file or device.
	//

	if (os_fread(vd->fd, &vd->format, 4))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fseek(vd->fd, 0, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	//
	// Disk detection and loading
	//

	uint32_t internal = 0; // Internal flags

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		if (vdisk_vdi_open(vd, flags, internal))
			return vd->errcode;
		break;
	case VDISK_FORMAT_VMDK:
		if (vdisk_vmdk_open(vd, flags, internal))
			return vd->errcode;
		break;
	case VDISK_FORMAT_VHD: L_FORMAT_CASE_VHD:
		if (vdisk_vhd_open(vd, flags, internal))
			return vd->errcode;
		break;
	case VDISK_FORMAT_VHDX:
		if (vdisk_vhdx_open(vd, flags, internal))
			return vd->errcode;
		break;
	case VDISK_FORMAT_QED:
		if (vdisk_qed_open(vd, flags, internal))
			return vd->errcode;
		break;
	case VDISK_FORMAT_QCOW:
		if (vdisk_qcow_open(vd, flags, internal))
			return vd->errcode;
		break;
	case VDISK_FORMAT_PHDD:
		if (vdisk_phdd_open(vd, flags, internal))
			return vd->errcode;
		break;
	default: // Attempt at different offsets

		// Fixed VHD: 512 bytes before EOF
		if (os_fseek(vd->fd, -512, SEEK_END))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		if (os_fread(vd->fd, &vd->vhd, sizeof(VHD_HDR)))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		if (vd->vhd.magic == VHD_MAGIC) {
			vd->format = VDISK_FORMAT_VHD;
			internal = 2;
			goto L_FORMAT_CASE_VHD;
		}

		return vdisk_i_err(vd, VVD_EVDFORMAT, LINE_BEFORE);
	}

	return 0;
}

//
// vdisk_create
//

int vdisk_create(VDISK *vd, const oschar *path, int format, uint64_t capacity, uint16_t flags) {
	vd->errfunc = __func__;

	if (flags & VDISK_CREATE_TEMP) {
		//TODO: Attach random number
		path = osstr("vdisk.tmp");
	} else if (path == NULL)
		return vdisk_i_err(vd, VVD_ENULL, LINE_BEFORE);

	if (capacity == 0)
		return vdisk_i_err(vd, VVD_EVDBOUND, LINE_BEFORE);

	if ((vd->fd = os_fcreate(path)) == 0)
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	if (flags & VDISK_RAW) {
		vd->format = VDISK_FORMAT_RAW;
		if (os_falloc(vd->fd, capacity))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		return (vd->errcode = VVD_EOK);
	}

	uint8_t *buffer;
	int vdisk_fixed = flags & VDISK_CREATE_FIXED;

	//TODO: vdisk_*_create internal functions

	switch (format) {
	case VDISK_FORMAT_VDI:
		vd->format = VDISK_FORMAT_VDI;
		vd->vdiv1.type = vdisk_fixed ? VDI_DISK_FIXED : VDI_DISK_DYN;
		// Default values
		// hdr
		vd->vdihdr.magic = VDI_HEADER_MAGIC;
		vd->vdihdr.majorv = 1;
		vd->vdihdr.minorv = 1;
		// struct
		vd->vdiv1.blocksalloc = 0;
		vd->vdiv1.blocksextra = 0;
		vd->vdiv1.blocksize = VDI_BLOCKSIZE;
		vd->vdiv1.cbSector = vd->vdiv1.LegacyGeometry.cbSector = 512;
		vd->vdiv1.cCylinders =
			vd->vdiv1.cHeads =
			vd->vdiv1.cSectors =
			vd->vdiv1.LegacyGeometry.cCylinders =
			vd->vdiv1.LegacyGeometry.cHeads =
			vd->vdiv1.LegacyGeometry.cSectors = 0;
		vd->vdiv1.disksize = 0;
		vd->vdiv1.fFlags = 0;
		vd->vdiv1.hdrsize = (uint32_t)sizeof(VDIHEADER1);
		vd->vdiv1.offBlocks = VDI_BLOCKSIZE;
		vd->vdiv1.offData = 2 * VDI_BLOCKSIZE;
		vd->vdiv1.blockstotal = 0;
		vd->vdiv1.type = VDI_DISK_DYN;
		vd->vdiv1.u32Dummy = 0; // Always
		memset(vd->vdiv1.szComment, 0, VDI_COMMENT_SIZE);
		memset(&vd->vdiv1.uuidCreate, 0, 16);
		memset(&vd->vdiv1.uuidLinkage, 0, 16);
		memset(&vd->vdiv1.uuidModify, 0, 16);
		memset(&vd->vdiv1.uuidParentModify, 0, 16);
		// Data
		vd->u32blockcount = capacity / vd->vdiv1.blocksize;
		if (vd->u32blockcount == 0)
			vd->u32blockcount = 1;
		uint32_t bsize = vd->u32blockcount << 2;
		if ((vd->u32block = malloc(bsize)) == NULL)
			return vdisk_i_err(vd, VVD_ENOMEM, LINE_BEFORE);
		vd->vdiv1.blockstotal = vd->u32blockcount;
		vd->vdiv1.disksize = capacity;
		switch (vd->vdiv1.type) {
		case VDI_DISK_DYN:
			for (size_t i = 0; i < vd->vdiv1.blockstotal; ++i)
				vd->u32block[i] = VDI_BLOCK_ZERO;
			break;
		case VDI_DISK_FIXED:
			if ((buffer = malloc(vd->vdiv1.blocksize)) == NULL)
				return vdisk_i_err(vd, VVD_ENOMEM, LINE_BEFORE);
			os_fseek(vd->fd, vd->vdiv1.offData, SEEK_SET);
			for (size_t i = 0; i < vd->vdiv1.blockstotal; ++i) {
				vd->u32block[i] = VDI_BLOCK_FREE;
				os_fwrite(vd->fd, buffer, vd->vdiv1.blocksize);
			}
			break;
		default:
			return vdisk_i_err(vd, VVD_EVDTYPE, LINE_BEFORE);
		}
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, LINE_BEFORE);
	}

	return vdisk_update(vd);
}

//
//TODO: vdisk_close(VDISK *vd)
//



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
		//TODO: Move pre-header signature in creation function
		if (os_fseek(vd->fd, 0, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		if (os_fwrite(vd->fd, VDI_SIGNATURE, 40))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		// skip signature
		if (os_fseek(vd->fd, VDI_SIGNATURE_SIZE, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		if (os_fwrite(vd->fd, &vd->vdihdr, sizeof(VDI_HDR)))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		if (os_fwrite(vd->fd, &vd->vdiv1, sizeof(VDIHEADER1)))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		// blocks
		if (os_fseek(vd->fd, vd->vdiv1.offBlocks, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		if (os_fwrite(vd->fd, vd->u32block, vd->u32blockcount * 4))
			return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
		break;
	/*case VDISK_FORMAT_VMDK:
		assert(0);
		break;
	case VDISK_FORMAT_VHD:
		assert(0);
		break;*/
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, LINE_BEFORE);
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

	//TODO: Consider an assert
	if (vd->read_lba == NULL)
		return vdisk_i_err(vd, VVD_EVDTODO, LINE_BEFORE);

	return vd->read_lba(vd, buffer, lba);
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

//TODO: vdisk_get_block_ptr(VDISK)
//TODO: vdisk_get_block_size(VDISK)
//TODO: vdisk_read_block(VDISK, index)
//TODO: vdisk_write_block(VDISK, index)
//TODO: vdisk_write_block_at(VDISK, source_index, target_index)

int vdisk_read_block(VDISK *vd, void *buffer, uint64_t index) {
	vd->errfunc = __func__;

	uint32_t readsize;
	uint64_t pos;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		if (index >= vd->u32blockcount)
			return vdisk_i_err(vd, VVD_EVDBOUND, LINE_BEFORE);
		if (vd->u32block[index] == VDI_BLOCK_ZERO)
			return vdisk_i_err(vd, VVD_EVDUNALLOC, LINE_BEFORE);
		readsize = vd->vdiv1.blocksize;
		pos = vd->offset + (vd->u32block[index] * vd->vdiv1.blocksize);
		break;
	case VDISK_FORMAT_VMDK:
		return vdisk_i_err(vd, VVD_EVDTODO, LINE_BEFORE);
	case VDISK_FORMAT_VHD:
		if (vd->vhd.type != VHD_DISK_DYN)
			return vdisk_i_err(vd, VVD_EVDTYPE, LINE_BEFORE);
		readsize = vd->vhddyn.blocksize;
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, LINE_BEFORE);
	}

	if (os_fseek(vd->fd, pos, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fread(vd->fd, buffer, readsize))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

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
			return vdisk_i_err(vd, VVD_EVDBOUND, LINE_BEFORE);
		if (vd->u32block[index] == VDI_BLOCK_ZERO) {
			pos = vd->nextblock;
			vd->u32block[index] = ((pos - vd->offset) / vd->vdiv1.blocksize);
			vd->nextblock += vd->vdiv1.blocksize;
		} else {
			pos = (vd->u32block[index] * vd->vdiv1.blocksize) + vd->vdiv1.offData;
		}
		blocksize = vd->vdiv1.blocksize;
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, LINE_BEFORE);
	}

	if (os_fseek(vd->fd, pos, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fwrite(vd->fd, buffer, blocksize))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

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
			return vdisk_i_err(vd, VVD_EVDBOUND, LINE_BEFORE);
		pos = (dindex * vd->vdiv1.blocksize) + vd->offset;
		blocksize = vd->vdiv1.blocksize;
		vd->u32block[bindex] = dindex;
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, LINE_BEFORE);
	}

	if (os_fseek(vd->fd, pos, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);
	if (os_fwrite(vd->fd, buffer, blocksize))
		return vdisk_i_err(vd, VVD_EOS, LINE_BEFORE);

	return (vd->errcode = VVD_EOK);
}

//
// vdisk_op_compact
//

int vdisk_op_compact(VDISK *vd, void(*cb)(uint32_t, void*)) {
	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		return vdisk_vdi_compact(vd, cb);
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE__);
	}
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
		// We're using the Win32 API, not the CRT functions, which may
		// yield different and probably unrelated messages
		static char _errmsgbuf[512];
		vd->errcode = GetLastError();
		int l = GetLocaleInfoEx( // Recommended over MAKELANGID
			LOCALE_NAME_USER_DEFAULT,
			LOCALE_ALL,
			0,
			0);
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
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
