#include <stdio.h>
#include <stdint.h>
#include "mbr.h"
#include <assert.h>

//
// mbr_lba
//

uint32_t mbr_lba(CHS *chs) {
	// LBA = (C * HPC + H) * SPT + (S − 1)
	// HPC	Max heads per cylinders, typically 16 (28-bit LBA)
	// SPT	Max sectors per strack, typically 63 (28-bit LBA)
	uint8_t sector = chs->sector & 0x3F;
	uint16_t cylinder = chs->cylinder | ((chs->sector & 0xC0) << 2);
	return (cylinder * 16 * chs->head) * 63 + (sector - 1);
}

//
// mbr_lba_a
//

uint32_t mbr_lba_a(CHS *chs, uint64_t dsize) {
	uint8_t HPC;

	if (dsize <= MBR_LBA_A_504_MB)
		HPC = 16;
	else if (dsize <= MBR_LBA_A_1008_MB)
		HPC = 32;
	else if (dsize <= MBR_LBA_A_2016_MB)
		HPC = 64;
	else if (dsize <= MBR_LBA_A_4032_MB)
		HPC = 128;
	else if (dsize <= MBR_LBA_A_8032_MB)
		HPC = 255;
	else
		HPC = 16; //TODO: Verify HPC value when disksize >8.5 GiB

	uint8_t sector = chs->sector & 0x3F;
	uint16_t cylinder = chs->cylinder | ((chs->sector & 0xC0) << 2);
	return (cylinder * HPC * chs->head) * 63 + (sector - 1);
}

//
// mbr_part_type_str
//

const char *mbr_part_type_str(uint8_t type) {
	// Sources:
	// - https://www.win.tue.nl/~aeb/partitions/partition_types-1.html
	// - https://en.wikipedia.org/wiki/Partition_type
	// - fdisk(1) (util-linux)
	switch (type) {
	case 0x00: return "Empty";
	case 0x01: return "FAT12";
	case 0x02: return "XENIX root";
	case 0x03: return "XENIX usr";
	case 0x04: return "FAT16 <32M";
	case 0x05: return "Extended";
	case 0x06: return "FAT16";
	case 0x07: return "HPFS/NTFS/exFAT";
	case 0x08: return "AIX";
	case 0x09: return "AIX bootable";
	case 0x0a: return "OS/2 Boot Managed";
	case 0x0b: return "W95 FAT32";
	case 0x0c: return "W95 FAT32 (LBA)";
	case 0x0e: return "W95 FAT16 (LBA)";
	case 0x0f: return "W95 Ext'd (LBA)";
	case 0x10: return "OPUS";
	case 0x11: return "Hidden FAT12";
	case 0x12: return "Compaq diagnostic parition";
	case 0x14: return "Hidden FAT16 <32M";
	case 0x16: return "Hidden FAT16";
	case 0x17: return "Hidden HPFS/NTF";
	case 0x18: return "AST SmartSleep";
	case 0x1b: return "Hidden W95 FAT32 <32M";
	case 0x1c: return "Hidden W95 FAT32";
	case 0x1e: return "Hidden W95 FAT16";
	case 0x24: return "NEC DOS";
	case 0x27: return "Hidden NTFS/RE Windows";
	case 0x39: return "Plan 9";
	case 0x3c: return "PartitionMagic";
	case 0x40: return "Venix 80286";
	case 0x41: return "PPC PReP Boot";
	case 0x42: return "SFS";
	case 0x4d: return "QNX4.x";
	case 0x4e: return "QNX4.x 2nd part";
	case 0x4f: return "QNX4.x 3rd part";
	case 0x50: return "OnTrack DM";
	case 0x51: return "OnTrack DM6 Aux";
	case 0x52: return "CP/M";
	case 0x53: return "OnTrack DM6 Aux";
	case 0x54: return "OnTrackDM6";
	case 0x55: return "EZ-Drive";
	case 0x56: return "Golden Bow";
	case 0x5c: return "Priam Edisk";
	case 0x61: return "SpeedStor";
	case 0x63: return "GNU HURD or Sys";
	case 0x64: return "Novell Netware";
	case 0x65: return "Novell Netware";
	case 0x70: return "DiskSecure Mult";
	case 0x75: return "PC/IX";
	case 0x80: return "Old Minix";
	case 0x81: return "Minix / old Linux";
	case 0x82: return "Linux swap / Solaris";
	case 0x83: return "Linux";
	case 0x84: return "OS/2 hidden";
	case 0x85: return "Linux extended";
	case 0x86: return "FAT16 volume set";
	case 0x87: return "NTFS volume set";
	case 0x88: return "Linux plaintext";
	case 0x8e: return "Linux LVM";
	case 0x93: return "Amoeba";
	case 0x94: return "Amoeba BBT";
	case 0x9f: return "BSD/OS";
	case 0xa0: return "IBM Thinkpad hibernation";
	case 0xa5: return "FreeBSD";
	case 0xa6: return "OpenBSD";
	case 0xa7: return "NeXTSTEP";
	case 0xa8: return "Darwin UFS";
	case 0xa9: return "NetBSD";
	case 0xab: return "Darwin boot";
	case 0xaf: return "HFS / HFS+";
	case 0xb7: return "BSDI fs";
	case 0xb8: return "BSDI swap";
	case 0xbb: return "Boot Wizard hidden";
	case 0xbc: return "Acronis FAT32 backup";
	case 0xbe: return "Solaris boot";
	case 0xbf: return "Solaris";
	case 0xc1: return "DRDOS/secure (FAT-12)";
	case 0xc4: return "DRDOS/secure (FAT-16)";
	case 0xc6: return "DRDOS/secure (FAT-32)";
	case 0xc7: return "Syrinx";
	case 0xda: return "Non-FS data";
	case 0xdb: return "CP/M / CTOS / KDG";
	case 0xde: return "Dell Utility";
	case 0xdf: return "BootIt";
	case 0xe1: return "DOS access";
	case 0xe3: return "DOS R/O";
	case 0xe4: return "SpeedStor";
	case 0xea: return "Rufus alignment";
	case 0xeb: return "BeOS fs";
	case 0xee: return "GPT";
	case 0xef: return "EFI (FAT-12/16/32)";
	case 0xf0: return "Linux/PA-RISC boot loader";
	case 0xf1: return "SpeedStor";
	case 0xf4: return "SpeedStor";
	case 0xf2: return "DOS secondary";
	case 0xfb: return "VMware VMFS";
	case 0xfc: return "VMware VMKCORE";
	case 0xfd: return "Linux raid auto";
	case 0xfe: return "LANstep";
	case 0xff: return "Xenix BBT";
	default:   return "Unknown";
	}
}
