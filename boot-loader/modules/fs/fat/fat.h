#ifndef BL_FAT_H
#define BL_FAT_H

#include "include/bl-types.h"

/* FAT 12/16/32. */
enum {
	BL_FAT12_TYPE	= 12,
	BL_FAT16_TYPE	= 16,
	BL_FAT32_TYPE	= 32,
};

/* FAT maximum number of clusters. */
enum {
	BL_FAT12_MAX_CLUSTERS	= 0xff5,
	BL_FAT16_MAX_CLUSTERS	= 0xfff5,
	BL_FAT32_MAX_CLUSTERS	= 0xffffff5,
};

/* FAT EOF. */
enum {
	BL_FAT12_EOF	= 0xff8,
	BL_FAT16_EOF	= 0xfff8,
	BL_FAT32_EOF	= 0xffffff8,
};

/* FAT BIOS Parameter Block */
struct bl_fat_bpb {
	__u8	jump_code[3];		/* Offset : 0x000 */
	__u8	oem[8];			/* Offset : 0x003 */
	__u16	bytes_per_sector;	/* Offset : 0x00b */
	__u8	sectors_per_cluster;	/* Offset : 0x00d */
	__u16	reserved_sectors;	/* Offset : 0x00e */
	__u8	num_of_fats;		/* Offset : 0x010 */
	__u16	rootdir_entries;	/* Offset : 0x011 */
	__u16	total_sectors16;	/* Offset : 0x013 */
	__u8	media;			/* Offset : 0x015 */
	__u16	sectors_per_fat16;	/* Offset : 0x016 */
	__u16	sectors_per_track;	/* Offset : 0x018 */
	__u16	num_of_heads;		/* Offset : 0x01a */
	__u32	hidden_sectors;		/* Offset : 0x01c */
	__u32	total_sectors32;	/* offset : 0x020 */
} __attribute__((packed));

/* FAT (12/16) Extended BIOS Paramter Block */
struct bl_fat_12_16_ebpb {
	__u8	physical_drive;			/* Offset : 0x024 */
	__u8	unused;				/* Offset : 0x025 */
	__u8	extended_boot_signature;	/* Offset : 0x026 */
	__u32	volume_id;			/* Offset : 0x027 */
	__u8	label[11];			/* Offset : 0x02b */
	__u8	fs_type[8];			/* Offset : 0x036 */
} __attribute__((packed));

/* FAT 32 Extended BIOS Paramter Block */
struct bl_fat32_ebpb {
	__u32	sectors_per_fat32;		/* Offset : 0x024 */
	__u16	flags;				/* Offset : 0x028 */
	__u16	version;			/* Offset : 0x02a */
	__u32	root_cluster;			/* Offset : 0x02c */
	__u16	fs_info_sector;			/* Offset : 0x030 */
	__u16	boot_sectors_backup_sector;	/* Offset : 0x032 */
	__u8	reserved[12];			/* Offset : 0x034 */
	__u8	physical_drive;			/* Offset : 0x040 */
	__u8	unused;				/* Offset : 0x041 */
	__u8	extended_boot_signature;	/* Offset : 0x042 */
	__u32	volumd_id;			/* Offset : 0x043 */
	__u8	label[11];			/* Offset : 0x047 */
	__u8	fs_type[8];			/* Offset : 0x052 */
} __attribute__((packed));

/* FAT Volume Boot Record */
struct bl_fat_vbr {
	struct	bl_fat_bpb bpb;		/* Offset : 0x000 */

	union {				/* Offset : 0x024 */
		struct bl_fat_12_16_ebpb	ebpb_12_16;
		struct bl_fat32_ebpb		ebpb32;
	};

	__u8	boot_code[420];	
	__u16	signature;		/* Offset : 0x1fe */
} __attribute__((packed));

/* FAT file names. */
#define BL_FAT_SHORT_FILE_NAME_LENGTH		8
#define BL_FAT_SHORT_FILE_EXTENSION_LENGTH	3
#define BL_FAT_NAME_PADDING	' '

#define BL_FAT_AVAILABLE_ENTRY_FLAG	0x00

/* FAT file attributes */
enum {
	BL_FAT_DIR_ENTRY_ATTR_NONE	= 0x00,
	BL_FAT_DIR_ENTRY_ATTR_RO	= 0x01,
	BL_FAT_DIR_ENTRY_ATTR_HIDDEN	= 0x02,
	BL_FAT_DIR_ENTRY_ATTR_SYSTEM	= 0x04,
	BL_FAT_DIR_ENTRY_ATTR_VOLUME	= 0x08,
	BL_FAT_DIR_ENTRY_ATTR_DIR	= 0x10,
	BL_FAT_DIR_ENTRY_ATTR_ARCHIVE	= 0x20,
};

/* FAT Directory Entry */
struct bl_fat_dir_entry {
	__u8	name[11];		/* Offset : 0x00 */
	__u8	attributes;		/* Offset : 0x0b */
	__u8	reserved;		/* Offset : 0x0c */
	__u8	ctime_cs;		/* Offset : 0x0d */
	__u16	ctime;			/* Offset : 0x0e */
	__u16	cdate;			/* Offset : 0x10 */
	__u16	adate;			/* Offset : 0x12 */
	__u16	start_high;		/* Offset : 0x14 */
	__u16	mtime;			/* Offset : 0x16 */
	__u16	mdate;			/* Offset : 0x18 */
	__u16	start;			/* Offset : 0x1a */
	__u32	size;			/* Offset : 0x1c */
} __attribute__((packed));

#endif

