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
// vvd_cb_progress
//

void vvd_cb_progress(uint32_t type, void *data) {
	switch (type) {
	case VVD_NOTIF_DONE:
		if (g_flags & VVD_PROGRESS)
		if (os_pfinish(&g_progress)) {
			fputs("os_pfinish: Could not finish progress bar", stderr);
			exit(1);
		}
		return;
	case VVD_NOTIF_VDISK_CREATED_TYPE_NAME:
		printf("%s\n", data);
		return;
	case VVD_NOTIF_VDISK_TOTAL_BLOCKS:
	case VVD_NOTIF_VDISK_TOTAL_BLOCKS64:
		if (g_flags & VVD_PROGRESS)
		if (os_pinit(&g_progress, PROG_MODE_POURCENT, 0)) {
			fputs("os_pinit: Could not init progress bar\n", stderr);
			exit(1);
		}
		return;
	case VVD_NOTIF_VDISK_CURRENT_BLOCK:
	case VVD_NOTIF_VDISK_CURRENT_BLOCK64:
		if (g_flags & VVD_PROGRESS)
		if (os_pupdate(&g_progress, 0)) {
			fputs("os_pinit: Could not update progress bar\n", stderr);
			exit(1);
		}
		return;
	}
}

//
// vvd_info_mbr
//

void vvd_info_mbr(MBR *mbr, uint32_t flags) {
	uint64_t totalsize = SECTOR_TO_BYTE(
		(uint64_t)mbr->pe[0].sectors + (uint64_t)mbr->pe[1].sectors +
		(uint64_t)mbr->pe[2].sectors + (uint64_t)mbr->pe[3].sectors
	);
	char strsize[BINSTR_LENGTH];
	bintostr(strsize, totalsize);

	if (flags & VVD_INFO_RAW) {
		printf(
		"\n"
		"disklabel          : DOS\n"
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
			i,
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
		"\n"
		"DOS (MBR) disklabel, %s used\n"
		"   Boot     Start        Size  Type\n", strsize
		);
		for (unsigned int i = 0; i < 4; ++i) {
			MBR_PARTITION pe = mbr->pe[i];
			bintostr(strsize, SECTOR_TO_BYTE(pe.sectors));
			printf(
			"%u. %c  %11u  %10s  %s\n",
			i,
			pe.status >= 0x80 ? '*' : ' ',
			pe.lba,
			strsize,
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
	int max = gpt->pt_entries;	// maximum limiter, typically 128
	char partname[EFI_PART_NAME_LENGTH];
	char partsize[BINSTR_LENGTH];
	UID_TEXT partguid, typeguid;
	GPT_ENTRY entry; // GPT entry
	uint32_t entrynum = 1;

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
		"flags              : 0x%08X\n"
		"partition flags    : 0x%08X\n",
		entrynum,
		partname,
		partguid,
		typeguid,
		entry.first.lba,
		entry.last.lba,
		entry.flags,
		entry.partflags
		);
	} else {
		bintostr(partsize, SECTOR_TO_BYTE(entry.last.lba - entry.first.lba));
		//TODO: GPT partition type (after name)
		printf(
		"%4u. %12" PRIu64 "%12s  s\n",
		entrynum, entry.first.lba, partsize
		);

		if (wr > 0)
			printf("      Name: %-36s", partname);

		// GPT flags
		if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
			puts("      + Platform required");
		if (entry.flags & EFI_PE_EFI_FIRMWARE_IGNORE)
			puts("      + Firmware ignore");
		if (entry.flags & EFI_PE_LEGACY_BIOS_BOOTABLE)
			puts("      + Legacy BIOS bootable");

		// Partition flags
		if (entry.partflags & EFI_PE_SUCCESSFUL_BOOT)
			puts("      + (Google) Successful boot");
		if (entry.partflags & EFI_PE_READ_ONLY)
			puts("      + (Microsoft) Read-only");
		if (entry.partflags & EFI_PE_SHADOW_COPY)
			puts("      + (Microsoft) Shadow copy");
		if (entry.partflags & EFI_PE_HIDDEN)
			puts("      + (Microsoft) Hidden");
	}

	if (entrynum > gpt->pt_entries)
		return;

	++lba; --max; ++entrynum;
	goto START;
}

//
// vvd_info
//

int vvd_info(VDISK *vd, uint32_t flags) {
	const char *type;	// vdisk type
	char disksize[BINSTR_LENGTH], blocksize[BINSTR_LENGTH];
	char uid1[UID_LENGTH], uid2[UID_LENGTH], uid3[UID_LENGTH], uid4[UID_LENGTH];

	switch (vd->format) {
	//
	// VDI
	//
	case VDISK_FORMAT_VDI: {
		switch (vd->vdiv1.type) {
		case VDI_DISK_DYN:	type = "dynamic"; break;
		case VDI_DISK_FIXED:	type = "fixed"; break;
		case VDI_DISK_UNDO:	type = "undo"; break;
		case VDI_DISK_DIFF:	type = "diff"; break;
		default:	type = "type?";
		}

		bintostr(disksize, vd->vdiv1.disksize);

		if (flags & VVD_INFO_RAW) {
			char create_uuid[UID_LENGTH], modify_uuid[UID_LENGTH],
				link_uuid[UID_LENGTH], parent_uuid[UID_LENGTH];

			bintostr(blocksize, vd->vdiv1.blocksize);
			uid_str(uid1, &vd->vdiv1.uuidCreate, UID_UUID);
			uid_str(uid2, &vd->vdiv1.uuidModify, UID_UUID);
			uid_str(uid3, &vd->vdiv1.uuidLinkage, UID_UUID);
			uid_str(uid4, &vd->vdiv1.uuidParentModify, UID_UUID);

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
			vd->vdihdr.majorv, vd->vdihdr.minorv,
			type,
			disksize, vd->vdiv1.disksize,
			vd->vdiv1.hdrsize,
			vd->vdiv1.fFlags,
			vd->vdiv1.u32Dummy,
			blocksize, vd->vdiv1.blocksize,
			vd->vdiv1.blockstotal, vd->vdiv1.blocksalloc, vd->vdiv1.blocksextra,
			vd->vdiv1.offBlocks, vd->vdiv1.offData,
			vd->vdiv1.cCylinders, vd->vdiv1.cHeads, vd->vdiv1.cSectors,
			vd->vdiv1.LegacyGeometry.cCylinders,
			vd->vdiv1.LegacyGeometry.cHeads,
			vd->vdiv1.LegacyGeometry.cSectors,
			vd->vdiv1.cbSector, vd->vdiv1.LegacyGeometry.cbSector,
			uid1, uid2, uid3, uid4
			);
		} else {
			printf(
			"VirtualBox VDI %s disk v%u.%u, %s\n",
			type, vd->vdihdr.majorv, vd->vdihdr.minorv, disksize
			);
			//TODO: Interpret flags
		}
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

		if (flags & VVD_INFO_RAW) {
			printf(
			"disk format        : VMDK\n"
			"version            : %u\n"
			"capacity           : %" PRIu64 " sectors\n"
			"overhead           : %" PRIu64 " sectors\n"
			"grain size         : %" PRIu64 " sectors\n",
			vd->vmdk.version,
			vd->vmdk.capacity,
			vd->vmdk.overHead,
			vd->vmdk.grainSize
			);
			
		} else {
			bintostr(disksize, vd->capacity);
			printf(
			"VMware VMDK disk v%u, %s compression, %s\n",
			vd->vmdk.version, comp, disksize
			);
		}

		if (vd->vmdk.uncleanShutdown)
			puts("+ Unclean shutdown");
	}
		break;
	//
	// VHD
	//
	case VDISK_FORMAT_VHD: {
		char sizecur[BINSTR_LENGTH];
		const char *byos;

		switch (vd->vhd.type) {
		case VHD_DISK_FIXED:	type = "fixed"; break;
		case VHD_DISK_DYN:	type = "dynamic"; break;
		case VHD_DISK_DIFF:	type = "differencing"; break;
		default:
			type = vd->vhd.type <= 6 ? "reserved (deprecated)" : "unknown";
		}

		switch (vd->vhd.creator_os) {
		case VHD_OS_WIN:	byos = "Windows"; break;
		case VHD_OS_MAC:	byos = "macOS"; break;
		default:	byos = "unknown"; break;
		}

		uid_str(uid1, &vd->vhd.uuid, UID_ASIS);
		str_s(vd->vhd.creator_app, 4);

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
			vd->vhd.major, vd->vhd.minor,
			type,
			vd->vhd.size_current,
			vd->vhd.size_original,
			vd->vhd.creator_app,
			vd->vhd.creator_major, vd->vhd.creator_minor,
			byos,
			vd->vhd.cylinders, vd->vhd.heads, vd->vhd.sectors,
			vd->vhd.checksum,
			uid1
			);
			if (vd->vhd.type != VHD_DISK_FIXED) {
				char paruuid[UID_LENGTH];
				uid_str(paruuid, &vd->vhddyn.parent_uuid, UID_ASIS);
				printf(
				"dyn. header ver.   : %u.%u\n"
				"offset table       : %" PRIu64 "\n"
				"offset data        : %" PRIu64 "\n"
				"block size         : %u\n"
				"dyn. checksum      : 0x%08X\n"
				"parent uuid        : %s\n"
				"parent timestamp   : %u\n"
				"bat entries        : %u\n"
				,
				vd->vhddyn.minor, vd->vhddyn.major,
				vd->vhddyn.table_offset,
				vd->vhddyn.data_offset,
				vd->vhddyn.blocksize,
				vd->vhddyn.checksum,
				paruuid,
				vd->vhddyn.parent_timestamp,
				vd->vhddyn.max_entries
				);
			}
		} else {
			bintostr(sizecur, vd->vhd.size_current);
			bintostr(disksize, vd->vhd.size_original);

			printf(
			"Connectix/Microsoft VHD %s disk v%u.%u, %s/%s\n",
			type, vd->vhd.major, vd->vhd.minor, sizecur, disksize
			);
		}

		if (vd->vhd.savedState)
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
			vd->qed.cluster_size,
			vd->qed.table_size,
			vd->qed.header_size,
			vd->qed.features,
			vd->qed.compat_features,
			vd->qed.autoclear_features,
			vd->qed.l1_offset
			);
			if (vd->qed.features & QED_F_BACKING_FILE) {
				printf(
				"back. name offset  : %u\n"
				"back. name size    : %u\n",
				vd->qed.backup_name_offset,
				vd->qed.backup_name_size
				);
			}
		} else {
			bintostr(disksize, vd->capacity);
			printf("QEMU Enhanced Disk, %s\n", disksize);
		}
		break;
	case VDISK_FORMAT_RAW: break; // No header info
	default:
		fputs("vvd_info: Format not supported\n", stderr);
		return VVD_EVDFORMAT;
	}

	//TODO: BSD disklabel detection
	//TODO: SGI disklabel detection

	//
	// MBR detection
	//

	MBR mbr;
	if (vdisk_read_sector(vd, &mbr, 0)) return EXIT_SUCCESS;
	if (mbr.sig != MBR_SIG) return EXIT_SUCCESS;
	vvd_info_mbr(&mbr, flags);

	//
	// Extended MBR detection (EBR)
	//

	uint64_t ebrlba;
	for (int i = 0; i < 4; ++i) {
		switch (mbr.pe[i].type) {
		case 0xEE: // EFI GPT Protective
		case 0xEF: // EFI System Partition
			// Start of disk
			if (vdisk_read_sector(vd, &mbr, 1)) return VVD_EOK;
			if (((GPT*)&mbr)->sig == EFI_SIG) {
				ebrlba = 2;
				goto L_GPT_RDY;
			}
			// End of disk
			ebrlba = BYTE_TO_SECTOR(vd->capacity) - 1;
			if (vdisk_read_sector(vd, &mbr, ebrlba)) return VVD_EOK;
			if (((GPT*)&mbr)->sig == EFI_SIG) {
				ebrlba -= ((GPT*)&mbr)->pt_entries; // typically 128
				goto L_GPT_RDY;
			}
			continue;
		L_GPT_RDY:
			vvd_info_gpt((GPT*)&mbr, flags);
			vvd_info_gpt_entries(vd, (GPT*)&mbr, ebrlba, flags);
			continue;
		}
	}

	return EXIT_SUCCESS;
}

//
// vvd_map
//

int vvd_map(VDISK *vd, uint32_t flags) {
	char bsizestr[BINSTR_LENGTH]; // If used
	uint64_t bsize; // block size
	uint32_t bcount; // block count
	int index64 = 0;

	switch (vd->format) {
	case VDISK_FORMAT_VDI:
		bcount = vd->vdiv1.blockstotal;
		bsize = vd->vdiv1.blocksize;
		break;
	case VDISK_FORMAT_VHD:
		if (vd->vhd.type != VHD_DISK_DYN) {
			fputs("vvd_map: vdisk is not dynamic\n", stderr);
			return VVD_EVDTYPE;
		}
		bcount = vd->u32blockcount;
		bsize = vd->vhddyn.blocksize;
		break;
	case VDISK_FORMAT_QED:
		index64 = 1;
		bcount = vd->u32blockcount;
		bsize = vd->qed.cluster_size;
		break;
	default:
		fputs("vvd_map: unsupported format\n", stderr);
		return VVD_EVDFORMAT;
	}

	bintostr(bsizestr, bsize);
	size_t i = 0;
	size_t bn;
	if (index64) {
		printf(
		"Allocation map: %u blocks to %s blocks\n"
		" offset d |                0 |                1 |"
			"                2 |                3 |\n"
		"----------+------------------+------------------+"
			"------------------+------------------+\n",
		bcount, bsizestr
		);
		bn = bcount - 4;
		for (; i < bn; i += 4) {
			printf(
			" %8zu | %16"PRIX64" | %16"PRIX64" | %16"PRIX64" | %16"PRIX64" |\n",
			i,
			vd->u64block[i],     vd->u64block[i + 1],
			vd->u64block[i + 2], vd->u64block[i + 3]
			);
		}
		if (bcount - i > 0) { // Left over
			printf(" %8zu |", i);
			for (; i < bcount; ++i)
				printf(" %16"PRIX64" |", vd->u64block[i]);
			putchar('\n');
		}
	} else {
		printf(
		"Allocation map: %u blocks of %s each\n"
		" offset d |        0 |        1 |        2 |        3 |"
			"        4 |        5 |        6 |        7 |\n"
		"----------+----------+----------+----------+----------+"
			"----------+----------+----------+----------+\n",
		bcount, bsizestr
		);
		bn = bcount - 8;
		for (; i < bn; i += 8) {
			printf(
			" %8zu | %8X | %8X | %8X | %8X | %8X | %8X | %8X | %8X |\n",
			i,
			vd->u32block[i],     vd->u32block[i + 1],
			vd->u32block[i + 2], vd->u32block[i + 3],
			vd->u32block[i + 4], vd->u32block[i + 5],
			vd->u32block[i + 6], vd->u32block[i + 7]
			);
		}
		if (bcount - i > 0) { // Left over
			printf(" %8zu |", i);
			for (; i < bcount; ++i)
				printf(" %8X |", vd->u32block[i]);
			putchar('\n');
		}
	}
	return EXIT_SUCCESS;
}

//
// vvd_new
//

int vvd_new(const oschar *path, uint32_t format, uint64_t capacity, uint32_t flags) {
	VDISK vd;
	if (vdisk_create(&vd, path, format, capacity, flags)) {
		vdisk_perror(&vd);
		return vd.errcode;
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
	if (vdisk_op_compact(vd, vvd_cb_progress)) {
		vdisk_perror(vd);
		return vd->errcode;
	}
	return EXIT_SUCCESS;
}
