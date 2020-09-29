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

int vdisk_i_err(VDISK *vd, int e, int l, const char *f) {
	vd->err.line = l - 1;
	vd->err.func = f;
	return (vd->err.num = e);
}

// Until all implementations are done, this allows to catch
// non-implemented functions during operation
void vdisk_i_pre_init(VDISK *vd) {
	memset(&vd->cb, 0, sizeof(vd->cb));
}

//
// vdisk_open
//

int vdisk_open(VDISK *vd, const oschar *path, uint32_t flags) {
	if ((vd->fd = os_fopen(path)) == 0)
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	vdisk_i_pre_init(vd);

	if (flags & VDISK_RAW)
		return vdisk_raw_open(vd, flags, 0);

	//
	// Format detection
	//
	// This hints the function to a format and tests both reading and
	// seeking capabilities on the file or device.
	//

	if (os_fread(vd->fd, &vd->format, 4))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
	if (os_fseek(vd->fd, 0, SEEK_SET))
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	//
	// Disk detection and loading
	//

	uint32_t internal = 0; // Internal flags

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		if (vdisk_vdi_open(vd, flags, internal))
			return vd->err.num;
		break;
	case VDISK_FORMAT_VMDK:
		if (vdisk_vmdk_open(vd, flags, internal))
			return vd->err.num;
		break;
	case VDISK_FORMAT_VHD: L_FORMAT_CASE_VHD:
		if (vdisk_vhd_open(vd, flags, internal))
			return vd->err.num;
		break;
	case VDISK_FORMAT_VHDX:
		if (vdisk_vhdx_open(vd, flags, internal))
			return vd->err.num;
		break;
	case VDISK_FORMAT_QED:
		if (vdisk_qed_open(vd, flags, internal))
			return vd->err.num;
		break;
	case VDISK_FORMAT_QCOW:
		if (vdisk_qcow_open(vd, flags, internal))
			return vd->err.num;
		break;
	case VDISK_FORMAT_PHDD:
		if (vdisk_phdd_open(vd, flags, internal))
			return vd->err.num;
		break;
	default: // Attempt at different offsets

		// Fixed VHD: 512 bytes before EOF
		if (os_fseek(vd->fd, -512, SEEK_END))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		if (os_fread(vd->fd, &vd->vhdhdr, sizeof(VHD_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		if (vd->vhdhdr.magic == VHD_MAGIC) {
			vd->format = VDISK_FORMAT_VHD;
			internal = 2;
			goto L_FORMAT_CASE_VHD;
		}

		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE__, __func__);
	}

	return 0;
}

//
// vdisk_create
//

int vdisk_create(VDISK *vd, const oschar *path, int format, uint64_t capacity, uint16_t flags) {
	
	if (flags & VDISK_CREATE_TEMP) {
		//TODO: Attach random number
		path = osstr("vdisk.tmp");
	} else if (path == NULL)
		return vdisk_i_err(vd, VVD_ENULL, __LINE__, __func__);

	if (capacity == 0)
		return vdisk_i_err(vd, VVD_EVDBOUND, __LINE__, __func__);

	if ((vd->fd = os_fcreate(path)) == 0)
		return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);

	if (flags & VDISK_RAW) {
		vd->format = VDISK_FORMAT_RAW;
		if (os_falloc(vd->fd, capacity))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		return VVD_EOK;
	}

	int e;
	switch (format) {
	case VDISK_FORMAT_VDI:
		e = vdisk_vdi_create(vd, capacity, flags);
		break;
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE__, __func__);
	}

	if (e)
		return vd->err.num;

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
	
	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		//TODO: Move pre-header signature in creation function
		if (os_fseek(vd->fd, 0, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		if (os_fwrite(vd->fd, VDI_SIGNATURE, 40))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		// skip signature
		if (os_fseek(vd->fd, VDI_SIGNATURE_SIZE, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		if (os_fwrite(vd->fd, vd->vdi.hdr, sizeof(VDI_HDR)))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		if (os_fwrite(vd->fd, vd->vdi.v1, sizeof(VDI_HEADERv1)))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		// blocks
		if (os_fseek(vd->fd, vd->vdi.v1->offBlocks, SEEK_SET))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		if (os_fwrite(vd->fd, vd->vdi.in->offsets, vd->vdi.v1->blk_total * 4))
			return vdisk_i_err(vd, VVD_EOS, __LINE__, __func__);
		break;
	/*case VDISK_FORMAT_VMDK:
		assert(0);
		break;
	case VDISK_FORMAT_VHD:
		assert(0);
		break;*/
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE__, __func__);
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
	
	//TODO: Consider an assert
	if (vd->cb.lba_read == NULL)
		return vdisk_i_err(vd, VVD_EVDTODO, __LINE__, __func__);

	return vd->cb.lba_read(vd, buffer, lba);
}

//
//TODO: Consider vdisk_read_sectors
//      Read multiple sectors at once
//

//
// vdisk_write_lba
//

int vdisk_write_lba(VDISK *vd, void *buffer, uint64_t lba) {
	
	assert(0);

	return VVD_EOK;
}

//
// vdisk_read_block
//

int vdisk_read_block(VDISK *vd, void *buffer, uint64_t index) {

	assert(0);

	return VVD_EOK;
}

//
// vdisk_write_block
//

int vdisk_write_block(VDISK *vd, void *buffer, uint64_t index) {

	assert(0);

	return VVD_EOK;
}

//
// vdisk_write_block_at
//

int vdisk_write_block_at(VDISK *vd, void *buffer, uint64_t bindex, uint64_t dindex) {	

	assert(0);

	return VVD_EOK;
}

//
// vdisk_op_compact
//

int vdisk_op_compact(VDISK *vd, void(*cb)(uint32_t, void*)) {
	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		return vdisk_vdi_compact(vd, cb);
	default:
		return vdisk_i_err(vd, VVD_EVDFORMAT, __LINE__, __func__);
	}
}

//
// vdisk_error
//

const char* vdisk_error(VDISK *vd) {
	switch (vd->err.num) {
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
		vd->err.num = GetLastError();
		int l = GetLocaleInfoEx( // Recommended over MAKELANGID
			LOCALE_NAME_USER_DEFAULT,
			LOCALE_ALL,
			0,
			0);
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
			NULL,
			vd->err.num,
			l,
			_errmsgbuf,
			512,
			NULL);
		return _errmsgbuf;
#else
		return strerror(vd->err.num = errno);
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
		vd->err.func, vd->err.line, vd->err.num, vdisk_error(vd));
}
