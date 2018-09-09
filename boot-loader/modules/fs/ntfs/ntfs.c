#include "ntfs.h"
#include "include/bl-utils.h"
#include "include/string.h"
#include "include/mbr.h"
#include "core/include/loader/loader.h"
#include "core/include/fs/fs.h"
#include "core/include/memory/heap.h"
#include "core/include/video/print.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

BL_MODULE_NAME("New Technology File System");

struct bl_ntfs_info {
	bl_uint64_t lba;
	struct bl_storage_device *disk;

	bl_uint16_t sector_size;
	bl_uint32_t cluster_size;

	bl_uint64_t mft_lba;
	bl_uint16_t mft_record_size;

	struct bl_ntfs_mft_record *mft_root;
	struct bl_ntfs_index_root *iroot;
};

struct bl_ntfs_file_data {
	int dir;
	struct bl_ntfs_index_root *iroot;

	int rootdir; // Is it root directory (MFT record 5) ?

	struct bl_ntfs_mft_record *mft_record;

	struct bl_ntfs_info *info;
};

static inline bl_uint64_t bl_ntfs_lcn_to_lba(struct bl_ntfs_info *info, bl_uint64_t lcn)
{
	return info->lba + (lcn << bl_log2(info->cluster_size / info->sector_size));
}

static inline bl_uint64_t bl_ntfs_mft_record_lba(struct bl_ntfs_info *info, bl_uint64_t record)
{
	return info->mft_lba + (MFT_RECORD(record) << bl_log2(info->mft_record_size /
				info->sector_size));
}

static inline struct bl_ntfs_attribute_header *bl_ntfs_attribute_first(struct bl_ntfs_mft_record *mft_record)
{
	return (bl_uint8_t *)mft_record + mft_record->attributes_offset;
}

/* Within the MFT record ! */
static inline struct bl_ntfs_attribute_header *bl_ntfs_attribute_next(struct bl_ntfs_attribute_header *attr)
{
	return (bl_uint8_t *)attr + attr->total_size;
}

/* Within the MFT record ! */
static inline int bl_ntfs_attribute_end(struct bl_ntfs_attribute_header *attr)
{
	return attr->type == BL_NTFS_ATTR_END;
}

static inline void *bl_ntfs_get_resident_attribute(struct bl_ntfs_attribute_header *attr)
{
	return (bl_uint8_t *)attr + attr->resident.attribute_offset;
}

static inline bl_uint64_t bl_ntfs_get_child_vcn(struct bl_ntfs_index_entry_descriptor *entry)
{
	// if ((entry->flags & BL_NTFS_INDEX_ENTRY_PARENT) == 0)
	//	return;

	return *(bl_uint64_t *)((bl_uint8_t *)entry + entry->total_size - sizeof(bl_uint64_t));
}

/* From $INDEX_ROOT ! */
	static inline struct bl_ntfs_index_entry_descriptor *
bl_ntfs_iroot_get_first_entry(struct bl_ntfs_index_root *iroot)
{
	return (bl_uint8_t *)&iroot->node + iroot->node.entries_offset;
}

/* From $INDEX_ALLOCATION ! */
	static inline struct bl_ntfs_index_entry_descriptor *
bl_ntfs_ialloc_get_first_entry(struct bl_ntfs_index_header *ialloc)
{
	return (bl_uint8_t *)&ialloc->node + ialloc->node.entries_offset;
}

	static inline struct bl_ntfs_index_entry_descriptor *
bl_ntfs_get_next_entry(struct bl_ntfs_index_entry_descriptor *entry)
{
	return (bl_uint8_t *)entry + entry->total_size;
}

static inline bl_uint64_t bl_ntfs_read_bytes(bl_uint8_t *ptr, bl_uint8_t length)
{
	if (length > 8 || length == 0)
		return (bl_uint64_t)-1;

	return *(bl_uint64_t *)ptr & ((1 << (8 * length)) - 1);
}

static void bl_ntfs_print_filename(struct bl_ntfs_file_name *filename)
{
	bl_uint8_t i;
	char s[2];

	s[1] = 0;

	for (i = 0; i < filename->filename_length; i++) {
		s[0] = (char)filename->filename[i];
		bl_print_str(s);
	}
}

/* Assume collation type is 0x1. */
static inline int bl_ntfs_strcmp(const char *name, struct bl_ntfs_file_name *filename)
{
	int i;
	bl_uint8_t length;

	i = 0;
	length = filename->filename_length;

	while (bl_toupper(name[i]) == bl_toupper((char)filename->filename[i]) && --length) i++;

	return bl_toupper(name[i]) - bl_toupper((char)filename->filename[i]);
}

static bl_status_t bl_ntfs_check_attribute_name(struct bl_ntfs_attribute_header *attr,
		const bl_wchar_t *index)
{
	bl_uint16_t *name;

	if (!attr->name_length)
		return BL_STATUS_UNSUPPORTED;

	if (attr->nonresident_flag)
		name = attr->nonresident.name;
	else
		name = attr->resident.name;

	if (!bl_wcsncmp(index, name, attr->name_length))
		return BL_STATUS_SUCCESS;

	return BL_STATUS_FAILURE;
}

static bl_uint64_t bl_ntfs_vcn_to_lcn(struct bl_ntfs_attribute_header *attr, bl_uint64_t vcn)
{
	bl_uint8_t *run;
	bl_uint8_t run_length, run_offset;
	bl_uint64_t cluster;
	bl_uint64_t length, offset;

	vcn = MFT_RECORD(vcn);
	cluster = 0;
	offset = 0;

	run = (bl_uint8_t *)attr + attr->nonresident.data_run_offset;

	while (1) {
		if (!run[0])
			return (bl_uint64_t)-1;

		run_length = run[0] & 0x7;
		run_offset = (run[0] >> 4) & 0x7;

		length = bl_ntfs_read_bytes(1 + run, run_length);
		cluster += length;

		offset += bl_ntfs_read_bytes(1 + run + run_length, run_offset);

		if (vcn <= cluster)
			return offset + vcn - (cluster - length);

		run += 1 + run_length + run_offset;
	}
}

struct bl_ntfs_index_allocation_info {
	bl_uint64_t vcn;
	struct bl_ntfs_info *info;
};

static void *bl_ntfs_get_index_allocation(struct bl_ntfs_attribute_header *attr, void *data)
{
	bl_status_t status;
	bl_uint64_t lcn;
	struct bl_ntfs_index_allocation_info *ialloc;
	struct bl_ntfs_index_header *index;

	status = bl_ntfs_check_attribute_name(attr, BL_NTFS_$I30);
	if (status)
		return NULL;

	if (!data)
		return NULL;

	ialloc = data;

	lcn = bl_ntfs_vcn_to_lcn(attr, ialloc->vcn);

	index = bl_heap_alloc(ialloc->info->iroot->index_record_size);
	if (!index)
		return NULL;

	status = bl_storage_device_read(ialloc->info->disk, (void *)index,
			bl_ntfs_lcn_to_lba(ialloc->info, lcn), ialloc->info->iroot->index_record_size, 0);
	if (status)
		goto _exit;

	if (bl_memcmp((void *)BL_NTFS_INDX, index->magic, sizeof(index->magic)))
		goto _exit;

	return index;

_exit:
	if (index)
		bl_heap_free(index, ialloc->info->iroot->index_record_size);

	return NULL;
}

static void *bl_ntfs_get_index_root(struct bl_ntfs_attribute_header *attr)
{
	bl_status_t status;

	status = bl_ntfs_check_attribute_name(attr, BL_NTFS_$I30);
	if (status)
		return NULL;

	return bl_ntfs_get_resident_attribute(attr);
}

static void *bl_ntfs_get_filename(struct bl_ntfs_attribute_header *attr)
{
	return bl_ntfs_get_resident_attribute(attr);
}

static void *bl_ntfs_get_mft_record_attribute(struct bl_ntfs_mft_record *mft_record,
		bl_uint32_t type, void *data)
{
	struct bl_ntfs_attribute_header *attr;

	for (attr = bl_ntfs_attribute_first(mft_record); !bl_ntfs_attribute_end(attr);
			attr = bl_ntfs_attribute_next(attr)) {
		if (attr->type != type)
			continue;

		if (attr->nonresident_flag)
			switch (attr->type) {
#if 0
				case BL_NTFS_ATTR_DATA:
					return bl_ntfs_data_dump(attr);

#endif
				case BL_NTFS_ATTR_INDEX_ALLOCATION:
					return bl_ntfs_get_index_allocation(attr, data);

				default:
					return NULL;
			}
		else
			switch (attr->type) {
				case BL_NTFS_ATTR_FILE_NAME:
					return bl_ntfs_get_filename(attr);

				case BL_NTFS_ATTR_INDEX_ROOT:
					return bl_ntfs_get_index_root(attr);

				default:
					return NULL;
			}
	}

	return NULL;
}

static inline bl_status_t bl_ntfs_read_mft(struct bl_ntfs_info *info, bl_uint8_t *mft_record,
		bl_uint64_t record)
{
	bl_status_t status;

	status = bl_storage_device_read(info->disk, (void *)mft_record,
			bl_ntfs_mft_record_lba(info, record), info->mft_record_size, 0);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static void *bl_ntfs_read_child_node(struct bl_ntfs_index_entry_descriptor *entry,
		bl_uint8_t *mft_dir, struct bl_ntfs_info *info)
{
	struct bl_ntfs_index_header *index;

	if ((entry->flags & BL_NTFS_INDEX_ENTRY_PARENT) == 0)
		return NULL;

	struct bl_ntfs_index_allocation_info ialloc = {
		.vcn = bl_ntfs_get_child_vcn(entry),
		.info = info,
	};

	void *data = (void *)&ialloc;

	index = bl_ntfs_get_mft_record_attribute(mft_dir, BL_NTFS_ATTR_INDEX_ALLOCATION, data);
	if (!index)
		return NULL;

	return index;
}

static bl_status_t bl_ntfs_iterate_directory_callback(const char *name, int directory,
		bl_file_data_t btree, struct bl_file_tree_node **node)
{
	bl_status_t status;
	struct bl_ntfs_info *info;
	struct bl_ntfs_file_data *fdata;
	struct bl_ntfs_index_entry_descriptor *entry;
	struct bl_ntfs_index_header *index = NULL, *temp;
	struct bl_ntfs_file_name *filename;
	struct bl_ntfs_mft_record *mft_dir, *mft_dir_next = NULL;
	struct bl_file_tree_node *_node = NULL;

	*node = NULL;

	fdata = btree;
	info = fdata->info;

	if (!fdata->dir)
		return BL_STATUS_INVALID_FILE_TYPE;

	mft_dir = fdata->mft_record;
	entry = bl_ntfs_iroot_get_first_entry(fdata->iroot);

#if 0
	bl_print_hex64(entry->undefined);
	bl_print_str(" ");
	bl_print_hex(entry->total_size);
	bl_print_str(" ");
	bl_print_hex(entry->content_size);
	bl_print_str(" ");
	bl_print_hex(entry->flags);
	bl_print_str("|");
#endif

	while (1) {
		if (entry->flags & BL_NTFS_INDEX_ENTRY_PARENT) {
			if (entry->flags & BL_NTFS_INDEX_ENTRY_LAST_ENTRY) {
				temp = index;

				index = bl_ntfs_read_child_node(entry, mft_dir, info);
				if (!index)
					break;

				if (temp) {
					bl_heap_free(temp, info->iroot->index_record_size);
					temp = NULL;
				}

				entry = bl_ntfs_ialloc_get_first_entry(index);

				continue;
			} else {
				filename = &entry->data[0];

				int res;
				res = bl_ntfs_strcmp(name, filename);

				if (res < 0) {
					temp = index;

					index = bl_ntfs_read_child_node(entry, mft_dir, info);
					if (!index)
						break;

					if (temp) {
						bl_heap_free(temp, info->iroot->index_record_size);
						temp = NULL;
					}

					entry = bl_ntfs_ialloc_get_first_entry(index);

					continue;
				} else if (res == 0) {
					goto _match;
				} else { // res > 0

				}
			}
		}

		if (entry->flags & BL_NTFS_INDEX_ENTRY_LAST_ENTRY)
			break;

		filename = &entry->data[0];
		if (!bl_ntfs_strcmp(name, filename)) {
_match:
			if (directory)
				if (!BL_NTFS_IS_DIRECTORY(filename)) {
					status = BL_STATUS_INVALID_FILE_TYPE;
					goto _exit;
				}

			_node = bl_heap_alloc(sizeof(struct bl_file_tree_node));
			if (!_node) {
				status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
				goto _exit;
			}

			_node->prev = NULL;

			_node->fdata = bl_heap_alloc(sizeof(struct bl_ntfs_file_data));
			if (!_node->fdata) {
				status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
				goto _exit;
			}

			((struct bl_ntfs_file_data *)_node->fdata)->dir = BL_NTFS_IS_DIRECTORY(filename);
			((struct bl_ntfs_file_data *)_node->fdata)->rootdir = 0;

			mft_dir_next = bl_heap_alloc(info->mft_record_size);
			if (!mft_dir_next) {
				status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
				goto _exit;
			}
			status = bl_ntfs_read_mft(info, mft_dir_next, entry->undefined);
			//bl_print_hex64(entry->undefined); bl_print_str("|");
			if (status)
				goto _exit;

			((struct bl_ntfs_file_data *)_node->fdata)->mft_record = mft_dir_next;

			if (BL_NTFS_IS_DIRECTORY(filename))
				((struct bl_ntfs_file_data *)_node->fdata)->iroot =
					bl_ntfs_get_mft_record_attribute(mft_dir_next, BL_NTFS_ATTR_INDEX_ROOT, NULL);

			((struct bl_ntfs_file_data *)_node->fdata)->info = info;


			*node = _node;
			_node = NULL;

			//bl_ntfs_print_filename(filename); bl_print_str("|");

			break;
		}

		entry = bl_ntfs_get_next_entry(entry);
	}

	if (*node)
		status = BL_STATUS_SUCCESS;
	else {
		bl_print_str(name);
		bl_print_str(" - File not found\n");
		status = BL_STATUS_FILE_NOT_FOUND;
	}

_exit:
	if (_node) {
		if (_node->fdata)
			bl_heap_free(_node->fdata, sizeof(struct bl_ntfs_file_data));

		bl_heap_free(_node, sizeof(struct bl_file_tree_node));
	}

	if (mft_dir_next)
		bl_heap_free(mft_dir_next, info->mft_record_size);

	if (index)
		bl_heap_free(index, info->iroot->index_record_size);

	return status;
}

static bl_status_t bl_ntfs_open(bl_fs_handle_t handle, const char *path, bl_file_t *file)
{
	bl_status_t status;
	struct bl_file_tree_node *root;
	struct bl_ntfs_info *info;
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

	root->fdata = bl_heap_alloc(sizeof(struct bl_ntfs_file_data));
	if (!root->fdata) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	((struct bl_ntfs_file_data *)root->fdata)->dir = 1;

	((struct bl_ntfs_file_data *)root->fdata)->rootdir = 1;

	info = handle->info;
	((struct bl_ntfs_file_data *)root->fdata)->mft_record = info->mft_root;
	((struct bl_ntfs_file_data *)root->fdata)->iroot = info->iroot;

	((struct bl_ntfs_file_data *)root->fdata)->info = info;

	/* Iterate NTFS. The callback must make sure it frees root node memory. */
	status = bl_file_iterate_path(handle->fs, path, root, &bl_ntfs_iterate_directory_callback,
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

static bl_status_t bl_ntfs_close(bl_file_data_t btree)
{
	struct bl_ntfs_file_data *fdata;

	if (!btree)
		return BL_STATUS_INVALID_PARAMETERS;

	fdata = btree;

	/* The root directory MFT record is stored in file system info structure.*/
	if (!fdata->rootdir)
		bl_heap_free(fdata->mft_record, fdata->info->mft_record_size);

	bl_memset(fdata, 0, sizeof(struct bl_ntfs_file_data));

	return BL_STATUS_SUCCESS;
}

static void bl_ntfs_dump_file_type(struct bl_ntfs_file_name *filename)
{
	if (filename->flags & BL_NTFS_FILE_FLAGS_SYSTEM)
		bl_print_str(" SYSTEM ");
	if (filename->flags & BL_NTFS_FILE_FLAGS_ARCHIVE)
		bl_print_str(" ARCHIVE ");
	if (filename->flags & BL_NTFS_FILE_FLAGS_COMPRESSED)
		bl_print_str(" COMPRESSED ");
	if (filename->flags & BL_NTFS_FILE_FLAGS_ENCRYPTED)
		bl_print_str(" ENCRYPTED ");
	if (filename->flags & BL_NTFS_FILE_FLAGS_DIRECTORY)
		bl_print_str(" DIRECTORY ");
}

static void bl_ntfs_dump_file_time(bl_uint64_t time)
{
	int i;
	bl_uint64_t ns, secs, days, hhmmss, mmdd;
	bl_uint32_t yy; // :(

	static int month_lengths[12] = {
		31, 0 /* 28 for regular year, 29 for leap year*/, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

	static int year_lengths[2] = { 365, 366 };

	bl_divmod64(time, 10000000, &secs, &ns);
	bl_divmod64(secs, 86400, &days, &hhmmss);

	yy = 1601;
	bl_uint64_t d = 0;

#define IS_LEAP_YEAR(yy)	((!(yy % 4) && (yy % 100)) || !(yy % 400))

	while (1) {
		d += year_lengths[IS_LEAP_YEAR(yy)];
		if (d > days)
			break;

		yy++;
	}

	bl_print_decimal(yy);
	bl_print_str("/");

	month_lengths[1] = IS_LEAP_YEAR(yy) ? 29 : 28;

	for (mmdd = 0, i = 0; i < 12; i++) {
		if (mmdd >= (bl_uint32_t)(d - days)) {
			bl_print_decimal(i + 1);
			bl_print_str("/");
			bl_print_decimal((bl_uint32_t)(d - days) - (mmdd - month_lengths[i - 1]));
			bl_print_str(",");

			break;
		}

		mmdd += month_lengths[i];
	}

	bl_print_decimal((bl_uint32_t)hhmmss / 3600);
	bl_print_str(":");
	bl_print_decimal(((bl_uint32_t)hhmmss % 3600) / 60);
	bl_print_str(":");
	bl_print_decimal((bl_uint32_t)hhmmss % 60);
	bl_print_str(".");
	bl_print_decimal64(ns);
}

static void bl_ntfs_dump_file_info(struct bl_ntfs_file_name *filename)
{
	bl_print_str("[-] ");
	bl_ntfs_print_filename(filename);
	bl_ntfs_dump_file_type(filename);

	bl_print_str("C: ");
	bl_ntfs_dump_file_time(filename->creation_time);

	bl_print_str("\n");
}

static void bl_ntfs_b_tree_recursion(struct bl_ntfs_mft_record *mft_dir,
		struct bl_ntfs_info *info, struct bl_ntfs_index_entry_descriptor *entry)
{
	struct bl_ntfs_index_header *index = NULL, *temp;
	struct bl_ntfs_file_name *filename;

	while (1) {
		if (entry->flags & BL_NTFS_INDEX_ENTRY_PARENT) {
			temp = index;

			index = bl_ntfs_read_child_node(entry, mft_dir, info);
			if (!index)
				break;

			if (temp) {
				bl_heap_free(temp, info->iroot->index_record_size);
				temp = NULL;
			}

			entry = bl_ntfs_ialloc_get_first_entry(index);

			bl_ntfs_b_tree_recursion(mft_dir, info, entry);
			return;
		}

		if (entry->flags & BL_NTFS_INDEX_ENTRY_LAST_ENTRY)
			break;

#if 0
		bl_print_hex64(entry->undefined);
		bl_print_str(" ");
		bl_print_hex(entry->total_size);
		bl_print_str(" ");
		bl_print_hex(entry->content_size);
		bl_print_str(" ");
		bl_print_hex(entry->flags);
		bl_print_str("|");
#endif

		filename = &entry->data[0];
		bl_ntfs_dump_file_info(filename);

		entry = bl_ntfs_get_next_entry(entry);
	}
}

static bl_status_t bl_ntfs_ls(bl_file_data_t btree)
{
	struct bl_ntfs_info *info;
	struct bl_ntfs_file_data *fdata;
	struct bl_ntfs_index_entry_descriptor *entry;
	struct bl_ntfs_mft_record *mft_dir;

	if (!btree)
		return BL_STATUS_INVALID_PARAMETERS;

	fdata = btree;
	info = fdata->info;

	if (fdata->dir) {
		mft_dir = fdata->mft_record;
		entry = bl_ntfs_iroot_get_first_entry(fdata->iroot);

		bl_ntfs_b_tree_recursion(mft_dir, info, entry);
	} else
		bl_ntfs_dump_file_info(bl_ntfs_get_mft_record_attribute(fdata->mft_record,
					BL_NTFS_ATTR_FILE_NAME, NULL));

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ntfs_check_ntfs(struct bl_storage_device *disk, struct bl_partition *partition,
		struct bl_ntfs_info *info)
{
	bl_status_t status;
	struct bl_ntfs_vbr vbr;

	info->disk = disk;

	status = bl_storage_device_read(info->disk, (bl_uint8_t *)&vbr, partition->lba,
			sizeof(struct bl_ntfs_vbr), 0);
	if (status)
		return status;

	/* Sanity checks. */
	if (bl_memcmp(vbr.oem, (void *)BL_NTFS_OEM, 8) || vbr.signature != BL_MBR_SIGNATURE ||
			bl_log2(vbr.bytes_per_sector) == -1 || bl_log2(vbr.sectors_per_cluster) == -1)
		return BL_STATUS_INVALID_FILE_SYSTEM;

	/* Basic information. */
	info->lba = partition->lba;

	info->sector_size = vbr.bytes_per_sector;
	info->cluster_size = vbr.sectors_per_cluster * info->sector_size;

	info->mft_lba = bl_ntfs_lcn_to_lba(info, vbr.mft_lcn);

	bl_int8_t clusters = (bl_int8_t)vbr.clusters_per_mft_record;
	if (clusters > 0)
		info->mft_record_size = clusters * info->cluster_size;
	else
		info->mft_record_size = (1 << (-1 * clusters));

	/* Read root directory MFT record (5). */
	info->mft_root = bl_heap_alloc(info->mft_record_size);
	if (!info->mft_root)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	status = bl_ntfs_read_mft(info, (void *)info->mft_root, BL_NTFS_FILE_ROOT);
	if (status)
		return status;

	info->iroot = bl_ntfs_get_mft_record_attribute(info->mft_root, BL_NTFS_ATTR_INDEX_ROOT,
			NULL);
	if (!info->iroot || info->iroot->attribute_type != BL_NTFS_ATTR_FILE_NAME)
		return BL_STATUS_UNSUPPORTED;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ntfs_mount(struct bl_storage_device *disk, struct bl_partition *partition,
		bl_fs_info_t *info)
{
	bl_status_t status;
	struct bl_ntfs_info *_info;

	_info = bl_heap_alloc(sizeof(struct bl_ntfs_info));
	if (!_info)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	status = bl_ntfs_check_ntfs(disk, partition, _info);
	if (status)
		return status;

	*info = _info;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ntfs_umount(bl_fs_info_t info)
{
	struct bl_ntfs_info *_info;

	if (!info)
		return BL_STATUS_INVALID_PARAMETERS;

	_info = info;

	if (_info->mft_root)
		bl_heap_free(_info->mft_root, _info->mft_record_size);

	bl_memset(_info, 0, sizeof(struct bl_ntfs_info));

	return BL_STATUS_SUCCESS;
}

static struct bl_fs bl_ntfs_fs = {
	.mount = bl_ntfs_mount,
	.umount = bl_ntfs_umount,
	.open = bl_ntfs_open,
	.close = bl_ntfs_close,
	.ls = bl_ntfs_ls,
};

BL_MODULE_INIT()
{
	bl_fs_register(&bl_ntfs_fs);
}

BL_MODULE_UNINIT()
{

}

#pragma GCC diagnostic pop

