/**
 * Main vvd operations
 */

#include <assert.h>
#include <string.h> // memcpy
#include <inttypes.h>
#include "vvd.h"
#include "utils/bin.h"
#include "fs/mbr.h"
#include "fs/gpt.h"

//TODO: Consider a "log" module

//
// Global variables
//
// Since only one instance of any vvd_* is running at a time (seeing that
// they're directly invoked from main()), these variables are useful for
// callbacks to avoid over-implementing the vdisk back-end.
//

// CLI progress bar var
struct progress_t g_progress;
//
uint32_t g_flags;

//
// vvd_perror
//

#if _WIN32
	#define ERRFMT "%08X"
#else
	#define ERRFMT "%d"
#endif

void vvd_perror(VDISK *vd) {
	fprintf(stderr, "%s@%u: (" ERRFMT ") %s\n",
		vd->err.func, vd->err.line, vd->err.num, vdisk_error(vd));
}

//
// Callback functions
//

// 
void vvd_cb_vdisk_created(VDISK *disk) {
	printf("Disk type %s created\n", vdisk_str(disk));
}

//
void vvd_cb_current_blocks(uint64_t total) {
	if (g_flags & VVD_PROGRESS)
	if (os_pupdate(&g_progress, 0)) {
		fputs("os_pinit: Could not update progress bar\n", stderr);
		exit(1);
	}
}

//
void vvd_cb_total_blocks(uint64_t total) {
	if (g_flags & VVD_PROGRESS)
	if (os_pinit(&g_progress, PROG_MODE_POURCENT, 0)) {
		fputs("os_pinit: Could not init progress bar\n", stderr);
		exit(1);
	}
}

//
void vvd_cb_done() {
	if (g_flags & VVD_PROGRESS)
	if (os_pfinish(&g_progress)) {
		fputs("os_pfinish: Could not finish progress bar", stderr);
		exit(1);
	}
}

//
// vvd_info_mbr
//

void vvd_info_mbr(MBR *mbr, uint32_t flags) {
	char strsize[BINSTR_LENGTH];
	uint64_t totalsize = SECTOR_TO_BYTE(
		(uint64_t)mbr->pe[0].sectors + (uint64_t)mbr->pe[1].sectors +
		(uint64_t)mbr->pe[2].sectors + (uint64_t)mbr->pe[3].sectors
	);

	bintostr(strsize, totalsize);

	if (flags & VVD_INFO_RAW) {
		printf(
		"disklabel          : MBR\n"
		"serial             : 0x%08X\n"
		"type               : 0x%04X\n",
		mbr->serial, mbr->type
		);
		for (unsigned int i = 0; i < 4; ++i) {
			MBR_PARTITION pe = mbr->pe[i];
			printf(
			"\n"
			"partition          : %u\n"
			"status             : 0x%02X\n"
			"type               : 0x%02X\n"
			"lba                : %u\n"
			"length             : %u sectors\n"
			"chs start          : %u/%u/%u\n"
			"chs end            : %u/%u/%u\n",
			i + 1,
			pe.status,
			pe.type,
			pe.lba,
			pe.sectors,
			// CHS start
			pe.chsfirst.cylinder | ((pe.chsfirst.sector & 0xC0) << 2),
			pe.chsfirst.head, pe.chsfirst.sector & 0x3F,
			// CHS end
			pe.chslast.cylinder | ((pe.chslast.sector & 0xC0) << 2),
			pe.chslast.head, pe.chslast.sector & 0x3F
			);
		}
	} else {
		printf(
		"MBR (DOS) disklabel, %s used\n"
		"   Boot     Start        Size  Id  Type\n", strsize
		);
		for (unsigned int i = 0; i < 4; ++i) {
			MBR_PARTITION pe = mbr->pe[i];
			bintostr(strsize, SECTOR_TO_BYTE(pe.sectors));
			printf(
			"%u. %c  %11u  %10s  %2x  %s\n",
			i + 1,
			pe.status >= 0x80 ? '*' : ' ',
			pe.lba,
			strsize,
			pe.type,
			mbr_part_type_str(pe.type)
			);
		}
	}
}

//
// vvd_info_gpt
//

void vvd_info_gpt(GPT *gpt, uint32_t flags) {
	char gptsize[BINSTR_LENGTH];
	UID_TEXT diskguid;

	uid_str(diskguid, &gpt->guid, UID_GUID);

	if (flags & VVD_INFO_RAW) {
		printf(
		"\n"
		"disklabel          : GPT\n"
		"version            : %u.%u\n"
		"header size        : %u\n"
		"header crc32       : 0x%08X\n"
		"part. table crc32  : 0x%08X\n"
		"header lba         : %" PRIu64 "\n"
		"backup lba         : %" PRIu64 "\n"
		"lba start          : %" PRIu64 "\n"
		"lba end            : %" PRIu64 "\n"
		"disk guid          : %s\n"
		"part. table lba    : %" PRIu64 "\n"
		"maximum entries    : %u\n"
		"entry size         : %u\n",
		gpt->majorver, gpt->minorver,
		gpt->headersize, gpt->headercrc32,
		gpt->pt_crc32,
		gpt->current.lba, gpt->backup.lba,
		gpt->first.lba, gpt->last.lba,
		diskguid,
		gpt->pt_location.lba, gpt->pt_entries, gpt->pt_esize
		);
	} else {
		bintostr(gptsize, SECTOR_TO_BYTE(gpt->last.lba - gpt->first.lba));

		printf(
		"\n"
		"GPT extended disklabel v%u.%u, %s used\n"
		"Disk GUID: %s\n",
		gpt->majorver, gpt->minorver, gptsize,
		diskguid
		);
	}
}

//
// vvd_info_gpt_entries
//

void vvd_info_gpt_entries(VDISK *vd, GPT *gpt, uint64_t lba, uint32_t flags) {
	char partname[EFI_PART_NAME_LENGTH];
	char partsize[BINSTR_LENGTH];
	UID_TEXT partguid, typeguid;
	GPT_ENTRY entry; // GPT entry
	uint32_t entrynum = 1;
	uint32_t f_bkpgpt = flags & VVD_INTERNAL_GPT_BKP;

	if ((flags & VVD_INFO_RAW) == 0)
		puts("Part         Start        Size  Type");

START:
	if (vdisk_read_sector(vd, &entry, lba)) {
		fputs("vvd_info_gpt_entries: Could not read GPT_ENTRY", stderr);
		return;
	}

	if (uid_nil(&entry.type))
		return;

	uid_str(typeguid, &entry.type, UID_GUID);
	uid_str(partguid, &entry.part, UID_GUID);
	int wr = wstra(partname, entry.partname, EFI_PART_NAME_LENGTH);

	if (flags & VVD_INFO_RAW) {
		printf(
		"\n"
		"partition          : %u\n"
		"name               : %-36s\n"
		"part guid          : %s\n"
		"type guid          : %s\n"
		"lba start          : %" PRIu64 "\n"
		"lba end            : %" PRIu64 "\n"
		"flags              : 0x%016" PRIX64 "\n",
		entrynum,
		wr > 0 ? partname : "",
		partguid,
		typeguid,
		entry.first.lba,
		entry.last.lba,
		entry.flags
		);
	} else {
		bintostr(partsize, SECTOR_TO_BYTE(entry.last.lba - entry.first.lba));
		const char *gpt_type = gpt_part_type_str(&entry.type);

		//TODO: GPT partition type (after name)
		printf(
		"%4u. %12" PRIu64 "%12s  %s\n",
		entrynum, entry.first.lba, partsize, gpt_type ? gpt_type : "Unknown"
		);

		if (wr > 0)
			printf("      Name: %-36s", partname);

		// GPT flags
		if (entry.flags & GPT_FLAG_PLATFORM_REQUIRED)
			puts("      + Platform required");
		if (entry.flags & GPT_FLAG_EFI_FIRMWARE_IGNORE)
			puts("      + Firmware ignore");
		if (entry.flags & GPT_FLAG_LEGACY_BIOS_BOOTABLE)
			puts("      + Legacy BIOS bootable");

		// Google flags
		if (entry.flags & GPT_FLAG_SUCCESSFUL_BOOT)
			puts("      + (Google) Successful boot");

		// Microsoft flags
		if (entry.flags & GPT_FLAG_READ_ONLY)
			puts("      + (Microsoft) GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY");
		if (entry.flags & GPT_FLAG_SHADOW_COPY)
			puts("      + (Microsoft) GPT_BASIC_DATA_ATTRIBUTE_SHADOW_COPY");
		if (entry.flags & GPT_FLAG_HIDDEN)
			puts("      + (Microsoft) GPT_BASIC_DATA_ATTRIBUTE_HIDDEN");
		if (entry.flags & GPT_FLAG_NO_DRIVE_LETTER)
			puts("      + (Microsoft) GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER");
	}

	if (entrynum > gpt->pt_entries)
		return;
	if (f_bkpgpt) --lba; else ++lba;

	++entrynum;
	goto START;
}

//
// vvd_info
//

int vvd_info(VDISK *vd, uint32_t flags) {
	const char *type;	// vdisk type
	char disksize[BINSTR_LENGTH], blocksize[BINSTR_LENGTH];
	char uid1[UID_BUFFER_LENGTH], uid2[UID_BUFFER_LENGTH],
		uid3[UID_BUFFER_LENGTH], uid4[UID_BUFFER_LENGTH];

	switch (vd->format) {
	//
	// VDI
	//
	case VDISK_FORMAT_VDI:
		switch (vd->vdi->v1.type) {
		case VDI_DISK_DYN:	type = "dynamic"; break;
		case VDI_DISK_FIXED:	type = "fixed"; break;
		case VDI_DISK_UNDO:	type = "undo"; break;
		case VDI_DISK_DIFF:	type = "diff"; break;
		default:	type = "type?";
		}

		bintostr(disksize, vd->vdi->v1.capacity);

		if (flags & VVD_INFO_RAW) {
			char create_uuid[UID_BUFFER_LENGTH], modify_uuid[UID_BUFFER_LENGTH],
				link_uuid[UID_BUFFER_LENGTH], parent_uuid[UID_BUFFER_LENGTH];

			bintostr(blocksize, vd->vdi->v1.blk_size);
			uid_str(uid1, &vd->vdi->v1.uuidCreate, UID_UUID);
			uid_str(uid2, &vd->vdi->v1.uuidModify, UID_UUID);
			uid_str(uid3, &vd->vdi->v1.uuidLinkage, UID_UUID);
			uid_str(uid4, &vd->vdi->v1.uuidParentModify, UID_UUID);

			printf(
			"disk format        : VDI\n"
			"version            : %u.%u\n"
			"type               : %s\n"
			"capacity           : %s (%"PRIu64")\n"
			"header size        : %u\n"
			"flags              : 0x%08X\n"
			"dummy              : 0x%08X\n"
			"block size         : %s (%u)\n"
			"blocks total       : %u\n"
			"blocks alloc       : %u\n"
			"blocks extra       : %u\n"
			"offset table       : %u\n"
			"offset data        : %u\n"
			"chs                : %u/%u/%u\n"
			"chs legacy         : %u/%u/%u\n"
			"sector size        : %u\n"
			"sector size legacy : %u\n"
			"create uuid        : %s\n"
			"modify uuid        : %s\n"
			"link uuid          : %s\n"
			"parent uuid        : %s\n",
			vd->vdi->hdr.majorver, vd->vdi->hdr.minorver,
			type,
			disksize, vd->vdi->v1.capacity,
			vd->vdi->v1.hdrsize,
			vd->vdi->v1.fFlags,
			vd->vdi->v1.u32Dummy,
			blocksize, vd->vdi->v1.blk_size,
			vd->vdi->v1.blk_total,
			vd->vdi->v1.blk_alloc,
			vd->vdi->v1.blk_extra,
			vd->vdi->v1.offBlocks,
			vd->vdi->v1.offData,
			vd->vdi->v1.cCylinders,
			vd->vdi->v1.cHeads,
			vd->vdi->v1.cSectors,
			vd->vdi->v1.LegacyGeometry.cCylinders,
			vd->vdi->v1.LegacyGeometry.cHeads,
			vd->vdi->v1.LegacyGeometry.cSectors,
			vd->vdi->v1.cbSector,
			vd->vdi->v1.LegacyGeometry.cbSector,
			uid1, uid2, uid3, uid4
			);
		} else {
			printf(
			"VirtualBox VDI %s disk v%u.%u, %s\n",
			type, vd->vdi->hdr.majorver, vd->vdi->hdr.minorver, disksize
			);
			//TODO: Interpret flags
		}
		break;
	//
	// VMDK
	//
	case VDISK_FORMAT_VMDK: {
		const char *comp; // compression
		//if (h.flags & COMPRESSED)
		switch (vd->vmdk->hdr.compressAlgorithm) {
		case 0:  comp = "no"; break;
		case 1:  comp = "DEFLATE"; break;
		default: comp = "?";
		}

		if (flags & VVD_INFO_RAW) {
			printf(
			"disk format        : VMDK\n"
			"version            : %u\n"
			"flags              : 0x%08X\n"
			"capacity           : %" PRIu64 " sectors\n"
			"overhead           : %" PRIu64 " sectors\n"
			"grain size         : %" PRIu64 " sectors\n"
			"descriptor offset  : %" PRIu64 " sectors\n"
			"descriptor size    : %" PRIu64 " sectors\n"
			"table entries      : %u\n"
			"backup l0 offset   : %" PRIu64 " sectors\n"
			"l0 offset          : %" PRIu64 " sectors\n"
			"overhead           : %" PRIu64 " sectors\n"
			"unclean shutdown   : %u\n"
			"single nl char     : 0x%02X\n"
			"non-end line char  : 0x%02X\n"
			"double nl char 1   : 0x%02X\n"
			"double nl char 2   : 0x%02X\n"
			"compression        : 0x%02X\n",
			vd->vmdk->hdr.version,
			vd->vmdk->hdr.flags,
			vd->vmdk->hdr.capacity,
			vd->vmdk->hdr.overHead,
			vd->vmdk->hdr.grainSize,
			vd->vmdk->hdr.descriptorOffset,
			vd->vmdk->hdr.descriptorSize,
			vd->vmdk->hdr.numGTEsPerGT,
			vd->vmdk->hdr.rgdOffset,
			vd->vmdk->hdr.gdOffset,
			vd->vmdk->hdr.overHead,
			vd->vmdk->hdr.uncleanShutdown,
			vd->vmdk->hdr.singleEndLineChar,
			vd->vmdk->hdr.nonEndLineChar,
			vd->vmdk->hdr.doubleEndLineChar1,
			vd->vmdk->hdr.doubleEndLineChar2,
			vd->vmdk->hdr.compressAlgorithm
			);
		} else {
			bintostr(disksize, vd->capacity);
			printf(
			"VMware VMDK disk v%u, %s compression, %s\n",
			vd->vmdk->hdr.version, comp, disksize
			);

			if (vd->vmdk->hdr.flags & VMDK_F_VALID_NL)
				puts("+ Valid newline detection");
			if (vd->vmdk->hdr.flags & VMDK_F_REDUNDANT_TABLE)
				puts("+ Redundant grain table used");
			if (vd->vmdk->hdr.flags & VMDK_F_ZEROED_GTE)
				puts("+ Zeroed-grain GTE used");
			if (vd->vmdk->hdr.flags & VDMK_F_COMPRESSED)
				puts("+ Grains are compressed");
			if (vd->vmdk->hdr.flags & VMDK_F_MARKERS)
				puts("+ Markers used");
			if (vd->vmdk->hdr.uncleanShutdown)
				puts("+ Unclean shutdown");
		}
	}
		break;
	//
	// VHD
	//
	case VDISK_FORMAT_VHD: {
		char sizecur[BINSTR_LENGTH];
		const char *byos;

		switch (vd->vhd->hdr.type) {
		case VHD_DISK_FIXED:	type = "fixed"; break;
		case VHD_DISK_DYN:	type = "dynamic"; break;
		case VHD_DISK_DIFF:	type = "differencing"; break;
		default:
			type = vd->vhd->hdr.type <= 6 ? "reserved (deprecated)" : "unknown";
		}

		switch (vd->vhd->hdr.creator_os) {
		case VHD_OS_WIN:	byos = "Windows"; break;
		case VHD_OS_MAC:	byos = "macOS"; break;
		default:	byos = "unknown"; break;
		}

		uid_str(uid1, &vd->vhd->hdr.uuid, UID_ASIS);
		str_s(vd->vhd->hdr.creator_app, 4);

		if (flags & VVD_INFO_RAW) {
			printf(
			"disk format        : VHD\n"
			"version            : %u.%u\n"
			"type               : %s\n"
			"current size       : %" PRIu64 "\n"
			"capacity           : %" PRIu64 "\n"
			"created by         : %.4s\n"
			"created by ver.    : %u.%u\n"
			"created on os      : %s\n"
			"chs                : %u/%u/%u\n"
			"checksum           : 0x%08X\n"
			"uuid               : %s\n",
			vd->vhd->hdr.major, vd->vhd->hdr.minor,
			type,
			vd->vhd->hdr.size_current,
			vd->vhd->hdr.size_original,
			vd->vhd->hdr.creator_app,
			vd->vhd->hdr.creator_major, vd->vhd->hdr.creator_minor,
			byos,
			vd->vhd->hdr.cylinders, vd->vhd->hdr.heads, vd->vhd->hdr.sectors,
			vd->vhd->hdr.checksum,
			uid1
			);
			if (vd->vhd->hdr.type != VHD_DISK_FIXED) {
				char paruuid[UID_BUFFER_LENGTH];
				uid_str(paruuid, &vd->vhd->dyn.parent_uuid, UID_ASIS);
				printf(
				"dyn. header ver.   : %u.%u\n"
				"offset table       : %" PRIu64 "\n"
				"offset data        : %" PRIu64 "\n"
				"block size         : %u\n"
				"dyn. checksum      : 0x%08X\n"
				"parent uuid        : %s\n"
				"parent timestamp   : %u\n"
				"bat entries        : %u\n",
				vd->vhd->dyn.minor, vd->vhd->dyn.major,
				vd->vhd->dyn.table_offset,
				vd->vhd->dyn.data_offset,
				vd->vhd->dyn.blocksize,
				vd->vhd->dyn.checksum,
				paruuid,
				vd->vhd->dyn.parent_timestamp,
				vd->vhd->dyn.max_entries
				);
			}
		} else {
			bintostr(sizecur, vd->vhd->hdr.size_current);
			bintostr(disksize, vd->vhd->hdr.size_original);

			printf(
			"Connectix/Microsoft VHD %s disk v%u.%u, %s/%s\n",
			type, vd->vhd->hdr.major, vd->vhd->hdr.minor, sizecur, disksize
			);
		}

		if (vd->vhd->hdr.savedState)
			puts("+ Saved state");
	}
		break;
//	case VDISK_FORMAT_VHDX:
	case VDISK_FORMAT_QED:
		if (flags & VVD_INFO_RAW) {
			printf(
			"disk format        : QED\n"
			"cluster size       : %u\n"
			"table size         : %u\n"
			"header size        : %u\n"
			"features           : 0x%" PRIX64 "\n"
			"compact features   : 0x%" PRIX64 "\n"
			"autoclear features : 0x%" PRIX64 "\n"
			"L1 offset          : 0x%" PRIX64 "\n",
			vd->qed->hdr.cluster_size,
			vd->qed->hdr.table_size,
			vd->qed->hdr.header_size,
			vd->qed->hdr.features,
			vd->qed->hdr.compat_features,
			vd->qed->hdr.autoclear_features,
			vd->qed->hdr.l1_offset
			);
			if (vd->qed->hdr.features & QED_F_BACKING_FILE) {
				printf(
				"back. name offset  : %u\n"
				"back. name size    : %u\n",
				vd->qed->hdr.backup_name_offset,
				vd->qed->hdr.backup_name_size
				);
			}
		} else {
			bintostr(disksize, vd->capacity);
			printf("QEMU Enhanced Disk, %s\n", disksize);
		}
		break;
	case VDISK_FORMAT_RAW: goto L_MBR; // No header info
	default:
		fputs("vvd_info: Format not supported\n", stderr);
		return VVD_EVDFORMAT;
	}

	putchar('\n');

	//TODO: BSD disklabel detection
	//TODO: SGI disklabel detection

	//
	// MBR detection
	//

	union {
		MBR mbr;
		GPT gpt;
	} label;

L_MBR:

	if (vdisk_read_sector(vd, &label, 0)) return EXIT_SUCCESS;
	if (label.mbr.sig != MBR_SIG) return EXIT_SUCCESS;
	vvd_info_mbr(&label.mbr, flags);

	//
	// Extended MBR detection (EBR)
	//

	uint64_t ebrlba;
	for (int i = 0; i < 4; ++i) {
		switch (label.mbr.pe[i].type) {
		case 0xEE: // EFI GPT Protective
		case 0xEF: // EFI System Partition
			// Start of disk
			if (vdisk_read_sector(vd, &label, 1)) return VVD_EOK;
			if (label.gpt.sig == EFI_SIG) {
				ebrlba = 2;
				goto L_GPT_RDY;
			}
			// End of disk
			ebrlba = BYTE_TO_SECTOR(vd->capacity) - 1;
			if (vdisk_read_sector(vd, &label, ebrlba)) return VVD_EOK;
			if (label.gpt.sig == EFI_SIG) {
				flags |= VVD_INTERNAL_GPT_BKP;
				goto L_GPT_RDY;
			}
			continue;
		L_GPT_RDY:
			vvd_info_gpt(&label.gpt, flags);
			vvd_info_gpt_entries(vd, &label.gpt, ebrlba, flags);
			continue;
		}
	}

	return EXIT_SUCCESS;
}

//
// vvd_map
//

int vvd_map(VDISK *vd, uint32_t flags) {
	union {
		uint32_t *u32;
		uint64_t *u64;
	} btable; // block table
	union {
		uint32_t u32;
		uint64_t u64;
	} bcount; // block table count
	union {
		uint32_t u32;
		uint64_t u64;
	} bsize;  // block index size in bytes

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		btable.u32 = vd->vdi->in.offsets;
		bcount.u32 = vd->vdi->v1.blk_total;
		bsize.u32 = vd->vdi->v1.blk_size;
		break;
	case VDISK_FORMAT_VHD:
		if (vd->vhd->hdr.type != VHD_DISK_DYN) {
			fputs("vvd_map: vdisk is not dynamic\n", stderr);
			return VVD_EVDTYPE;
		}
		btable.u32 = vd->vhd->in.offsets;
		bcount.u32 = vd->vhd->dyn.max_entries;
		bsize.u32 = vd->vhd->dyn.blocksize;
		break;
	case VDISK_FORMAT_QED:
		btable.u64 = vd->qed->in.L1.offsets;
		bcount.u64 = vd->qed->in.entries;
		bsize.u64 = vd->qed->hdr.cluster_size;
		goto L_USES_64BIT_INDEX;
	default:
		fputs("vvd_map: unsupported format\n", stderr);
		return VVD_EVDFORMAT;
	}

	char bsizestr[BINSTR_LENGTH];
	size_t i = 0;
	size_t bn;	// Block table limit

	// 32-bit offsets

	bintostr(bsizestr, bsize.u32);
	printf(
	"Allocation map: %u blocks of %s each\n"
	" offset d |        0 |        1 |        2 |        3 |"
	"        4 |        5 |        6 |        7 |\n"
	"----------+----------+----------+----------+----------+"
	"----------+----------+----------+----------+\n",
	bcount.u32, bsizestr
	);
	bn = bcount.u32 - 8;
	for (; i < bn; i += 8) {
		printf(
		" %8zu | %8X | %8X | %8X | %8X | %8X | %8X | %8X | %8X |\n",
		i,
		btable.u32[i],     btable.u32[i + 1],
		btable.u32[i + 2], btable.u32[i + 3],
		btable.u32[i + 4], btable.u32[i + 5],
		btable.u32[i + 6], btable.u32[i + 7]
		);
	}
	if (bcount.u32 - i > 0) { // Left over
		printf(" %8zu |", i);
		for (; i < bcount.u32; ++i)
			printf(" %8X |", btable.u32[i]);
		putchar('\n');
	}

	return EXIT_SUCCESS;

	// 64-bit offsets

L_USES_64BIT_INDEX:

	bintostr(bsizestr, bsize.u64);
	printf(
	"Allocation map: %" PRIu64 " blocks to %s blocks\n"
	" offset d |                0 |                1 |"
	"                2 |                3 |\n"
	"----------+------------------+------------------+"
	"------------------+------------------+\n",
	bcount.u64, bsizestr
	);
	bn = bcount.u64 - 4;
	for (; i < bn; i += 4) {
		printf(
		" %8zu | %16"PRIX64" | %16"PRIX64" | %16"PRIX64" | %16"PRIX64" |\n",
		i,
		btable.u64[i],     btable.u64[i + 1],
		btable.u64[i + 2], btable.u64[i + 3]
		);
	}
	if (bcount.u64 - i > 0) { // Left over
		printf(" %8zu |", i);
		for (; i < bcount.u64; ++i)
			printf(" %16"PRIX64" |", btable.u64[i]);
		putchar('\n');
	}

	return EXIT_SUCCESS;
}

//
// vvd_new
//

int vvd_new(const oschar *path, uint32_t format, uint64_t capacity, uint32_t flags) {
	VDISK vd;
	if (vdisk_create(&vd, path, format, capacity, flags)) {
		vvd_perror(&vd);
		return vd.err.num;
	}
	printf("vvd_new: %s disk created successfully\n", vdisk_str(&vd));
	return EXIT_SUCCESS;
}

//
// vvd_compact
//

int vvd_compact(VDISK *vd, uint32_t flags) {
	puts("vvd_compact: [warning] This function is still work in progress");
	g_flags = flags;
	if (vdisk_op_compact(vd)) {
		vvd_perror(vd);
		return vd->err.num;
	}
	return EXIT_SUCCESS;
}
