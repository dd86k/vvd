#include <stdint.h>
#include <stddef.h>
#include "gpt.h"	// includes uid.h
#include "hash.h"

struct gpt_i_entry_t {
	uint32_t hash;	// See hash.c for current implementation
	const char *value;	// GUID string value
	const char *name;	// Partition type name
};

// Source: https://en.wikipedia.org/wiki/GUID_Partition_Table#Partition_type_GUIDs
//TODO: Wait for xxHash implementation, multiple collisions found
const struct gpt_i_entry_t gpt_i_types[] = {
	{ 0x8E022B3A, "00000000-0000-0000-0000-000000000000", "Empty" },
	{ 0x0514B937, "024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR partition scheme" },
	{ 0xACCE99DD, "C12A7328-F81F-11D2-BA4B-00A0C93EC93B", "EFI System partition" },
	{ 0xC7E0162E, "21686148-6449-6E6F-744E-656564454649", "BIOS boot partition" },
	{ 0xC72C7A70, "D3BFE2DE-3DAF-11DF-BA40-E3A556D89593", "Intel Fast Flash" },
	{ 0x10AC1CDD, "F4019732-066E-4E12-8273-346C5641494F", "Sony boot partition" },
	{ 0xF752E0C7, "BFBFAFE7-A34F-448A-9A5B-6213EB736C22", "Lenovo boot partition" },
	// Windows
	{ 0x269ADB66, "E3C9E316-0B5C-4DB8-817D-F92DF00215AE", "Microsoft Reserved Partition" },
	{ 0x5E16CC4F, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Windows Basic data partition" },
	{ 0x41C253B7, "5808C8AA-7E8F-42E0-85D2-E1E90434CFB3", "Windows Logical Disk Manager metadata partition" },
	{ 0x0E6EE3DF, "AF9B60A0-1431-4F62-BC68-3311714A69AD", "Windows Logical Disk Manager data partition" },
	{ 0x067AF665, "DE94BBA4-06D1-4D40-A16A-BFD50179D6AC", "Windows Recovery Environment" },
	{ 0x43D5E6B4, "37AFFC90-EF7D-4E96-91C3-2D7AE055B174", "IBM General Parallel File System partition" },
	{ 0x14246E2B, "E75CAF8F-F680-4CEE-AFA3-B001E56EFC2D", "Windows Storage Spaces partition" },
	{ 0x78218E65, "558D43C5-A1AC-43C0-AAC8-D1472B2923D1", "Windows Storage Replica partition" },
	// HP-UX
	{ 0xADF08B11, "75894C1E-3AEB-11D3-B7C1-7B03A0000000", "HP-UX Data partition" },
	{ 0x46C5DCAA, "E2A1E728-32E3-11D6-A682-7B03A0000000", "HP-UX Service partition" },
	// Linux
	{ 0x484E9B70, "0FC63DAF-8483-4772-8E79-3D69D8477DE4", "Linux filesystem data" },
	{ 0x1C0E699B, "A19D880F-05FC-4D3B-A006-743F0F84911E", "Linux RAID partition" },
	{ 0xDABC39F1, "44479540-F297-41B2-9AF7-D131D5F0458A", "Linux Root partition (x86)" },
	{ 0xED1FD152, "4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709", "Linux Root partition (x86-64)" },
	{ 0x4CE50D2E, "69DAD710-2CE4-4E3C-B16C-21A1D49ABED3", "Linux Root partition (32-bit ARM)" },
	{ 0xB3C56AFD, "B921B045-1DF0-41C3-AF44-4C6F280D3FAE", "Linux Root partition (64-bit ARM/AArch64)" },
	{ 0xAA5B0BB4, "BC13C2FF-59E6-4262-A352-B275FD6F7172", "Linux Boot partition" },
	{ 0x1B3093AB, "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F", "Linux Swap partition" },
	{ 0x4CCA9687, "E6D6D379-F507-44C2-A23C-238F2A3DF928", "Linux Logical Volume Manager partition" },
	{ 0xD1EA3E64, "933AC7E1-2EB4-4F13-B844-0E14E2AEF915", "Linux Home Partition" },
	{ 0x05FAA95A, "3B8F8425-20E0-4F3B-907F-1A25A76F98E8", "Linux Server Data (/srv) Partition" },
	{ 0x658D616D, "7FFEC5C9-2D00-49B7-8941-3EA10A5586B7", "Linux Plain dm-crypt partition" },
	{ 0x235DB26A, "CA7D7CCB-63ED-4C53-861C-1742536059CC", "Linux Unified Key Setup partition" },
	{ 0x81C15265, "8DA63339-0007-60C0-C436-083AC8230908", "Linux Reserved" },
	// FreeBSD
	{ 0x4BB3F4C4, "83BD6B9D-7F41-11DC-BE0B-001560B84F0F", "FreeBSD Boot Partition" },
	{ 0x40D76637, "516E7CB4-6ECF-11D6-8FF8-00022D09712B", "FreeBSD Data Partition" },
	{ 0x601A83A7, "516E7CB5-6ECF-11D6-8FF8-00022D09712B", "FreebSD Swap Partition" },
	{ 0xA2AB033D, "516E7CB6-6ECF-11D6-8FF8-00022D09712B", "FreeBSD Unix File System Partition" },
	{ 0x56D5FEF5, "516E7CB8-6ECF-11D6-8FF8-00022D09712B", "FreeBSD Vinum Volume Manager Partition " },
	{ 0x907CCD98, "516E7CBA-6ECF-11D6-8FF8-00022D09712B", "FreeBSD ZFS Partition" },
	// macOS
	{ 0x470B460A, "48465300-0000-11AA-AA11-00306543ECAC", "Hierarchical File System Plus (HFS+) partition" },
	{ 0x5FCA3FBC, "7C3457EF-0000-11AA-AA11-00306543ECAC", "Apple APFS or FileVault volume container" },
	{ 0x1429FF52, "55465300-0000-11AA-AA11-00306543ECAC", "Apple UFS container" },
	{ 0x0001C10D, "6A898CC3-1DD2-11B2-99A6-080020736631", "Apple ZFS" },
	{ 0xDC5F557C, "52414944-0000-11AA-AA11-00306543ECAC", "Apple RAID partition" },
	{ 0x962D949D, "52414944-5F4F-11AA-AA11-00306543ECAC", "Apple RAID partition, offline" },
	{ 0xD786CE98, "426F6F74-0000-11AA-AA11-00306543ECAC", "Apple Boot partition (Recovery HD)" },
	{ 0x95841C2E, "4C616265-6C00-11AA-AA11-00306543ECAC", "Apple Label" },
	{ 0xD9360BFE, "5265636F-7665-11AA-AA11-00306543ECAC", "Apple TV Recovery partition" },
	{ 0xE232C3B3, "53746F72-6167-11AA-AA11-00306543ECAC", "Apple HFS+ FileVault volume container" },
	{ 0x39238B68, "B6FA30DA-92D2-4A9A-96F1-871EC6486200", "Apple SoftRAID_Status partition" },
	{ 0xCF12298F, "2E313465-19B9-463F-8126-8A7993773801", "Apple SoftRAID_Scratch partition" },
	{ 0xF3228EEE, "FA709C7E-65B1-4593-BFD5-E71D61DE9B02", "Apple SoftRAID_Volume partition" },
	{ 0xA211B25C, "BBBA6DF5-F46F-4A89-8F59-8765B2727503", "Apple SoftRAID_Cache partition " },
	// Solaris
	{ 0x58A8CC47, "6A82CB45-1DD2-11B2-99A6-080020736631", "Solaris Boot Partition" },
	{ 0x99F24576, "6A85CF4D-1DD2-11B2-99A6-080020736631", "Solaris Root Partition" },
	{ 0xC47C9EA5, "6A87C46F-1DD2-11B2-99A6-080020736631", "Solaris Swap Partition" },
	{ 0xB764C015, "6A8B642B-1DD2-11B2-99A6-080020736631", "Solaris Backup Partition" },
	{ 0x0001C10D, "6A898CC3-1DD2-11B2-99A6-080020736631", "Solaris User (/usr) Partition" },
	{ 0xEE129404, "6A8EF2E9-1DD2-11B2-99A6-080020736631", "Solaris /var Partition" },
	{ 0xBA86FAD9, "6A90BA39-1DD2-11B2-99A6-080020736631", "Solaris Home (/home) Partition" },
	{ 0xB1EFAAC0, "6A9283A5-1DD2-11B2-99A6-080020736631", "Solaris Alternate Sector" },
	{ 0x08647456, "6A945A3B-1DD2-11B2-99A6-080020736631", "Solaris Reserved" },
	{ 0x3CE40B9B, "6A9630D1-1DD2-11B2-99A6-080020736631", "Solaris Reserved" },
	{ 0x21103731, "6A980767-1DD2-11B2-99A6-080020736631", "Solaris Reserved" },
	{ 0xCB448C01, "6A96237F-1DD2-11B2-99A6-080020736631", "Solaris Reserved" },
	{ 0xF8F6FF5C, "6A8D2AC7-1DD2-11B2-99A6-080020736631", "Solaris Reserved" },
	// NetBSD
	{ 0x606E6C4E, "49F48D32-B10E-11DC-B99B-0019D1879648", "NetBSD Swap Partition" },
	{ 0x3F0F4034, "49F48D5A-B10E-11DC-B99B-0019D1879648", "NetBSD FFS Partition" },
	{ 0x3486D322, "49F48D82-B10E-11DC-B99B-0019D1879648", "NetBSD LFS Partition" },
	{ 0x25718784, "49F48DAA-B10E-11DC-B99B-0019D1879648", "NetBSD RAID Partition" },
	{ 0xC53767E8, "2DB519C4-B10F-11DC-B99B-0019D1879648", "NetBSD Concatended Partition" },
	{ 0xC6539646, "2DB519EC-B10F-11DC-B99B-0019D1879648", "NetBSD Encrypted Partition" },
	// ChromeOS
	{ 0xEE26115E, "FE3A2A5D-4F32-41A7-B725-ACCC3285A309", "ChromeOS kernel" },
	{ 0x50868D21, "3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC", "ChromeOS rootfs" },
	{ 0xDA299AC0, "2E0A753D-9E48-43B0-8337-B15192CB1B5E", "ChromeOS reserved" },
	// CoreOS
	{ 0xB791D051, "5DFBF5F4-2848-4BAC-AA5E-0D9A20B745A6", "CoreOS /usr Partition" },
	{ 0x13F8C760, "3884DD41-8582-4404-B9A8-E9B84F2DF50E", "CoreOS Resizable rooffs Partition" },
	{ 0xE94125B6, "C95DC21A-DF0E-4340-8D7B-26CBFA9A03E0", "CoreOS OEM Customized Partition" },
	{ 0xB7AC5EE7, "BE9067B9-EA49-4F15-B4F6-F36F8C9E1818", "CoreOS RAID rootfs Partition" },
	// Haiku
	{ 0x66BC7C15, "42465331-3BA3-10F1-802A-4861696B7521", "Haiku BFS" },
	// MidnightBSD
	{ 0x75A67933, "85D5E45E-237C-11E1-B4B3-E89A8F7FC3A7", "MidnightBSD Boot Partition" },
	{ 0xE80FF09F, "85D5E45A-237C-11E1-B4B3-E89A8F7FC3A7", "MidnightBSD Data Partition" },
	{ 0x4A731390, "85D5E45B-237C-11E1-B4B3-E89A8F7FC3A7", "MidnightBSD Swap Partition" },
	{ 0xEB0DD6B8, "0394EF8B-237E-11E1-B4B3-E89A8F7FC3A7", "MidnightBSD UFS Partition" },
	{ 0xF29693F7, "85D5E45C-237C-11E1-B4B3-E89A8F7FC3A7", "MidnightBSD Vinum Volume Manager Partition" },
	{ 0x139BE2B5, "85D5E45D-237C-11E1-B4B3-E89A8F7FC3A7", "MidnightBSD ZFS Partition" },
	// Ceph
	{ 0xEDCE712A, "45B0969E-9B03-4F30-B4C6-B4B80CEFF106", "Ceph Journal" },
	{ 0x14A24331, "45B0969E-9B03-4F30-B4C6-5EC00CEFF106", "Ceph dm-crypt Journal" },
	{ 0x992ABA9B, "4FBD7E29-9D25-41B8-AFD0-062C0CEFF05D", "Ceph OSD" },
	{ 0x4D709BC7, "4FBD7E29-9D25-41B8-AFD0-5EC00CEFF05D", "Ceph dm-crypt OSD" },
	{ 0xE3E1A7CE, "89C57F98-2FE5-4DC0-89C1-F3AD0CEFF2BE", "Ceph Disk in creation" },
	{ 0xD1E11E1B, "89C57F98-2FE5-4DC0-89C1-5EC00CEFF2BE", "Ceph dm-crypt Disk in creation" },
	{ 0xEBF0EFE4, "CAFECAFE-9B03-4F30-B4C6-B4B80CEFF106", "Ceph Block" },
	{ 0xA8AC9BB2, "30CD0809-C2B2-499C-8879-2D6B78529876", "Ceph Block DB" },
	{ 0x105D4F20, "5CE17FCE-4087-4169-B7FF-056CC58473F9", "Ceph Block write-ahead log" },
	{ 0x019E802F, "FB3AABF9-D25F-47CC-BF5E-721D1816496B", "Ceph Lockbox for dm-crypt keys" },
	{ 0xBA15CFAE, "4FBD7E29-8AE0-4982-BF9D-5A8D867AF560", "Ceph Multipath OSD" },
	{ 0x0C84891F, "45B0969E-8AE0-4982-BF9D-5A8D867AF560", "Ceph Multipath Journal" },
	{ 0xA36A1973, "CAFECAFE-8AE0-4982-BF9D-5A8D867AF560", "Ceph Multipath Block" },
	{ 0x6FA76A64, "7F4A666A-16F3-47A2-8445-152EF4D03F6C", "Ceph Multipath Block" },
	{ 0x07949EE4, "EC6D6385-E346-45DC-BE91-DA2A7C8B3261", "Ceph Multipath Block DB" },
	{ 0xA2CF5925, "01B41E1B-002A-453C-9F17-88793989FF8F", "Ceph Multipath Block write-ahead log" },
	{ 0x74A943D6, "CAFECAFE-9B03-4F30-B4C6-5EC00CEFF106", "Ceph dm-crypt Block" },
	{ 0x5C6D0C5B, "93B0052D-02D9-4D8A-A43B-33A3EE4DFBC3", "Ceph dm-crypt Block DB" },
	{ 0x276146A6, "306E8683-4FE2-4330-B7C0-00A917C16966", "Ceph dm-crypt Block write-ahead log" },
	{ 0xA8D72DB9, "45B0969E-9B03-4F30-B4C6-35865CEFF106", "Ceph dm-crypt LUKS Journal" },
	{ 0x63F9352A, "CAFECAFE-9B03-4F30-B4C6-35865CEFF106", "Ceph dm-crypt LUKS Block" },
	{ 0xF8A5CD43, "166418DA-C469-4022-ADF4-B30AFD37F176", "Ceph dm-crypt LUKS Block DB" },
	{ 0x0F0F3645, "86A32090-3647-40B9-BBBD-38D8C573AA86", "Ceph dm-crypt LUKS Block write-ahead log" },
	{ 0xA5CED512, "4FBD7E29-9D25-41B8-AFD0-35865CEFF05D", "Ceph dm-crypt LUKS OSD" },
	// OpenBSD
	{ 0xF6E74095, "824CC7A0-36A8-11E3-890A-952519AD3F61", "OpenBSD Data Partition" },
	// QNX
	{ 0xDB34F656, "CEF5A9AD-73BC-4601-89F3-CDEEEEE321A1", "QNX6 Power-safe FS" },
	// Plan9
	{ 0xB6B6A42F, "C91818F9-8025-47AF-89D2-F030D7000C2C", "Plan9 Partition" },
	// VMware ESX
	{ 0x31E75E8A, "9D275380-40AD-11DB-BF97-000C2911D1B8", "VMware ESX vmkcore Partition" },
	{ 0x4186E797, "AA31E02A-400F-11DB-9590-000C2911D1B8", "VMware ESX VMFS Partition" },
	{ 0x34FB36D1, "9198EFFC-31C0-11DB-8F78-000C2911D1B8", "VMware ESX Reserved" },
	// Android-x86
	{ 0x83A9422A, "2568845D-2332-4675-BC39-8FA5A4748D15", "Android-x86 Bootloader" },
	{ 0x72A01C37, "114EAFFE-1552-4022-B26E-9B053604CF84", "Android-x86 Bootloader2" },
	{ 0xB1905CF7, "49A4D17F-93A3-45C1-A0DE-F50B2EBE2599", "Android-x86 Boot" },
	{ 0x3B969C16, "4177C722-9E92-4AAB-8644-43502BFD5506", "Android-x86 Recovery" },
	{ 0xC2B69556, "EF32A33B-A409-486C-9141-9FFB711F6266", "Android-x86 Misc" },
	{ 0xD34FF551, "20AC26BE-20B7-11E3-84C5-6CFDB94711E9", "Android-x86 Metadata" },
	{ 0x7923A6EA, "38F428E6-D326-425D-9140-6E0EA133647C", "Android-x86 System" },
	{ 0x427737E7, "A893EF21-E428-470A-9E55-0668FD91A2D9", "Android-x86 Cache" },
	{ 0x37237ADB, "DC76DDA9-5AC1-491C-AF42-A82591580C0D", "Android-x86 Data" },
	{ 0x810B4A92, "EBC597D0-2053-4B15-8B64-E0AAC75F4DB1", "Android-x86 Persistent" },
	{ 0xF14F045A, "C5A0AEEC-13EA-11E5-A1B1-001E67CA0C3C", "Android-x86 Vendor" },
	{ 0x9D0020AD, "BD59408B-4514-490D-BF12-9878D963F378", "Android-x86 Config" },
	{ 0x57EC611F, "8F68CC74-C5E5-48DA-BE91-A0C8C15E9C80", "Android-x86 Factory" },
	{ 0x1B09DB30, "9FDAA6EF-4B3F-40D2-BA8D-BFF16BFB887B", "Android-x86 Factory (alt)" },
	{ 0x19B8B344, "767941D0-2085-11E3-AD3B-6CFDB94711E9", "Android-x86 Fastboot/Tertiary" },
	{ 0xCE907A9F, "AC6D7924-EB71-4DF8-B48D-E267B27148FF", "Android-x86 OEM" },
	// Android
	{ 0x3066CD72, "19A710A2-B3CA-11E4-B026-10604B889DCF", "Android Meta" },
	{ 0x41AA0D1B, "193D1EA4-B3CA-11E4-B075-10604B889DCF", "Android EXT" },
	// ONIE
	{ 0x85F1605D, "7412F7D5-A156-4B13-81DC-867174929325", "ONIE Boot" },
	{ 0xE1D5C3A2, "D4E6E2CD-4469-46F3-B5CB-1BFF57AFC149", "ONIE Config" },
	// PowerPC
	{ 0x57D8C5E7, "9E1A2D38-C612-4316-AA26-8B49521E5A8B", "PowerPC PReP Boot" },
	// freedesktop.org
	{ 0xAA5B0BB4, "BC13C2FF-59E6-4262-A352-B275FD6F7172", "freedesktop.org Shared Boot Loader Configuration" },
	// Atari TOS
	{ 0x67AA0DD3, "734E5AFE-F61A-11E6-BC64-92361F002671", "Atari TOS Basic Data Partition" },
	// VeraCrypt
	{ 0xAD1F50BC, "8C8F8EFF-AC95-4770-814A-21994F2DBC8F", "VeraCrypt Encrypted Data Partition" },
	// Empty marker
	{ 0, NULL, NULL }
};

const char* gpt_part_type_str(UID *uid) {
	uint32_t hash = hash_compute((char*)uid, 16);

	const struct gpt_i_entry_t *entry = (struct gpt_i_entry_t*)gpt_i_types;
	while (entry->hash) {
		if (hash == entry->hash)
			return entry->name;
		++entry;
	}

	return NULL;
}
