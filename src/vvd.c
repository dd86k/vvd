/**
 * Main vvd operations
 */
#include <assert.h>
#include <string.h> // memcpy
#include <inttypes.h>
#include "vvd.h"
#include "utils.h"
#include "fs/mbr.h"
#include "fs/gpt.h"

//
// vvd_info
//

int vvd_info(VDISK *vd) {
	const char *type; // vdisk type
	char disksize[BIN_FLENGTH];

	switch (vd->format) {
	//
	// VDI
	//
	case VDISK_FORMAT_VDI: {
		switch (vd->vdi.type) {
		case VDI_DISK_DYN:	type = "dynamic"; break;
		case VDI_DISK_FIXED:	type = "fixed"; break;
		case VDI_DISK_UNDO:	type = "undo"; break;
		case VDI_DISK_DIFF:	type = "diff"; break;
		default:	type = "type?";
		}

		char bsize[BIN_FLENGTH];	// block size
		char create_uuid[UID_LENGTH], modify_uuid[UID_LENGTH],
			link_uuid[UID_LENGTH], parent_uuid[UID_LENGTH];

		fbins(vd->vdi.disksize, disksize);
		fbins(vd->vdi.blocksize, bsize);
		uid_str(&vd->vdi.uuidCreate, create_uuid, UID_UUID);
		uid_str(&vd->vdi.uuidModify, modify_uuid, UID_UUID);
		uid_str(&vd->vdi.uuidLinkage, link_uuid, UID_UUID);
		uid_str(&vd->vdi.uuidParentModify, parent_uuid, UID_UUID);

		printf(
		"VirtualBox VDI %s disk v%u.%u, %s\n"
		"Header size: %u, Flags: %XH, Dummy: %u\n"
		"Blocks: %u (allocated: %u, extra: %u), %s size\n"
		"Offset to data: %Xh, to alloc blocks: %Xh\n"
		"Cylinders: %u (legacy: %u)\n"
		"Heads: %u (legacy: %u)\n"
		"Sectors: %u (legacy: %u)\n"
		"Sector size: %u (legacy: %u)\n"
		"Create  UUID: %s\n"
		"Modifiy UUID: %s\n"
		"Linkage UUID: %s\n"
		"Parent  UUID: %s\n",
		type, vd->vdihdr.majorv, vd->vdihdr.minorv, disksize,
		vd->vdi.hdrsize, vd->vdi.fFlags, vd->vdi.u32Dummy,
		vd->vdi.totalblocks, vd->vdi.blocksalloc, vd->vdi.blocksextra, bsize,
		vd->vdi.offData, vd->vdi.offBlocks,
		vd->vdi.cCylinders, vd->vdi.LegacyGeometry.cCylinders,
		vd->vdi.cHeads, vd->vdi.LegacyGeometry.cHeads,
		vd->vdi.cSectors, vd->vdi.LegacyGeometry.cSectors,
		vd->vdi.cbSector, vd->vdi.LegacyGeometry.cbSector,
		create_uuid, modify_uuid, link_uuid, parent_uuid
		);
	}
		break;
	//
	// VMDK
	//
	case VDISK_FORMAT_VMDK: {
		char *comp; // compression
		//if (h.flags & COMPRESSED)
		switch (vd->vmdk.compressAlgorithm) {
		case 0:  comp = "no"; break;
		case 1:  comp = "DEFLATE"; break;
		default: comp = "?";
		}

		char size[BIN_FLENGTH];
		fbins(vd->capacity, size);
		printf(
		"VMware VMDK disk v%u, %s compression, %s\n"
		"\nCapacity: %"PRIu64" Sectors\n"
		"Overhead: %"PRIu64" Sectors\n"
		"Grain size (Raw): %"PRIu64" Sectors\n",
		vd->vmdk.version, comp, size,
		vd->vmdk.capacity, vd->vmdk.overHead, vd->vmdk.grainSize
		);

		if (vd->vmdk.uncleanShutdown)
			printf("+ Unclean shutdown");
	}
		break;
	//
	// VHD
	//
	case VDISK_FORMAT_VHD: {
		char sizecur[BIN_FLENGTH], uuid[UID_LENGTH];
		char *type, *os;

		switch (vd->vhd.type) {
		case VHD_DISK_FIXED:	type = "fixed"; break;
		case VHD_DISK_DYN:	type = "dynamic"; break;
		case VHD_DISK_DIFF:	type = "differencing"; break;
		default:
			type = vd->vhd.type <= 6 ? "reserved (deprecated)" : "unknown";
		}

		switch (vd->vhd.creator_os) {
		case VHD_OS_WIN:	os = "Windows"; break;
		case VHD_OS_MAC:	os = "macOS"; break;
		default:	os = "unknown"; break;
		}

		uid_str(&vd->vhd.uuid, uuid, UID_UUID);
		fbins(vd->vhd.size_current, sizecur);
		fbins(vd->vhd.size_original, disksize);
		printf(
		"Conectix/Microsoft VHD %s disk v%u.%u, %s/%s\n"
		"Created via %.4s v%u.%u on %s\n"
		"Cylinders: %u, Heads: %u, Sectors: %u\n"
		"CRC32: %08X, UUID: %s\n",
		type, vd->vhd.major, vd->vhd.minor, sizecur, disksize,
		vd->vhd.creator_app, vd->vhd.creator_major, vd->vhd.creator_minor, os,
		vd->vhd.cylinders, vd->vhd.heads, vd->vhd.sectors,
		vd->vhd.checksum,
		uuid
		);
		if (vd->vhd.type != VHD_DISK_FIXED) {
			char paruuid[UID_LENGTH];
			uid_str(&vd->vhddyn.parent_uuid, paruuid, UID_UUID);
			printf(
			"Dynamic header v%u.%u, data: %" PRIu64 ", table: %" PRIu64 "\n"
			"Blocksize: %u, checksum: %08X\n"
			"Parent UUID: %s, Parent timestamp: %u\n"
			"%u BAT Entries, %u maximum BAT entries\n",
			vd->vhddyn.minor, vd->vhddyn.major,
			vd->vhddyn.data_offset, vd->vhddyn.table_offset,
			vd->vhddyn.blocksize, vd->vhddyn.checksum,
			paruuid, vd->vhddyn.parent_timestamp,
			vd->u32nblocks, vd->vhddyn.max_entries
			);
		}
		if (vd->vhd.savedState)
			puts("+ Saved state");
	}
		break;
	case VDISK_FORMAT_RAW: break; // No header info
	default:
		fputs("vvd_info: Format not supported\n", stderr);
		return VVD_EVDFORMAT;
	}

	//
	// MBR detection
	//

	MBR mbr;
	if (vdisk_read_lba(vd, &mbr, 0)) return VVD_EVDOK;
	if (mbr.sig != MBR_SIG) return VVD_EVDOK;
	mbr_info_stdout(&mbr);

	//
	// Extended MBR detection (EBR)
	//

	int ebrtype = 0;
	for (int i = 0; i < 4; ++i) {
		// EFI GPT Protective or EFI System Partition
		if (mbr.pe[i].parttype == 0xEE || mbr.pe[i].parttype == 0xEF) {
			ebrtype = 1;
			break;
		}
	}

	switch (ebrtype) {
	//
	// GPT detection
	//
	case 1: {
		uint64_t gpt_lba;
		// Start of disk
		if (vdisk_read_lba(vd, &mbr, 1)) return VVD_EVDOK;
		if (((GPT*)&mbr)->sig == EFI_SIG) {
			gpt_lba = 2;
			goto L_GPT_RDY;
		}
		// End of disk
		gpt_lba = BYTE_TO_SECTOR(vd->capacity) - 1;
		if (vdisk_read_lba(vd, &mbr, gpt_lba)) return VVD_EVDOK;
		if (((GPT*)&mbr)->sig == EFI_SIG) {
			gpt_lba -= 128;
			goto L_GPT_RDY;
		}
		break;
L_GPT_RDY:
		gpt_info_stdout((GPT *)&mbr);
		gpt_info_entries_stdout(vd, (GPT *)&mbr, gpt_lba);
	}
		break;
	}

	return VVD_EVDOK;
}

//
// vvd_map
//

int vvd_map(VDISK *vd, uint32_t flags) {
	char bsizestr[BIN_FLENGTH]; // If used
	uint64_t bsize; // block size
	uint32_t bcount; // block count
	uint32_t *b = vd->u32blocks; // block pointer

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		bcount = vd->vdi.totalblocks;
		bsize = vd->vdi.blocksize;
		break;
	case VDISK_FORMAT_VHD:
		if (vd->vhd.type != VHD_DISK_DYN) {
			fputs("vvd_map: vdisk is not dynamic\n", stderr);
			return VVD_EVDTYPE;
		}
		bcount = vd->u32nblocks;
		bsize = vd->vhddyn.blocksize;
		break;
	default:
		fputs("vvd_map: Unsupported format\n", stderr);
		return VVD_EVDFORMAT;
	}

	fbins(bsize, bsizestr);
	printf(
	"Allocation map: %u blocks of %s each\n"
	"   offset |        0 |        1 |        2 |        3 |"
	           "        4 |        5 |        6 |        7 |\n"
	"----------+----------+----------+----------+----------+"
	           "----------+----------+----------+----------+\n",
	bcount, bsizestr
	);
	size_t i = 0;
	size_t bn = bcount - 8;
	for (; i < bn; i += 8) {
		printf(
		" %8zu | %8X | %8X | %8X | %8X | %8X | %8X | %8X | %8X |\n",
		i,
		b[i],     b[i + 1],
		b[i + 2], b[i + 3],
		b[i + 4], b[i + 5],
		b[i + 6], b[i + 7]
		);
	}
	if (bcount - i > 0) { // Left over
		printf(" %8zu |", i);
		for (; i < bcount; ++i)
			printf(" %8X |", b[i]);
		putchar('\n');
	}
	return VVD_EVDOK;
}

//
// vvd_new
//

int vvd_new(VDISK *vd, uint64_t vsize) {
	uint8_t *buffer;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		vd->u32nblocks = vsize / vd->vdi.blocksize;
		if (vd->u32nblocks == 0)
			vd->u32nblocks = 1;
		uint32_t bsize = vd->u32nblocks << 2;
		if ((vd->u32blocks = malloc(bsize)) == NULL)
			return VVD_EVDALLOC;
		vd->vdi.totalblocks = vd->u32nblocks;
		vd->vdi.disksize = vsize;
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
			fputs("vvd_new: Type currently unsupported", stderr);
			return VVD_EVDTYPE;
		}
		break;
	default:
		fputs("vvd_new: Format currently unsupported", stderr);
		return VVD_EVDFORMAT;
	}

L_DISKEMPTY:
	if (vdisk_update(vd)) {
		fputs("vvd_new: Could not write headers\n", stderr);
		return 1;
	}
	return 0;
}

//
// vvd_compact
//

int vvd_compact(VDISK *vd) {
	if (vd->u32nblocks == 0) {
		fputs("vvd_compact: No allocated blocks\n", stderr);
		return VVD_EVDMISC;
	}

	switch (vd->format) {
	//
	// VDI
	//
	case VDISK_FORMAT_VDI:
		if (vd->vdi.type != VDI_DISK_DYN) {
			fputs("vdi_compact: vdisk not dynamic\n", stderr);
			return VVD_EVDTYPE;
		}

		uint8_t *buffer;	// Block buffer

		//
		// Block buffer for transfer
		//

		if ((buffer = malloc(vd->vdi.blocksize)) == NULL)
			return VVD_EVDALLOC;

		//
		// Temporary VDISK
		//
		// Also assigns the same attributes from the source vdisk
		//

		VDISK vdtmp;
		if ((vdtmp.u32blocks = malloc(vd->u32nblocks << 2)) == NULL)
			return VVD_EVDALLOC;
		vdtmp.offset = vd->offset;
		vdtmp.format = vd->format;
		vdtmp.u32nblocks = vd->u32nblocks;
		for (size_t i = 0; i < vdtmp.u32nblocks; ++i)
			vdtmp.u32blocks[i] = VDI_BLOCK_UNALLOCATED;
		memcpy(&vdtmp.vdihdr, &vd->vdihdr, sizeof(VDI_HDR));
		memcpy(&vdtmp.vdi, &vd->vdi, sizeof(VDIHEADER1));
		if (vdisk_open(NULL, &vdtmp, VDISK_CREATE_TEMP)) {
			vdisk_perror(__func__);
			return vdisk_errno;
		}
		printf("vdi_compact: %s disk created\n", vdisk_str(&vdtmp));

		//TODO: Continue vvd_compact::vdi

		// "Optimized" buffer size
		size_t oblocksize = vd->vdi.blocksize / sizeof(size_t);
		// "Optimized" buffer pointer
		size_t *obuffer = (size_t*)buffer;
		uint32_t stat_unalloc = 0;	// unallocated blocks
		uint32_t stat_occupied = 0;	// allocated blocks with data
		uint32_t stat_zero = 0;	// blocks with no data inside
		uint32_t stat_alloc = 0;	// allocated blocks
		char strbsize[BIN_FLENGTH];
		fbins(vd->vdi.blocksize, strbsize);
		printf("vdi_compact: Writing (%s blocks, %u checks/%u bytes)...\n",
			strbsize, (uint32_t)oblocksize, (uint32_t)sizeof(size_t));
		uint64_t d = 0;	// disk block index
		os_seek(vd->fd, vd->offset, SEEK_SET);
		for (size_t i = 0; i < vd->u32nblocks; ++i) {
			if (vd->u32blocks[i] == VDI_BLOCK_UNALLOCATED ||
				vd->u32blocks[i] == VDI_BLOCK_FREE) {
				vdtmp.u32blocks[i] = VDI_BLOCK_UNALLOCATED;
				++stat_unalloc;
				continue;
			}
			int re = vdisk_read_block(vd, buffer, i);
			if (re == VVD_EVDREAD || re == VVD_EVDSEEK) {
				fprintf(stderr, "vdi_compact: Couldn't %s disk\n",
					re == VVD_EVDSEEK ? "seek" : "read");
				return re;
			}
			// Check if block has data, if so, write the block into tmp VDISK
			++stat_alloc;
			for (size_t b = 0; b < oblocksize; ++b) {
				if (obuffer[b])
					goto L_HASDATA;
			}
			++stat_zero;
			continue;
L_HASDATA:
			//if (vdisk_write_block_at(&vdtmp, buffer, i, d)) {
			//	fputs("vdi_compact: Couldn't write to disk\n", stderr);
			//	return VVD_EVDWRITE;
			//}
			vdtmp.u32blocks[i] = i;
			os_write(vdtmp.fd, buffer, vd->vdi.blocksize);
			++stat_occupied;
			++d;
		}
		vdtmp.vdi.blocksalloc = stat_occupied;
		vdisk_update(&vdtmp);
		printf(
			"vdi_compact: %u/%u blocks written, %u unallocated, %u zero, %u total\n",
			stat_occupied, stat_alloc, stat_unalloc, stat_zero, vd->u32nblocks
		);
		return 0;
	//
	// RAW
	//
	case VDISK_FORMAT_RAW:
		fputs("vvd_compact: RAW files/devices are not supported\n",
			stderr);
		return VVD_EVDFORMAT;
	default:
		fputs("vvd_compact: VDISK format not supported\n",
			stderr);
		return VVD_EVDFORMAT;
	}

	return 0;
}
