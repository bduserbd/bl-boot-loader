#ifndef BL_NTFS_H
#define BL_NTFS_H

#include "include/bl-types.h"

/* NTFS volume identifer. */
#define BL_NTFS_OEM	"NTFS    "

/* NTFS Volume Boot Record - $Boot. */
struct bl_ntfs_vbr {
	__u8	jump_code[3];
	__u8	oem[8];
	__u16	bytes_per_sector;
	__u8	sectors_per_cluster;
	__u8	reserved0[7];
	__u8	media;
	__u16	reserved1;
	__u16	sectors_per_track;
	__u16	num_of_heads;
	__u32	hidden_sectors;
	__u8	reserved2[8];
	__u64	total_sectors;
	__u64	mft_lcn;
	__u64	mft_mirr_lcn;
	__u8	clusters_per_mft_record;
	__u8	reserved3[3];
	__u8	clusters_per_index_record;
	__u8	reserved4[3];
	__u64	volume_serial_number;
	__u32	checksum;
	__u8	boot_code[426];
	__u16	signature;
} __attribute__((packed));

/* MFT file indexes. */
enum {
	BL_NTFS_FILE_MFT	=  0,
	BL_NTFS_FILE_MFTMIRR	=  1,
	BL_NTFS_FILE_LOGFILE	=  2,
	BL_NTFS_FILE_VOLUME	=  3,
	BL_NTFS_FILE_ATTRDEF	=  4,
	BL_NTFS_FILE_ROOT	=  5,
	BL_NTFS_FILE_BITMAP	=  6,
	BL_NTFS_FILE_BOOT	=  7,
	BL_NTFS_FILE_BADCLUS	=  8,
	BL_NTFS_FILE_QUOTA	=  9,
	BL_NTFS_FILE_UPCASE	= 10,
	BL_NTFS_FILE_EXTEND	= 11,
};

#define MFT_RECORD(record)	(record & ((1ULL << 48) - 1))

/* NTFS MFT record. */
struct bl_ntfs_mft_record {
	__u8	magic[4];
	__u16	update_sequence_offset;
	__u16	update_sequence_count;
	__u64	log_file_seq_num;
	__u16	sequence_number;
	__u16	hard_link_count;
	__u16	attributes_offset;
	__u16	flags;
	__u32	record_size;
	__u32	allocated_size;
	__u64	base_mft_record;
	__u16	next_attribute_id;
	__u16	alignment;
	__u32	mft_record_number;
} __attribute__((packed));

/* NTFS Attributes types. */
enum {
	BL_NTFS_ATTR_UNUSED			= 0x00,
	BL_NTFS_ATTR_STANDARD_INFORMATION	= 0x10,
	BL_NTFS_ATTR_ATTRIBUTE_LIST		= 0x20,
	BL_NTFS_ATTR_FILE_NAME			= 0x30,
	BL_NTFS_ATTR_OBJECT_ID			= 0x40,
	BL_NTFS_ATTR_SECURITY_DESCRIPTOR	= 0x50,
	BL_NTFS_ATTR_VOLUME_NAME		= 0x60,
	BL_NTFS_ATTR_VOLUME_INFORMATION		= 0x70,
	BL_NTFS_ATTR_DATA			= 0x80,
	BL_NTFS_ATTR_INDEX_ROOT			= 0x90,
	BL_NTFS_ATTR_INDEX_ALLOCATION		= 0xa0,
	BL_NTFS_ATTR_BITMAP			= 0xb0,
	BL_NTFS_ATTR_SYMLINK			= 0xc0,
	BL_NTFS_ATTR_EA_INFORMATION		= 0xd0,
	BL_NTFS_ATTR_EA				= 0xe0,
	BL_NTFS_ATTR_PROPERTY_SET		= 0xf0,
	BL_NTFS_ATTR_LOGGED_UTILITY_STREAM	= 0x100,
	BL_NTFS_ATTR_END			= 0xffffffff,
};

/* NTFS attribute name of an ordinary directory index. */
#define BL_NTFS_$I30	L"$I30"

/* NTFS Attribute header. */
struct bl_ntfs_attribute_header {
	__u32	type;
	__u32	total_size;
	__u8	nonresident_flag;
	__u8	name_length;
	__u16	name_offset;
	__u16	flags;
	__u16	attribute_id;

	union {
		struct {
			__u32	attribute_length;
			__u16	attribute_offset;
			__u8	indexed_flag;
			__u8	padding;
			__u16	name[0];
		} resident;

		struct {
			__u64	start_vcn;
			__u64	last_vcn;
			__u16	data_run_offset;
			__u16	compression_unit_size;
			__u32	padding;
			__u64	allocated_size;
			__u64	real_size;
			__u64	initialized_size;
			__u16	name[0];
		} nonresident;
	};
} __attribute__((packed));

/* NTFS $STANDARD_INFORMATION attribute. */
struct bl_ntfs_standard_information {
	__u64	creation_time;
	__u64	data_alteration_time;
	__u64	mft_alteration_time;
	__u64	access_time;
	__u32	flags;
	__u32	maximum_versions;
	__u32	version_number;
	__u32	class_id;
	__u32	owner_id;
	__u32	security_id;
	__u64	quota_charged;
	__u64	update_sequence_number;
} __attribute__((packed));

/* NTFS $FILE_NAME flags. */
enum {
	BL_NTFS_FILE_FLAGS_READ_ONLY		= 0x00000001,
	BL_NTFS_FILE_FLAGS_HIDDEN		= 0x00000002,
	BL_NTFS_FILE_FLAGS_SYSTEM		= 0x00000004,
	BL_NTFS_FILE_FLAGS_ARCHIVE		= 0x00000020,
	BL_NTFS_FILE_FLAGS_DEVICE		= 0x00000040,
	BL_NTFS_FILE_FLAGS_NORMAL		= 0x00000080,
	BL_NTFS_FILE_FLAGS_TEMPORARY		= 0x00000100,
	BL_NTFS_FILE_FLAGS_SPARSE		= 0x00000200,
	BL_NTFS_FILE_FLAGS_REPARSE_POINT	= 0x00000400,
	BL_NTFS_FILE_FLAGS_COMPRESSED		= 0x00000800,
	BL_NTFS_FILE_FLAGS_OFFLINE		= 0x00001000,
	BL_NTFS_FILE_FLAGS_NO_CONTENT_INDEXED	= 0x00002000,
	BL_NTFS_FILE_FLAGS_ENCRYPTED		= 0x00004000,
	BL_NTFS_FILE_FLAGS_DIRECTORY		= 0x10000000,
	BL_NTFS_FILE_FLAGS_INDEX_VIEW		= 0x20000000,
};

/* NTFS $FILE_NAME attribute. */
struct bl_ntfs_file_name {
	__u64	parent_directory;
	__u64	creation_time;
	__u64	file_alteration_time;
	__u64	mft_alteration_time;
	__u64	file_read_time;
	__u64	allocated_size;
	__u64	data_size;
	__u32	flags;
	__u32	unused;
	__u8	filename_length;
	__u8	filename_namespace;
	__u16	filename[0];
} __attribute__((packed));

#define BL_NTFS_IS_DIRECTORY(filename)	\
	(filename->flags & BL_NTFS_FILE_FLAGS_DIRECTORY)

/* NTFS index node header. */
struct bl_ntfs_index_node_header {
	__u32	entries_offset;
	__u32	entries_size;
	__u32	entries_allocated_size;
	__u8	leaf_node;
	__u8	padding[3];
} __attribute__((packed));

/* NTFS collation, sorting rules. */
enum {
	BL_NTFS_COLLATION_BINARY	= 0x00,
	BL_NTFS_COLLATION_FILENAME	= 0x01,
	BL_NTFS_COLLATION_UNICODE	= 0x02,
	BL_NTFS_COLLATION_ULONG		= 0x10,
	BL_NTFS_COLLATION_SID		= 0x11,
	BL_NTFS_COLLATION_SECURITY_HASH	= 0x12,
	BL_NTFS_COLLATION_ULONGS	= 0x13,
};

/* NTFS $INDEX_ROOT attribute. */
struct bl_ntfs_index_root {
	__u32	attribute_type;
	__u32	collation_type;
	__u32	index_record_size;
	__u8	clusters_per_index_record;
	__u8	padding[3];
	struct bl_ntfs_index_node_header node;
} __attribute__((packed));

/* NTFS B-tree node types. */
enum {
	//BL_NTFS_INDEX_ENTRY_LEAF 	= 0x0,
	BL_NTFS_INDEX_ENTRY_PARENT	= 0x1,
	BL_NTFS_INDEX_ENTRY_LAST_ENTRY	= 0x2,
};

/* NTFS generic index entry descriptor. */
struct bl_ntfs_index_entry_descriptor {
	__u64	undefined;
	__u16	total_size;
	__u16	content_size;
	__u8	flags;
	__u8	padding[3];
	__u8	data[0];
} __attribute__((packed));

/* NTFS directory index. */
#define BL_NTFS_INDX	"INDX"

/* NTFS standard index header. Used for $INDEX_ALLOCATION. */
struct bl_ntfs_index_header {
	__u8	magic[4];
	__u16	update_sequence_offset;
	__u16	update_sequence_count;
	__u64	log_file_seq_num;
	__u64	vcn;
	struct bl_ntfs_index_node_header node;
} __attribute__((packed));

#endif

