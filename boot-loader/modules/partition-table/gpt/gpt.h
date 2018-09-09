#ifndef BL_GPT_H
#define BL_GPT_H

#include "include/bl-types.h"

#define BL_GPT_SIGNATURE	((bl_uint8_t *)"EFI PART")

/* GPT header. */
struct bl_gpt_header {
	__u8		signature[8];
	__u32		revision;
	__u32		header_size;
	__u32		crc32;
	__u32		reserved;
	__u64		current_lba;
	__u64		backup_lba;
	__u64		first_usable_lba;
	__u64		last_usable_lba;
	bl_guid_t	guid;
	__u64		partitions_lba;
	__u32		partitions_count;
	__u32		partition_entry_size;
	__u32		partitions_crc32;
} __attribute__((packed));

/* Empty GPT partition. */
#define BL_GPT_UNUSED_ENTRY	\
	{ 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} }

/* GPT partition entry. */
struct bl_gpt_partition_entry {
	bl_guid_t	partition_type_guid;
	bl_guid_t	unique_guid;
	__u64		first_lba;
	__u64		last_lba;
	__u64		attributes;
	__u16		name[36];
} __attribute__((packed));

#endif

