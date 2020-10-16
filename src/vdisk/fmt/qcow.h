/**
 * QCOW: QEMU Copy-On-Write disk
 * 
 * https://github.com/qemu/qemu/blob/master/docs/interop/qcow2.txt
 */

#include <stdint.h>

// QCOW header structure
typedef struct {
	// 
	uint32_t magic;
	// 
	uint32_t version;
	// 
	uint64_t backing_file_offset;
	// 
	uint32_t backing_file_size;
	// 
	uint32_t cluster_bits;
	// In bytes
	uint64_t size;
	// Encryption method
	uint32_t crypt_method;
	// L1 table size in bytes
	uint32_t l1_size;
	// L1 table offset in bytes
	uint64_t l1_offset;
	// 
	uint64_t refcount_table_offset;
	//
	uint32_t refcount_table_clusters;
	// 
	uint32_t nb_snapshots;
	// 
	uint64_t snapshots_offset;
} QCOW_HDR;

struct VDISK;

int vdisk_qcow_open(struct VDISK *vd, uint32_t flags, uint32_t internal);
