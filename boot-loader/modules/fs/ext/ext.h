#ifndef BL_EXT_H
#define BL_EXT_H

#include "include/bl-types.h"

/* Support for EXT2/3/4 */

/* EXT file system magic */
#define BL_EXT_MAGIC		0xef53

/* EXT super block offset from partition start */
#define BL_EXT_SUPER_BLOCK_OFFSET	0x400

/* EXT block size */
#define BL_EXT_BLOCK_SIZE(log_block_size)	(1024 << log_block_size)

/* EXT compatible features. */
enum {
	BL_EXT_FEATURE_COMPAT_DIR_PREALLOC	= 0x0001,
	BL_EXT_FEATURE_COMPAT_IMAGIC_INODES	= 0x0002,
	BL_EXT_FEATURE_COMPAT_HAS_JOURNAL	= 0x0004,
	BL_EXT_FEATURE_COMPAT_BL_EXT_ATTR	= 0x0008,
	BL_EXT_FEATURE_COMPAT_RESIZE_INODE	= 0x0010,
	BL_EXT_FEATURE_COMPAT_DIR_INDEX		= 0x0020,
	BL_EXT_FEATURE_COMPAT_SPARSE_SUPER2	= 0x0200,
};

/* EXT incompatible features. */
enum {
	BL_EXT_FEATURE_INCOMPAT_COMPRESSION	= 0x0001,
	BL_EXT_FEATURE_INCOMPAT_FILETYPE	= 0x0002,
	BL_EXT_FEATURE_INCOMPAT_RECOVER		= 0x0004,
	BL_EXT_FEATURE_INCOMPAT_JOURNAL_DEV	= 0x0008,
	BL_EXT_FEATURE_INCOMPAT_META_BG		= 0x0010,
	BL_EXT_FEATURE_INCOMPAT_EXTENTS		= 0x0040,
	BL_EXT_FEATURE_INCOMPAT_64BIT		= 0x0080,
	BL_EXT_FEATURE_INCOMPAT_MMP		= 0x0100,
	BL_EXT_FEATURE_INCOMPAT_FLEX_BG		= 0x0200,
	BL_EXT_FEATURE_INCOMPAT_EA_INODE	= 0x0400,
	BL_EXT_FEATURE_INCOMPAT_DIRDATA		= 0x1000,
	BL_EXT_FEATURE_INCOMPAT_CSUM_SEED	= 0x2000,
	BL_EXT_FEATURE_INCOMPAT_LARGEDIR	= 0x4000,
	BL_EXT_FEATURE_INCOMPAT_INLINE_DIR	= 0x8000,
	BL_EXT_FEATURE_INCOMPAT_ENCRYPT		= 0x10000,
};

/* EXT readonly compatible features. */
enum {
	BL_EXT_FEATURE_RO_COMPAT_SPARSE_SUPER	= 0x0001,
	BL_EXT_FEATURE_RO_COMPAT_LARGE_FILE	= 0x0002,
	BL_EXT_FEATURE_RO_COMPAT_BTREE_DIR	= 0x0004,
	BL_EXT_FEATURE_RO_COMPAT_GDT_CSUM	= 0x0010,
	BL_EXT_FEATURE_RO_COMPAT_DIR_NLINK	= 0x0020,
	BL_EXT_FEATURE_RO_COMPAT_EXTRA_ISIZE	= 0x0040,
	BL_EXT_FEATURE_RO_COMPAT_QUOTA		= 0x0100,
	BL_EXT_FEATURE_RO_COMPAT_BIG_ALLOC	= 0x0200,
	BL_EXT_FEATURE_RO_COMPAT_METADATA_CSUM	= 0x0400,
	BL_EXT_FEATURE_RO_COMPAT_READONLY	= 0x1000,
	BL_EXT_FEATURE_RO_COMPAT_PROJECT	= 0x2000,
};

/* EXT super block */
struct bl_ext_super_block {
	__u32	inodes_count;
	__u32	blocks_count;
	__u32	r_blocks_count;
	__u32	free_blocks_count;
	__u32	free_inodes_count;
	__u32	first_data_block;
	__u32	log_block_size;
	__u32	log_frag_size;
	__u32	blocks_per_group;
	__u32	frags_per_group;
	__u32	inodes_per_group;
	__u32	mtime;
	__u32	wtime;
	__u16	mnt_count;
	__u16	max_mnt_count;
	__u16	magic;
	__u16	state;
	__u16	errors;
	__u16	minor_rev_level;
	__u32	lastcheck;
	__u32	checkinterval;
	__u32	creator_os;
	__u32	rev_level;
	__u16	def_resuid;
	__u16	def_resgid;
	__u32	first_ino;
	__u16	inode_size;
	__u16	block_group_nr;
	__u32	feature_compat;
	__u32	feature_incompat;
	__u32	feature_ro_compat;
	__u8	uuid[16];
	__u8	volume_name[16];
	__u8	last_mounted[64];
	__u32	algo_bitmap;
	__u8	prealloc_blocks;
	__u8	prealloc_dir_blocks;
	__u16	alignment0;
	__u8	journal_uuid[16];
	__u32	journal_inum;
	__u32	journal_dev;
	__u32	last_orphan;
	__u32	hash_seed[4];
	__u8	def_hash_version;
	__u8	padding0[3];
	__u32	default_mount_opts;
	__u32	first_meta_bg;
	__u8	reserved[760];
} __attribute__((packed));

/* EXT group descriptor */
struct bl_ext_group_descriptor {
	__u32	block_bitmap;
	__u32	inode_bitmap;
	__u32	inode_table;
	__u16	free_blocks_count;
	__u16	free_inodes_count;
	__u16	used_dirs_count;
	__u16	pad;
	__u8	reserved[12];
} __attribute__((packed));

/* EXT extent header. */
#define BL_EXT_EXTENT_HEADER_MAGIC	0xf30a

struct bl_ext_extent_header {
	__u16	magic;
	__u16	entries;
	__u16	max;
	__u16	depth;
	__u32	generation;
} __attribute__((packed));

/* EXT extent tree leaf nodes. */
struct bl_ext_extent {
	__u32	block;
	__u16	len;
	__u16	start_hi;
	__u32	start_lo;
} __attribute__((packed));

/* EXT extent index nodes. */
struct bl_ext_extent_idx {
	__u32	block;
	__u32	leaf_lo;
	__u16	leaf_hi;
	__u16	unused;
} __attribute__((packed));

/* Blocks in the EXT inode */
#define BL_EXT_N_BLOCKS	15

/* EXT inode modes. */
enum {
	BL_EXT_INODE_MODE_FIFO		= 0x1000,
	BL_EXT_INODE_MODE_IFCHR		= 0x2000,
	BL_EXT_INODE_MODE_IFDIR		= 0x4000,
	BL_EXT_INODE_MODE_IFBLK		= 0x6000,
	BL_EXT_INODE_MODE_IFREG		= 0x8000,
	BL_EXT_INODE_MODE_IFLNK		= 0xa000,
	BL_EXT_INODE_MODE_IFSOCK	= 0xc000,
};

/* EXT inode. */
struct bl_ext_inode {
	__u16	mode;
	__u16	uid;
	__u32	size_lo;
	__u32	atime;
	__u32	ctime;
	__u32	mtime;
	__u32	dtime;
	__u16	gid;
	__u16	links_count;
	__u32	block_lo;
	__u32	flags;
	__u32	osd1;

	union {
		struct {
			__u32	block[BL_EXT_N_BLOCKS];
		};

		struct {
			struct bl_ext_extent_header	eheader;

			union {
				struct bl_ext_extent		eextent[4];
				struct bl_ext_extent_idx	eindex[4];
			};
		};
	};

	__u32	generation;
	__u32	file_acl_lo;
	__u32	size_high;
	__u32	obso_faddr;
	__u8	osd2[12];
} __attribute__((packed));

/* EXT directory entry memory alignment. */
#define BL_EXT2_DIR_ENTRY_ALIGN	0x4

/* EXT directory entry. */
struct bl_ext_dir_entry {
	__u32	inode;
	__u16	rec_len;
	__u8	name_len;
	__u8	file_type;
	char	name[];
} __attribute__((packed));

#endif

