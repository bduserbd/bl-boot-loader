#include "ext.h"
#include "core/include/fs/fs.h"
#include "core/include/loader/loader.h"
#include "core/include/memory/heap.h"
#include "core/include/video/print.h"

BL_MODULE_NAME("Extended File System");

enum {
	BL_EXT2_TYPE,
	BL_EXT3_TYPE,
	BL_EXT4_TYPE,
};

struct bl_ext_info {
	int type;

	bl_uint64_t lba;
	struct bl_storage_device *disk;

	bl_uint64_t first_data_lba;

	bl_uint32_t block_size;

	bl_uint32_t blocks_count;
	bl_uint32_t blocks_per_group;

#if 0
	bl_uint32_t total_inodes;
#endif

	bl_uint16_t inode_size;
	bl_uint32_t inodes_per_group;

	int groups_count;
	struct bl_ext_group_descriptor *groups;
};

struct bl_ext_file_data {
	struct bl_ext_inode *inode;

	struct bl_ext_info *info;
};

#if 0
#include "include/disk.h"
#include "include/string.h"
#include "include/bios.h"

static inline struct bl_ext2_dir_entry *bl_ext_get_next_dir(
		struct bl_ext2_dir_entry *dir)
{
	return (struct bl_ext2_dir_entry *)(BL_HEAP_ALIGN_UP((bl_addr_t)dir +
				sizeof(struct bl_ext2_dir_entry) + dir->name_len,
				BL_EXT2_DIR_ENTRY_ALIGN));
}

static unsigned int bl_ext_traverse_directory(struct bl_ext2_dir_entry *dir,
		const char *name)
{
	while (dir->inode) {
		/*bl_bios_puts(name);
		  bl_bios_puts(" : ");
		  bl_bios_puts(dir->name);
		  bl_bios_puts(" : ");
		  bl_bios_puthex(dir->inode);
		  bl_bios_puts("\n");*/

		if (!bl_strncmp(name, dir->name, bl_strnlen(name, 255))) {
			return dir->inode;
		}
		dir = bl_ext_get_next_dir(dir);
	}

	return 0;
}

#endif

static inline bl_uint32_t bl_ext_inode_index_block_group(struct bl_ext_info *info, bl_uint32_t inode_number)
{
	return (inode_number - 1) % info->inodes_per_group;
}

static inline bl_uint32_t bl_ext_inode_offset_to_sectors(struct bl_ext_info *info, bl_uint32_t inode_number)
{
	return (info->inode_size * bl_ext_inode_index_block_group(info, inode_number)) /
			BL_STORAGE_SECTOR_SIZE;
}

static inline bl_uint32_t bl_ext_inode_sector_offset(struct bl_ext_info *info, bl_uint32_t inode_number)
{
	return (info->inode_size * bl_ext_inode_index_block_group(info, inode_number)) %
			BL_STORAGE_SECTOR_SIZE;
}

static inline bl_uint32_t bl_ext_block_to_lba(struct bl_ext_info *info, bl_uint32_t block)
{
	return info->lba + block * info->block_size / BL_STORAGE_SECTOR_SIZE;
}

static inline bl_uint32_t bl_ext_inode_block_group(struct bl_ext_info *info, bl_uint32_t inode_number)
{
	return (inode_number - 1) / info->inodes_per_group;
}

static bl_status_t bl_ext_get_inode(struct bl_ext_info *info, bl_uint32_t inode_number,
		struct bl_ext_inode *inode)
{
	bl_uint32_t block_group;

	block_group = bl_ext_inode_block_group(info, inode_number);

	return bl_storage_device_read(info->disk, (bl_uint8_t *)inode,
			bl_ext_block_to_lba(info, info->groups[block_group].inode_table) +
			bl_ext_inode_offset_to_sectors(info, inode_number),
			sizeof(struct bl_ext_inode), bl_ext_inode_sector_offset(info, inode_number));
}

static bl_status_t bl_ext_get_block(struct bl_ext_info *info, bl_uint32_t block, bl_uint8_t *data)
{
	return bl_storage_device_read(info->disk, data, bl_ext_block_to_lba(info, block),
			info->block_size, 0);
}

#if 0
enum {
	BL_EXT_BLOCK_DIRECT_MAP,
	BL_EXT_BLOCK_INDIRECT_MAP,
	BL_EXT_BLOCK_DOUBLE_INDIRECT_MAP,
	BL_EXT_BLOCK_TRIPLE_INDIRECT_MAP,
};

struct bl_ext_iterator {
	int mapping;

	bl_uint32_t direct_block_index;

#if 0
	bl_uint32_t indirect_block_index;
	bl_uint32_t double_indirect_block_index;
	bl_uint32_t triple_indirect_block_index;
#endif

	bl_uint8_t *content;

	struct bl_ext_info *info;

	struct bl_ext_inode *inode;
};

static bl_status_t bl_ext_iterator_init(struct bl_ext_info *info, struct bl_ext_inode *inode,
	struct bl_ext_iterator *it)
{
	it->mapping = BL_EXT_BLOCK_DIRECT_MAP;

	it->direct_block_index = 0;

#if 0
	it->indirect_block_index = 0;
	it->double_indirect_block_index = 0;
	it->triple_indirect_block_index = 0;
#endif

	it->content = bl_heap_alloc(info->block_size);
	if (!it->content)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	it->info = info;

	it->inode = inode;

	return BL_STATUS_SUCCESS;
}

static void bl_ext_iterator_uninit(struct bl_ext_iterator *it)
{
	if (it->content) {
		bl_heap_free(it->content, it->info->block_size);
		it->content = NULL;
	}

	bl_memset(it, 0, sizeof(struct bl_ext_iterator));
}

static bl_status_t bl_ext_iterator_next(struct bl_ext_iterator *it)
{
	bl_status_t status;

	if (it->mapping == BL_EXT_BLOCK_DIRECT_MAP) {
		if (it->direct_block_index == 12)
			it->mapping = BL_EXT_BLOCK_INDIRECT_MAP;
		else {
			bl_print_str("\n");
			bl_print_hex(it->inode->block[it->direct_block_index]);

			status = bl_ext_get_block(it->info, it->inode->block[1],
					it->content);
			if (status)
				return status;

			it->direct_block_index++;
		}
	}

	if (it->mapping == BL_EXT_BLOCK_INDIRECT_MAP)
		return BL_STATUS_FAILURE;

	return BL_STATUS_SUCCESS;
}
#endif

static inline void *bl_ext4_get_first_extent_node(struct bl_ext_extent_header *eheader)
{
	return (bl_uint8_t *)eheader + sizeof(struct bl_ext_extent_header);
}

static bl_status_t bl_ext4_iterate_directory(const char *name, int directory,
		struct bl_ext_file_data *fdata, struct bl_file_tree_node **node)
{
	struct bl_ext_info *info;
	struct bl_ext_extent_header *eheader;

	union {
		struct bl_ext_extent *eextent;
		struct bl_ext_extent_idx *eindex;
	};

	info = fdata->info;

	eheader = &fdata->inode->eheader;

	while (1) {
		bl_print_hex(eheader->magic);
		bl_print_str("\n");

		if (eheader->depth) {

		} else {

		}

		break;
	}

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ext_iterate_directory_callback(const char *name, int directory,
		bl_file_data_t tree, struct bl_file_tree_node **node)
{
	bl_status_t status;
	struct bl_ext_file_data *fdata;

	fdata = tree;

	if (fdata->info->type == BL_EXT4_TYPE) {
		bl_print_str(name); bl_print_str("\n");
		status = bl_ext4_iterate_directory(name, directory, fdata, node);
		if (status)
			return status;
	} else {

	}

	return BL_STATUS_FAILURE;
}

static bl_status_t bl_ext_open(bl_fs_handle_t handle, const char *path, bl_file_t *file)
{
	bl_status_t status;
	struct bl_file_tree_node *root;
	struct bl_ext_inode *inode;
	struct bl_ext_info *info;
	bl_file_data_t *fdata;
	bl_file_t _file;

	/* Prepare root node. */
	root = bl_heap_alloc(sizeof(struct bl_file_tree_node));
	if (!root) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	root->fdata = NULL;
	root->prev = NULL;

	root->fdata = bl_heap_alloc(sizeof(struct bl_ext_file_data));
	if (!root->fdata) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	info = handle->info;
	((struct bl_ext_file_data *)root->fdata)->info = info;

	inode = bl_heap_alloc(sizeof(struct bl_ext_inode));
	if (!inode) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	status = bl_ext_get_inode(info, 2, inode);
	if (status)
		return status;

	((struct bl_ext_file_data *)root->fdata)->inode = inode;;

	/* Iterate EXT. */
	status = bl_file_iterate_path(handle->fs, path, root, &bl_ext_iterate_directory_callback,
			&fdata);
	if (status)
		goto _exit;

	/* Return file handle. */
	_file = bl_heap_alloc(sizeof(*_file));
	if (!_file) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	_file->handle = handle;
	_file->fdata = fdata;

	*file = _file;
	_file = NULL;

_exit:
	return status;
}

static bl_status_t bl_ext_close(bl_file_data_t somedata)
{
	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ext_check_ext(struct bl_storage_device * disk, struct bl_partition *partition,
		struct bl_ext_info *info)
{
	bl_status_t status;
	struct bl_ext_super_block sblock;

	info->disk = disk;

	status = bl_storage_device_read(disk, (bl_uint8_t *)&sblock, partition->lba +
			BL_EXT_SUPER_BLOCK_OFFSET / BL_STORAGE_SECTOR_SIZE,
			sizeof(struct bl_ext_super_block), 0);
	if (status)
		return status;

	/* Sanity checks. */
	if (sblock.magic != BL_EXT_MAGIC)
		return BL_STATUS_INVALID_FILE_SYSTEM;

	if (sblock.inode_size == 0x80) {
		info->type = BL_EXT2_TYPE;
	} else if (sblock.inode_size == 0x100) {
		if ((sblock.feature_incompat & BL_EXT_FEATURE_INCOMPAT_EXTENTS) == 0)
			return BL_STATUS_UNSUPPORTED;

		info->type = BL_EXT4_TYPE;
	} else {
		return BL_STATUS_INVALID_FILE_SYSTEM;
	}

	/* Basic information. */
	info->lba = partition->lba;

	info->block_size = BL_EXT_BLOCK_SIZE(sblock.log_block_size);

	info->blocks_count = sblock.blocks_count;
	info->blocks_per_group = sblock.blocks_per_group;

	info->groups_count = info->blocks_count / info->blocks_per_group +
		((info->blocks_count % info->blocks_per_group) > 0);

	info->groups = bl_heap_alloc(info->groups_count * sizeof(struct bl_ext_group_descriptor));
	if (!info->groups)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	status = bl_storage_device_read(info->disk, (bl_uint8_t *)info->groups, info->lba +
			info->block_size / BL_STORAGE_SECTOR_SIZE, info->groups_count *
			sizeof(struct bl_ext_group_descriptor), 0);
	if (status)
		return status;

	info->inode_size = sblock.inode_size;
	info->inodes_per_group = sblock.inodes_per_group;

	bl_print_hex(sblock.feature_compat);
	bl_print_str(" ");
	bl_print_hex(sblock.feature_incompat);
	bl_print_str(" ");
	bl_print_hex(sblock.feature_ro_compat);
	bl_print_str("\n");

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ext_mount(struct bl_storage_device *disk, struct bl_partition *partition,
		bl_fs_info_t *info)
{
	bl_status_t status;
	struct bl_ext_info *_info;

	_info = bl_heap_alloc(sizeof(struct bl_ext_info));
	if (!_info)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	status = bl_ext_check_ext(disk, partition, _info);
	if (status)
		return status;

	*info = _info;

	return BL_STATUS_SUCCESS;
}

static struct bl_fs ext_fs = {
	.mount = bl_ext_mount,
	.open = bl_ext_open,
	.close = bl_ext_close,
};

BL_MODULE_INIT()
{
	bl_fs_register(&ext_fs);
}

BL_MODULE_UNINIT()
{

}

