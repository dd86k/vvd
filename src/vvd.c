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

int vvd_info(VDISK *vd, uint32_t flags) {
	const char *type;	// vdisk type
	char dsize[BIN_FLENGTH], bsize[BIN_FLENGTH];	// disk and block size
	char uid1[UID_LENGTH], uid2[UID_LENGTH], uid3[UID_LENGTH], uid4[UID_LENGTH];

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

		char create_uuid[UID_LENGTH], modify_uuid[UID_LENGTH],
			link_uuid[UID_LENGTH], parent_uuid[UID_LENGTH];

		fbins(vd->vdi.disksize, dsize);
		fbins(vd->vdi.blocksize, bsize);
		uid_str(&vd->vdi.uuidCreate, uid1, UID_UUID);
		uid_str(&vd->vdi.uuidModify, uid2, UID_UUID);
		uid_str(&vd->vdi.uuidLinkage, uid3, UID_UUID);
		uid_str(&vd->vdi.uuidParentModify, uid4, UID_UUID);

		printf(
		"VirtualBox VDI %s disk v%u.%u, %s\n"
		"Header size: %u, Flags: %XH, Dummy: %u\n"
		"Blocks: %u (allocated: %u, extra: %u), %s size\n"
		"Offset to data: %Xh, to allocation table: %Xh\n"
		"Cylinders: %u (legacy: %u), Heads: %u (legacy: %u), Sectors: %u (legacy: %u)\n"
		"Sector size: %u (legacy: %u)\n"
		"Create UUID: %s\n"
		"Modify UUID: %s\n"
		"Link   UUID: %s\n"
		"Parent UUID: %s\n",
		type, vd->vdihdr.majorv, vd->vdihdr.minorv, dsize,
		vd->vdi.hdrsize, vd->vdi.fFlags, vd->vdi.u32Dummy,
		vd->vdi.totalblocks, vd->vdi.blocksalloc, vd->vdi.blocksextra, bsize,
		vd->vdi.offData, vd->vdi.offBlocks,
		vd->vdi.cCylinders, vd->vdi.LegacyGeometry.cCylinders,
		vd->vdi.cHeads, vd->vdi.LegacyGeometry.cHeads,
		vd->vdi.cSectors, vd->vdi.LegacyGeometry.cSectors,
		vd->vdi.cbSector, vd->vdi.LegacyGeometry.cbSector,
		uid1, uid2, uid3, uid4
		);
	}
		break;
	//
	// VMDK
	//
	case VDISK_FORMAT_VMDK: {
		const char *comp; // compression
		//if (h.flags & COMPRESSED)
		switch (vd->vmdk.compressAlgorithm) {
		case 0:  comp = "no"; break;
		case 1:  comp = "DEFLATE"; break;
		default: comp = "?";
		}

		fbins(vd->capacity, dsize);
		printf(
		"VMware VMDK disk v%u, %s compression, %s\n"
		"Capacity: %"PRIu64" Sectors\n"
		"Overhead: %"PRIu64" Sectors\n"
		"Grain size (Raw): %"PRIu64" Sectors\n",
		vd->vmdk.version, comp, dsize,
		vd->vmdk.capacity, vd->vmdk.overHead, vd->vmdk.grainSize
		);

		if (vd->vmdk.uncleanShutdown)
			puts("+ Unclean shutdown");
	}
		break;
	//
	// VHD
	//
	case VDISK_FORMAT_VHD: {
		char sizecur[BIN_FLENGTH];
		const char *os;

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

		uid_str(&vd->vhd.uuid, uid1, UID_UUID);
		fbins(vd->vhd.size_current, sizecur);
		fbins(vd->vhd.size_original, dsize);

		printf(
		"Conectix/Microsoft VHD %s disk v%u.%u, %s/%s\n"
		"Created via %.4s v%u.%u on %s\n"
		"Cylinders: %u, Heads: %u, Sectors: %u\n"
		"CRC32: %08X, UUID: %s\n",
		type, vd->vhd.major, vd->vhd.minor, sizecur, dsize,
		vd->vhd.creator_app, vd->vhd.creator_major, vd->vhd.creator_minor, os,
		vd->vhd.cylinders, vd->vhd.heads, vd->vhd.sectors,
		vd->vhd.checksum,
		uid1
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
	if (vdisk_read_lba(vd, &mbr, 0)) return EXIT_SUCCESS;
	if (mbr.sig != MBR_SIG) return EXIT_SUCCESS;
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
		if (vdisk_read_lba(vd, &mbr, 1)) return VVD_EOK;
		if (((GPT*)&mbr)->sig == EFI_SIG) {
			gpt_lba = 2;
			goto L_GPT_RDY;
		}
		// End of disk
		gpt_lba = BYTE_TO_SECTOR(vd->capacity) - 1;
		if (vdisk_read_lba(vd, &mbr, gpt_lba)) return VVD_EOK;
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

	return VVD_EOK;
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
		fputs("vvd_map: unsupported format\n", stderr);
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
	return VVD_EOK;
}

//
// vvd_new
//

int vvd_new(const _oschar *path, uint32_t format, uint64_t capacity, uint32_t flags) {
	VDISK vd;
	if (vdisk_create(&vd, path, format, capacity, flags)) {
		vdisk_perror(&vd);
		return vd.errcode;
	}
	printf("vvd_new: %s disk created successfully\n", vdisk_str(&vd));
	return 0;
}

//
// vvd_compact
//

int vvd_compact(VDISK *vd, uint32_t flags) {
	if (vd->u32nblocks == 0) {
		fputs("vvd_compact: no allocated blocks\n", stderr);
		return VVD_EVDMISC;
	}

	puts("vvd_compact: [warning] This function is still being developed");

	struct progress_t progress;
	switch (vd->format) {
	//
	// VDI
	//
	case VDISK_FORMAT_VDI: {
		//TODO: Continue vvd_compact::vdi

		if (vd->vdi.type != VDI_DISK_DYN) {
			fputs("vvd_compact: vdisk not dynamic\n", stderr);
			return VVD_EVDTYPE;
		}

		uint8_t *buffer;	// Block buffer

		//
		// Block buffer for transfer
		//

		if ((buffer = malloc(vd->vdi.blocksize)) == NULL)
			return VVD_EALLOC;

		//
		// Temporary VDISK
		//
		// Also assigns the same attributes from the source vdisk
		//

		VDISK vdtmp;
		if ((vdtmp.u32blocks = malloc(vd->u32nblocks * 4)) == NULL)
			return VVD_EALLOC;
		vdtmp.offset = vd->offset;
		vdtmp.format = vd->format;
		vdtmp.u32nblocks = vd->u32nblocks;
		for (size_t i = 0; i < vdtmp.u32nblocks; ++i)
			vdtmp.u32blocks[i] = VDI_BLOCK_UNALLOC;
		memcpy(&vdtmp.vdihdr, &vd->vdihdr, sizeof(VDI_HDR));
		memcpy(&vdtmp.vdi, &vd->vdi, sizeof(VDIHEADER1));
		if (vdisk_create(&vdtmp, NULL, VDISK_FORMAT_VDI, vd->capacity, VDISK_CREATE_TEMP)) {
			vdisk_perror(&vdtmp);
			return vd->errcode;
		}
		printf("vvd_compact: %s temporary disk created\n", vdisk_str(&vdtmp));

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
		printf("vvd_compact: writing (%s/block, %u checks/%u bytes)...\n",
			strbsize, (uint32_t)oblocksize, (uint32_t)sizeof(size_t));
		os_pinit(&progress, PROG_MODE_POURCENT, vd->u32nblocks - 1);

		uint64_t d = 0;	// disk block index
		os_fseek(vd->fd, vd->offset, SEEK_SET);
		for (uint32_t i = 0; i < vd->u32nblocks; ++i) {
			os_pupdate(&progress, (uint32_t)i);
			uint32_t bi = vd->u32blocks[i]; // block index
			if (bi == VDI_BLOCK_UNALLOC || bi == VDI_BLOCK_FREE) {
				vdtmp.u32blocks[i] = VDI_BLOCK_UNALLOC;
				++stat_unalloc;
				continue;
			}
			if (vdisk_read_block(vd, buffer, i)) {
				vdisk_perror(vd);
				return vd->errcode;
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
			if (vdisk_write_block_at(&vdtmp, buffer, i, d)) {
				vdisk_perror(&vdtmp);
				return vdtmp.errcode;
			}
			++stat_occupied;
			++d;
		}
		vdtmp.vdi.blocksalloc = stat_occupied;
		if (vdisk_update(&vdtmp)) {
			vdisk_perror(&vdtmp);
			return vdtmp.errcode;
		}
		os_pfinish(&progress);
		printf("vvd_compact: %u/%u blocks written, %u unallocated, %u zero, %u total\n",
			stat_occupied, stat_alloc, stat_unalloc, stat_zero, vd->u32nblocks
		);
		return 0;
	}
	//
	// RAW
	//
	case VDISK_FORMAT_RAW:
		fputs("vvd_compact: RAW files/devices are not supported\n", stderr);
		return VVD_EVDFORMAT;
	default:
		fputs("vvd_compact: VDISK format not supported\n", stderr);
		return VVD_EVDFORMAT;
	}

	return 0;
}
