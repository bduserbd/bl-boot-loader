#ifndef BL_MBR_H
#define BL_MBR_H

#include "include/bl-types.h"

/* MBR signature. */
#define BL_MBR_SIGNATURE	0xaa55

/* Partition activeness */
#define BL_MBR_ACTIVE_PARTITION		0x80
#define BL_MBR_INACTIVE_PARTITION	0x00

/* DOS partition types */
enum {
	BL_MBR_PARTITION_TYPE_NONE		= 0x00,
	BL_MBR_PARTITION_TYPE_FAT12		= 0x01,
	BL_MBR_PARTITION_TYPE_EXTENDED		= 0x05,
	BL_MBR_PARTITION_TYPE_NTFS	        = 0x07,
	BL_MBR_PARTITION_TYPE_FAT32		= 0x0b,
	BL_MBR_PARTITION_TYPE_FAT32_LBA		= 0x0c,
	BL_MBR_PARTITION_TYPE_FAT16_LBA		= 0x0e,
	BL_MBR_PARTITION_TYPE_LINUX_SWAP	= 0x82,
	BL_MBR_PARTITION_TYPE_EXT2FS		= 0x83,
	BL_MBR_PARTITION_TYPE_LINUX_EXTENDED	= 0x85,
	BL_MBR_PARTITION_TYPE_FREEBSD		= 0xa5,
	BL_MBR_PARTITION_TYPE_OPENBSD		= 0xa6,
	BL_MBR_PARTITION_TYPE_NETBSD		= 0xa9,
	BL_MBR_PARTITION_TYPE_GPT_DISK		= 0xee,
};

/* The MBR. */
struct bl_mbr_partition_entry {
	__u8 active_flag;

	/* CHS start (unused) */
	__u8 start_head;
	__u8 start_sector;
	__u8 start_cylinder;

	__u8 partition_type;

	/* CHS end (unused) */
	__u8 end_head;
	__u8 end_sector;
	__u8 end_cylinder;

	/* Partition start (LBA)  */
	__u32 start;

	/* Partition length (sectors) */
	__u32 length;
} __attribute__ ((packed));

struct bl_mbr {
	__u8 boot_code[446];
	struct bl_mbr_partition_entry entries[4];
	__u16 signature;
} __attribute__ ((packed));

#endif

