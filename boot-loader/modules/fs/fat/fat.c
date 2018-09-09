#include "fat.h"
#include "include/bl-utils.h"
#include "include/export.h"
#include "include/string.h"
#include "include/mbr.h"
#include "core/include/loader/loader.h"
#include "core/include/storage/storage.h"
#include "core/include/fs/fs.h"
#include "core/include/memory/heap.h"

BL_MODULE_NAME("File Allocation Table (12/16/32)");

/* Observation of whole FAT. */
struct bl_fat_info {
	int fat_type;

	struct bl_storage_device *disk;

	bl_uint32_t sector_size;
	bl_uint32_t cluster_size;
	bl_uint32_t eof;

	bl_uint64_t total_sectors;
	bl_uint64_t total_clusters;

	bl_uint64_t fat_lba;
	bl_uint64_t sectors_per_fat;

	/* Good with FAT16 & FAT32. With FAT12 entries are overlapped
	   (within sectors) so add one or two bytes for allocation. */
	void *fat_cache;
	bl_uint32_t fat_cache_size;
	bl_uint32_t fat_cache_sector;

	bl_uint64_t rootdir_lba;
	bl_uint16_t rootdir_entries;

	bl_uint64_t data_lba;
};

#define BL_FAT_SECTORS_PER_CLUSTER(info)	(info->cluster_size / info->sector_size)

struct bl_fat_file_data {
	int rootdir;
	bl_uint32_t cluster;
	struct bl_fat_info *info;
};

static bl_status_t bl_fat_check_regular_name(const char *filename)
{
	int i;

	for (i = 0; filename[i]; i++)
		switch (filename[i]) {
		case '\"': case '*': case '/':
		case ':': case '<': case '>':
		case '?': case '\\': case '|':
		case 127:
		case 1 ... 31: 
			return BL_STATUS_INVALID_FILE_NAME;

		default:
			break;
		}

	return BL_STATUS_SUCCESS;
}

static void bl_fat_short_name_free(char *filename)
{
	bl_heap_free(filename, 11 + sizeof('.'));
}

static char *bl_fat_short_name_to_regular_name(struct bl_fat_dir_entry *entry)
{
	int i;
	int length1, length2;
	char *s;

	s = bl_heap_alloc(sizeof(entry->name) + sizeof('.'));
	if (!s)
		return NULL;

	/* Short file name. */
	for (i = BL_FAT_SHORT_FILE_NAME_LENGTH - 1; i >= 0 && entry->name[i] ==
		BL_FAT_NAME_PADDING; i--) ;

	length1 = i + 1;
	for (i = 0; i < length1; i++)
		s[i] = entry->name[i];

	/* Short file extension. */
	for (i = BL_FAT_SHORT_FILE_EXTENSION_LENGTH - 1; i >= 0 && entry->name[i +
		BL_FAT_SHORT_FILE_NAME_LENGTH] == BL_FAT_NAME_PADDING; i--) ;

	if (i < 0)
		return s;
	else
		s[length1++] = '.';

	length2 = i + 1;
	for (i = 0; i < length2; i++)
		s[length1 + i] = entry->name[i + BL_FAT_SHORT_FILE_NAME_LENGTH];

	return s;
}

static bl_status_t bl_fat_read_rootdir(struct bl_fat_info *info, void *buf,
	bl_size_t size, bl_offset_t offset)
{
	return bl_storage_device_read(info->disk, (void *)buf, info->rootdir_lba +
		(offset >> bl_log2(info->sector_size)), size, offset & (info->sector_size - 1));
}

/* Within the data region. */
static inline bl_uint32_t bl_fat_entry_to_data_sector(struct bl_fat_info *info,
	bl_uint32_t fat_entry)
{
	return info->data_lba + (fat_entry - 2) * BL_FAT_SECTORS_PER_CLUSTER(info);
}

/* Within the FAT itself. */
static inline bl_uint32_t bl_fat_entry_to_sector(struct bl_fat_info *info, bl_uint32_t fat_entry)
{
	return info->fat_lba + fat_entry / ((info->fat_cache_size << 3) /
		(bl_uint32_t)info->fat_type);
}

static bl_uint32_t bl_fat_get_next_entry(struct bl_fat_info *info, bl_uint32_t fat_entry)
{
	bl_status_t status;
	bl_uint32_t fat_sector;

	fat_sector = bl_fat_entry_to_sector(info, fat_entry);
	if (fat_sector != info->fat_cache_sector) {
		status = bl_storage_device_read(info->disk, info->fat_cache, fat_sector,
			info->fat_cache_size, 0);
		if (status)
			return 0;

		info->fat_cache_sector = fat_sector;
	}

	switch (info->fat_type) {
	case BL_FAT12_TYPE:
		return ((bl_uint16_t *)info->fat_cache)[fat_entry] & 0xfff;

	case BL_FAT16_TYPE:
		return ((bl_uint16_t *)info->fat_cache)[fat_entry];

	case BL_FAT32_TYPE:
		return ((bl_uint32_t *)info->fat_cache)[fat_entry];
	}

	return 0;
}

static bl_status_t bl_fat_read_cluster_chain(struct bl_fat_file_data *fdata, void *buf,
	bl_size_t size, bl_offset_t offset)
{
	bl_uint32_t i;
	bl_status_t status;
	bl_uint32_t fat_entry;
	bl_uint32_t read_bytes, single_read_size;
	struct bl_fat_info *info;

	info = fdata->info;

	fat_entry = fdata->cluster;
	for (i = 0; i < offset / info->cluster_size; i++) {
		fat_entry = bl_fat_get_next_entry(info, fat_entry);
		if (!fat_entry || fat_entry == info->eof)
			return BL_STATUS_FILE_NOT_FOUND;
	}

	read_bytes = 0;
	while (1) {
		//bl_print_hex(fat_entry);
		//bl_print_str(" ");

		single_read_size = BL_MIN(size, info->cluster_size);

		status = bl_storage_device_read(info->disk, (bl_uint8_t *)buf + read_bytes,
			bl_fat_entry_to_data_sector(info, fat_entry), single_read_size,
			offset % info->cluster_size);
		if (status)
			return status;

		size -= single_read_size;
		if (size == 0)
			break;

		read_bytes += single_read_size;

		fat_entry = bl_fat_get_next_entry(info, fat_entry);
		if (!fat_entry || fat_entry == info->eof)
			return BL_STATUS_FILE_NOT_FOUND;
	}

	//bl_print_str("\n");

	return BL_STATUS_SUCCESS;
}

/* Perform raw read of data from FAT. We distinguish reads of FAT12 & FAT16
   (where root directory has its separate region) from those of FAT32. */
static bl_status_t bl_fat_generic_read(struct bl_fat_file_data *fdata, void *buf,
	bl_size_t size, bl_offset_t offset)
{
	if (fdata->rootdir && (fdata->info->fat_type == BL_FAT12_TYPE ||
		fdata->info->fat_type == BL_FAT16_TYPE))
		return bl_fat_read_rootdir(fdata->info, buf, size, offset);
	else
		return bl_fat_read_cluster_chain(fdata, buf, size, offset);
}

struct bl_fat_iterator {
	int type;
	char *filename;

	int empty;

	int entry;
	bl_uint32_t cluster;

	struct bl_fat_file_data *fdata;
};

static bl_status_t bl_fat_iterator_next(struct bl_fat_iterator *it)
{
	bl_status_t status;
	struct bl_fat_dir_entry entry;

	/* Read directory entry. */
	status = bl_fat_generic_read(it->fdata, (void *)&entry, sizeof(struct bl_fat_dir_entry),
		(it->entry - 1) * sizeof(struct bl_fat_dir_entry));
	if (status)
		return status;

	/* Have we reached the end ? */
	if (entry.name[0] == BL_FAT_AVAILABLE_ENTRY_FLAG) {
		it->empty = 1;
		return BL_STATUS_SUCCESS;
	}

	/* File type & name manipulation. */
	if ((entry.attributes & BL_FAT_DIR_ENTRY_ATTR_VOLUME) == 0) {
		if (entry.attributes & BL_FAT_DIR_ENTRY_ATTR_DIR)
			it->type = BL_FILE_TYPE_DIRECTORY;
		else
			it->type = BL_FILE_TYPE_REGULAR;

		if (it->filename)
			bl_fat_short_name_free(it->filename);

		it->filename = bl_fat_short_name_to_regular_name(&entry);
		if (!it->filename)
			return BL_STATUS_FAILURE;
	} else {
		it->type = BL_FILE_TYPE_UNKNOWN;
		it->filename = NULL;
	}

	/* Cluster. */
	it->cluster = entry.start;

	it->entry++;

	return BL_STATUS_SUCCESS;
}

static int bl_fat_iterator_end(struct bl_fat_iterator *it)
{
	return !!it->empty;
}

static void bl_fat_iterator_uninit(struct bl_fat_iterator *it)
{
	if (it->filename)
		bl_fat_short_name_free(it->filename);

	bl_memset(it, 0, sizeof(struct bl_fat_iterator));
}

static void bl_fat_iterator_init(struct bl_fat_file_data *fdata, struct bl_fat_iterator *it)
{
	it->type = BL_FILE_TYPE_UNKNOWN;
	it->filename = NULL;

	it->empty = 0;

	it->entry = 1;

	it->fdata = fdata;
}

static bl_status_t bl_fat_iterate_directory_callback(const char *filename, int directory,
	bl_file_data_t dirdata, struct bl_file_tree_node **node)
{
	bl_status_t status;
	struct bl_fat_iterator it;
	struct bl_fat_file_data *fdata;
	struct bl_file_tree_node *_node;

	_node = NULL;
	*node = NULL;

	status = bl_fat_check_regular_name(filename);
	if (status)
		return status;

	fdata = dirdata;

	bl_fat_iterator_init(fdata, &it);

	for ((status = bl_fat_iterator_next(&it)); !bl_fat_iterator_end(&it);
		(status = bl_fat_iterator_next(&it))) {
		/* Verify file. */
		if (status)
			goto _exit;

		if (directory && it.type != BL_FILE_TYPE_DIRECTORY) {
			status = BL_STATUS_INVALID_FILE_TYPE;
			goto _exit;
		}

		if (it.type != BL_FILE_TYPE_UNKNOWN && !bl_strcasecmp(it.filename, filename)) {
			//bl_print_str(it.filename);
			//bl_print_str(" ");

			/* Construct tree node. */
			_node = bl_heap_alloc(sizeof(struct bl_file_tree_node));
			if (!_node) {
				status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
				goto _exit;
			}

			_node->fdata = NULL;
			_node->fdata = bl_heap_alloc(sizeof(struct bl_fat_file_data));
			if (!_node->fdata) {
				status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
				goto _exit;
			}

			((struct bl_fat_file_data *)_node->fdata)->rootdir = 0;
			((struct bl_fat_file_data *)_node->fdata)->cluster = it.cluster;
			((struct bl_fat_file_data *)_node->fdata)->info = fdata->info;

			*node = _node;
			_node = NULL;

			break;
		}
	}

	//if (!node) {
	//	return BL_STATUS_FILE_NOT_FOUND;
	//}

	status = BL_STATUS_SUCCESS;

_exit:
	if (_node) {
		bl_heap_free(_node, sizeof(struct bl_file_tree_node));
		if (_node->fdata)
			bl_heap_free(_node->fdata, sizeof(struct bl_fat_file_data));
	}

	bl_fat_iterator_uninit(&it);

	return status;
}

static bl_status_t bl_fat_open(bl_fs_handle_t handle, const char *path, bl_file_t *file)
{
	bl_status_t status;
	struct bl_file_tree_node *root = NULL;

	/* Prepare root node. */
	root = bl_heap_alloc(sizeof(struct bl_file_tree_node));
	if (!root) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	root->fdata = NULL;
	root->prev = NULL;
	
	root->fdata = bl_heap_alloc(sizeof(struct bl_fat_file_data));
	if (!root->fdata) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	((struct bl_fat_file_data *)root->fdata)->rootdir = 1;
	((struct bl_fat_file_data *)root->fdata)->info = (struct bl_fat_info *)handle->info;


	/* Iterate FAT. */
	status = bl_file_iterate_path(handle->fs, path, root,
		&bl_fat_iterate_directory_callback, &file->fdata);

_exit:
	if (root) {
		if (root->fdata)
			bl_heap_free(root->fdata, sizeof(struct bl_fat_file_data));

		bl_heap_free(root, sizeof(struct bl_file_tree_node));
	}

	return status;
}

static bl_status_t bl_fat_close(bl_file_data_t fdata)
{
	if (!fdata)
		return BL_STATUS_INVALID_PARAMETERS;

	((struct bl_fat_file_data *)fdata)->rootdir = -1;
	((struct bl_fat_file_data *)fdata)->cluster = (bl_uint32_t)-1;
	((struct bl_fat_file_data *)fdata)->info = NULL;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_fat_check_fat(struct bl_storage_device *disk, struct bl_partition *partition,
	struct bl_fat_info *info)
{
	bl_status_t status;
	struct bl_fat_vbr vbr;

	info->disk = disk;

	status = bl_storage_device_read(info->disk, (bl_uint8_t *)&vbr, partition->lba,
		sizeof(struct bl_fat_vbr), 0);
	if (status)
		return status;

	/* Is this realy FAT ? */
	if (!vbr.bpb.num_of_fats || !vbr.bpb.rootdir_entries || vbr.signature != BL_MBR_SIGNATURE ||
		bl_log2(vbr.bpb.bytes_per_sector) == -1 || bl_log2(vbr.bpb.sectors_per_cluster) == -1)
		return BL_STATUS_INVALID_FILE_SYSTEM;

	/* Units. */
	info->sector_size = vbr.bpb.bytes_per_sector;
	info->cluster_size = info->sector_size * vbr.bpb.sectors_per_cluster;

	/* Total sectors. */
	if (vbr.bpb.total_sectors16)
		info->total_sectors = vbr.bpb.total_sectors16;
	else
		info->total_sectors = vbr.bpb.total_sectors32;

	/* File allocation table info. */
	info->fat_lba = partition->lba + vbr.bpb.reserved_sectors;
	if (vbr.bpb.sectors_per_fat16)
		info->sectors_per_fat = vbr.bpb.sectors_per_fat16;
	else
		info->sectors_per_fat = vbr.ebpb32.sectors_per_fat32;

	/* Root directory. */
	info->rootdir_lba = info->fat_lba + info->sectors_per_fat * vbr.bpb.num_of_fats;
	info->rootdir_entries = vbr.bpb.rootdir_entries;

	/* Data. */
	info->data_lba = info->rootdir_lba + BL_MEMORY_ALIGN_UP(info->rootdir_entries *
		sizeof(struct bl_fat_dir_entry), BL_STORAGE_SECTOR_SIZE) /
		BL_STORAGE_SECTOR_SIZE;

	/* Total clusters. */
	/* Divide 64-bit numbers using log2 (`__udivdi3' is not used). */
	info->total_clusters = (partition->lba + info->total_sectors - info->data_lba) >>
		bl_log2(vbr.bpb.sectors_per_cluster);

	/* FAT type. */
	if (info->total_clusters < BL_FAT12_MAX_CLUSTERS) {
		info->fat_type = BL_FAT12_TYPE;
		info->eof = BL_FAT12_EOF;
	} else if (info->total_clusters < BL_FAT16_MAX_CLUSTERS) {
		info->fat_type = BL_FAT16_TYPE;
		info->eof = BL_FAT16_EOF;
	} else {
		info->fat_type = BL_FAT32_TYPE;
		info->eof = BL_FAT32_EOF;
	}

	/* FAT cache. */
	info->fat_cache_size = info->sector_size;
	if (info->fat_type == BL_FAT12_TYPE)
		info->fat_cache_size += 3 - info->sector_size % 3;

	info->fat_cache = bl_heap_alloc(info->fat_cache_size);
	if (!info->fat_cache)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	info->fat_cache_sector = (bl_uint32_t)-1;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_fat_mount(struct bl_storage_device *disk, struct bl_partition *partition,
	bl_fs_info_t *info)
{
	bl_status_t status;
	struct bl_fat_info *_info;

	_info = bl_heap_alloc(sizeof(struct bl_fat_info));
	if (!_info)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	status = bl_fat_check_fat(disk, partition, _info);
	if (status)
		return status;

	*info = _info;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_fat_umount(bl_fs_info_t info)
{
	struct bl_fat_info *_info;

	if (!info)
		return BL_STATUS_INVALID_PARAMETERS;

	_info = info;

	bl_heap_free(_info->fat_cache, _info->fat_cache_size);

	bl_memset(_info, 0, sizeof(struct bl_fat_info));

	return BL_STATUS_SUCCESS;
}

static struct bl_fs bl_fat_fs = {
	.mount = bl_fat_mount,
	.umount = bl_fat_umount,
	.open = bl_fat_open,
	.close = bl_fat_close,
};

BL_MODULE_INIT()
{
	bl_fs_register(&bl_fat_fs);
}

BL_MODULE_UNINIT()
{

}

