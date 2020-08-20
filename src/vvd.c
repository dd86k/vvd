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
		"\nMBR: SERIAL %08X, %s USED, TYPE %04u\n"
		"   STAT  TYPE        LBA        SIZE    CHS start-end\n",
		mbr->serial, strsize, mbr->type
		);
		for (unsigned int i = 0; i < 4; ++i) {
			MBR_PARTITION pe = mbr->pe[i];
			printf(
			"%u.  %2XH   %2XH %10u  %10u  %4u:%3u:%2u-%4u:%3u:%2u\n",
			i, pe.status, pe.type, pe.lba, pe.sectors,
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
		"\nDOS (MBR) disklabel, %s used\n"
		"   Boot     Start        Size  Type\n", strsize
		);
		for (unsigned int i = 0; i < 4; ++i) {
			MBR_PARTITION pe = mbr->pe[i];
			bintostr(strsize, SECTOR_TO_BYTE(pe.sectors));
			printf(
			"%u. %c  %11u  %10s  %s\n",
			i,
			pe.status & 0x80 ? '*' : ' ',
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
	bintostr(gptsize, SECTOR_TO_BYTE(gpt->last.lba - gpt->first.lba));
	uid_str(diskguid, &gpt->guid, UID_GUID);
	if (flags & VVD_INFO_RAW) {
		printf(
		"\nGPT: v%u.%u (%u B), HDR CRC32 %08X, PT CRC32 %08X\n"
		"HEADER LBA %" PRIu64 ", BACKUP LBA %" PRIu64 "\n"
		"LBA %" PRIu64 " to %" PRIu64 " (%s)\n"
		"DISK GUID: %s\n"
		"PT LBA %" PRIu64 ", %u MAX ENTRIES, ENTRY SIZE %u\n",
		gpt->majorver, gpt->minorver, gpt->headersize, gpt->crc32, gpt->pt_crc32,
		gpt->current.lba, gpt->backup.lba,
		gpt->first.lba, gpt->last.lba, gptsize,
		diskguid,
		gpt->pt_location.lba, gpt->pt_entries, gpt->pt_esize
		);
	} else {
		printf(
		"\nGPT extended disklabel v%u.%u, %s used\n"
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

START:
	if (vdisk_read_sector(vd, &entry, lba)) {
		fputs("vvd_info_gpt_entries: Could not read GPT_ENTRY", stderr);
		return;
	}

	if (uid_nil(&entry.type))
		return;

	uid_str(typeguid, &entry.type, UID_GUID);
	uid_str(partguid, &entry.part, UID_GUID);
	wstra(partname, entry.partname, EFI_PART_NAME_LENGTH);

	bintostr(partsize, SECTOR_TO_BYTE(entry.last.lba - entry.first.lba));
	if (flags & VVD_INFO_RAW) {
		printf(
		"%2u. %-36s\n"
		"  LBA %" PRIu64 " TO %" PRIu64 " (%s)\n"
		"  PART GUID: %s\n"
		"  TYPE GUID: %s\n"
		"  FLAGS: %XH, PART FLAGS: %XH\n",
		entrynum, partname,
		entry.first.lba, entry.last.lba, partsize,
		partguid, typeguid,
		entry.flags, entry.partflags
		);
	} else {
		printf(
		"%2u. %s, %-36s\n",
		entrynum, partsize, partname
		);
	}

	// GPT flags
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Platform required");
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Firmware ignore");
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Legacy BIOS bootable");

	// Partition flags
	if (entry.partflags & EFI_PE_SUCCESSFUL_BOOT)
		puts("+ (Google) Successful boot");
	if (entry.partflags & EFI_PE_READ_ONLY)
		puts("+ (Microsoft) Read-only");
	if (entry.partflags & EFI_PE_SHADOW_COPY)
		puts("+ (Microsoft) Shadow copy");
	if (entry.partflags & EFI_PE_HIDDEN)
		puts("+ (Microsoft) Hidden");

	if (max <= 0)
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

		char create_uuid[UID_LENGTH], modify_uuid[UID_LENGTH],
			link_uuid[UID_LENGTH], parent_uuid[UID_LENGTH];

		bintostr(disksize, vd->vdiv1.disksize);
		bintostr(blocksize, vd->vdiv1.blocksize);
		uid_str(uid1, &vd->vdiv1.uuidCreate, UID_UUID);
		uid_str(uid2, &vd->vdiv1.uuidModify, UID_UUID);
		uid_str(uid3, &vd->vdiv1.uuidLinkage, UID_UUID);
		uid_str(uid4, &vd->vdiv1.uuidParentModify, UID_UUID);

		printf(
		"VirtualBox VDI %s disk v%u.%u, %s\n"
		"Header size: %u, Flags: %XH, Dummy: %u\n"
		"Blocks: %u (allocated: %u, extra: %u), %s size\n"
		"Offset to data: %Xh, to allocation table: %Xh\n"
		"Cylinders: %u (legacy: %u)\n"
		"Heads: %u (legacy: %u)\n"
		"Sectors: %u (legacy: %u)\n"
		"Sector size: %u (legacy: %u)\n"
		"Create UUID: %s\n"
		"Modify UUID: %s\n"
		"Link   UUID: %s\n"
		"Parent UUID: %s\n",
		type, vd->vdihdr.majorv, vd->vdihdr.minorv, disksize,
		vd->vdiv1.hdrsize, vd->vdiv1.fFlags, vd->vdiv1.u32Dummy,
		vd->vdiv1.totalblocks, vd->vdiv1.blocksalloc, vd->vdiv1.blocksextra, blocksize,
		vd->vdiv1.offData, vd->vdiv1.offBlocks,
		vd->vdiv1.cCylinders, vd->vdiv1.LegacyGeometry.cCylinders,
		vd->vdiv1.cHeads, vd->vdiv1.LegacyGeometry.cHeads,
		vd->vdiv1.cSectors, vd->vdiv1.LegacyGeometry.cSectors,
		vd->vdiv1.cbSector, vd->vdiv1.LegacyGeometry.cbSector,
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

		bintostr(disksize, vd->capacity);
		printf(
		"VMware VMDK disk v%u, %s compression, %s\n"
		"Capacity: %"PRIu64" Sectors\n"
		"Overhead: %"PRIu64" Sectors\n"
		"Grain size (Raw): %"PRIu64" Sectors\n",
		vd->vmdk.version, comp, disksize,
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
		char sizecur[BINSTR_LENGTH];
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

		uid_str(uid1, &vd->vhd.uuid, UID_ASIS);
		bintostr(sizecur, vd->vhd.size_current);
		bintostr(disksize, vd->vhd.size_original);
		str_s(vd->vhd.creator_app, 4);

		printf(
		"Connectix/Microsoft VHD %s disk v%u.%u, %s/%s\n"
		"Created by \"%.4s\" v%u.%u on %s\n"
		"Cylinders: %u, Heads: %u, Sectors: %u\n"
		"CRC32: %08X, UUID: %s\n",
		type, vd->vhd.major, vd->vhd.minor, sizecur, disksize,
		vd->vhd.creator_app, vd->vhd.creator_major, vd->vhd.creator_minor, os,
		vd->vhd.cylinders, vd->vhd.heads, vd->vhd.sectors,
		vd->vhd.checksum,
		uid1
		);

		if (vd->vhd.type != VHD_DISK_FIXED) {
			char paruuid[UID_LENGTH];
			uid_str(paruuid, &vd->vhddyn.parent_uuid, UID_ASIS);
			printf(
			"Dynamic header v%u.%u, data: %" PRIu64 ", table: %" PRIu64 "\n"
			"Blocksize: %u, checksum: %08X\n"
			"Parent UUID: %s, timestamp: %u\n"
			"BAT: %u entries\n",
			vd->vhddyn.minor, vd->vhddyn.major,
			vd->vhddyn.data_offset, vd->vhddyn.table_offset,
			vd->vhddyn.blocksize, vd->vhddyn.checksum,
			paruuid, vd->vhddyn.parent_timestamp,
			vd->vhddyn.max_entries
			);
		}

		if (vd->vhd.savedState)
			puts("+ Saved state");
	}
		break;
//	case VDISK_FORMAT_VHDX:
	case VDISK_FORMAT_QED:
		bintostr(disksize, vd->capacity);
		printf(
		"QEMU Enhanced Disk\n"
		"Cluster size: %u, table size: %u, header size: %u\n"
		"Features: %" PRIx64 "\n"
		"Compat features: %" PRIx64 "\n"
		"Autoclear features: %" PRIx64 "\n"
		"L1 offset: %" PRIu64 "\n",
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
			"Backing name offset: %u\n"
			"Backing name size: %u\n",
			vd->qed.backup_name_offset,
			vd->qed.backup_name_size
			);
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
			vvd_info_gpt((GPT *)&mbr, flags);
			vvd_info_gpt_entries(vd, (GPT *)&mbr, ebrlba, flags);
			continue;
		}
	}

	return VVD_EOK;
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
		bcount = vd->vdiv1.totalblocks;
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
	return VVD_EOK;
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
	return 0;
}

//
// vvd_compact
//

int vvd_compact(VDISK *vd, uint32_t flags) {
	if (vd->u32blockcount == 0) {
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

		if (vd->vdiv1.type != VDI_DISK_DYN) {
			fputs("vvd_compact: vdisk not dynamic\n", stderr);
			return VVD_EVDTYPE;
		}

		if (vd->vdiv1.blocksalloc == 0)
			return EXIT_SUCCESS;

		//
		// Block buffer for transfer
		//

		uint8_t *buffer;	// Block buffer
		if ((buffer = malloc(vd->vdiv1.blocksize)) == NULL)
			return VVD_EALLOC;

		//
		// Temporary VDISK
		// Also assigns the same attributes from the source vdisk
		//

		VDISK vdtmp;
		memcpy(&vdtmp.vdihdr, &vd->vdihdr, sizeof(VDI_HDR));
		memcpy(&vdtmp.vdiv1, &vd->vdiv1, sizeof(VDIHEADER1));
		/*if (vdisk_create(&vdtmp, NULL, VDISK_FORMAT_VDI, vd->capacity, VDISK_CREATE_TEMP)) {
			vdisk_perror(&vdtmp);
			return vd->errcode;
		}*/
		if ((vdtmp.u32block = malloc(vd->vdiv1.blocksalloc * 4)) == NULL)
			return VVD_EALLOC;
		vdtmp.offset = vd->offset;
		vdtmp.format = vd->format;
		vdtmp.u32blockcount = vd->u32blockcount;

		for (size_t i = 0; i < vd->vdiv1.blocksalloc; ++i)
			vdtmp.u32block[i] = VDI_BLOCK_FREE;

		printf("vvd_compact: %s temporary disk created\n", vdisk_str(&vdtmp));

		//
		// Main housework
		//

		// "Optimized" buffer size
		size_t oblocksize = vd->vdiv1.blocksize / sizeof(size_t);
		// "Optimized" buffer pointer
		size_t *obuffer = (size_t*)buffer;

		uint32_t stat_unalloc = 0;	// unallocated blocks
		uint32_t stat_occupied = 0;	// allocated blocks with data
		uint32_t stat_zero = 0;	// blocks with no data inside
		uint32_t stat_alloc = 0;	// allocated blocks

		char strbsize[BINSTR_LENGTH];
		bintostr(strbsize, vd->vdiv1.blocksize);
		printf("vvd_compact: writing (%s/block, %u checks/%u bytes)...\n",
			strbsize, (uint32_t)oblocksize, (uint32_t)sizeof(size_t));
		os_pinit(&progress, PROG_MODE_POURCENT, vd->u32blockcount - 1);

		uint32_t d = 0;
		uint32_t i = 0;
		// check/fix allocation errors before compating
		for (; i < vd->u32blockcount; ++i) {
			uint32_t bi = vd->u32block[i]; // block index
			if (bi >= VDI_BLOCK_FREE) {
				++stat_unalloc;
				continue;
			}
			os_pupdate(&progress, (uint32_t)i);
			if (bi < vd->vdiv1.blocksalloc) {
				if (vdtmp.u32block[bi] == VDI_BLOCK_FREE) {
					vdtmp.u32block[bi] = i;
				} else {
					vd->u32block[bi] = VDI_BLOCK_FREE;
					//TODO: Update header once manipulating source
					//rc = vdiUpdateBlockInfo(pImage, i);
					vdisk_write_block_at(&vdtmp, buffer, i, d++);
				}
			} else {
				vd->u32block[bi] = VDI_BLOCK_FREE;
				//TODO: Update header once manipulating source
				//vd->u32block[bi] = VDI_BLOCK_FREE;
				vdisk_write_block_at(&vdtmp, buffer, i, d++);
			}
		}
		// Find redundant information and update the block pointers accordingly
		for (i = 0; i < vd->u32blockcount; ++i) {
			uint32_t bi = vd->u32block[i]; // block index
			if (bi >= VDI_BLOCK_FREE) {
				++stat_unalloc;
				continue;
			}
		}
		// Fill bubbles with other data if available
//		for (i = 0; o < vd->vdi.blocksalloc
		
		vdtmp.vdiv1.blocksalloc = stat_occupied;
		if (vdisk_update(&vdtmp)) {
			vdisk_perror(&vdtmp);
			return vdtmp.errcode;
		}
		os_pfinish(&progress);
		printf("vvd_compact: %u/%u blocks written, %u unallocated, %u zero, %u total\n",
			stat_occupied, stat_alloc, stat_unalloc, stat_zero, vd->u32blockcount
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
