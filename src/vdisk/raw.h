struct VDISK;

int vdisk_raw_read_lba(struct VDISK *vd, void *buffer, uint64_t index);