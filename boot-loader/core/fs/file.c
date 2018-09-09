#include "include/string.h"
#include "include/export.h"
#include "core/include/fs/fs.h"
#include "core/include/memory/heap.h"

bl_status_t bl_file_iterate_path(struct bl_fs *fs, const char *path,
	struct bl_file_tree_node *root, bl_fs_iterate_directory_callback_t callback,
	bl_file_data_t **fdata)
{
	bl_size_t len;
	bl_status_t status;
	char *copy_path, *filename, *next_filename;
	struct bl_file_tree_node *top, *node;
	int directory;

	static const char *bl_path_delimiter =  "/";
	static const char *bl_current_directory = ".";
	static const char *bl_previous_directory = "..";

	if (!fs || !path || !root || !callback)
		return BL_STATUS_INVALID_PARAMETERS;

	len = bl_strlen(path);
	copy_path = bl_strndup(path, len);
	if (!copy_path)
		return BL_STATUS_FAILURE;

	top = root;
	filename = bl_strtok(copy_path, bl_path_delimiter);

	while (filename) {
		next_filename = bl_strtok(NULL, bl_path_delimiter);
		if (next_filename)
			directory = 1;
		else {
			if (path[len - 1] == '/')
				directory = 1;
			else
				directory = 0;
		}

		if (!bl_strcmp(filename, bl_previous_directory)) {
			if (top != root) {
				node = top;
				top = node->prev;

				fs->close(node->fdata);
				bl_heap_free(node, sizeof(struct bl_file_tree_node));
			}
		} else if (bl_strcmp(filename, bl_current_directory)) {
			status = callback(filename, directory, top->fdata, &node);
			if (status)
				goto _exit;

			node->prev = top;
			top = node;
		}

		filename = next_filename;
	}

	*fdata = top->fdata;

	status = BL_STATUS_SUCCESS;

_exit:
	if (top) {
		for (node = top->prev; node; node = node->prev) {
			fs->close(node->fdata);
			bl_heap_free(node, sizeof(struct bl_file_tree_node));
		}

		bl_heap_free(top, sizeof(struct bl_file_tree_node));
	}

	bl_heap_free(copy_path, len);

	return status;
}
BL_EXPORT_FUNC(bl_file_iterate_path);

bl_file_t bl_file_open(bl_fs_handle_t handle, const char *path)
{
        bl_status_t status;
        bl_file_t file;

	if (!path || path[0] != '/')
		return NULL;

	if (!handle || !handle->fs || !handle->fs->open || !handle->fs->close)
		return NULL;

        status = handle->fs->open(handle, path, &file);
        if (status)
                return NULL;

	file->handle = handle;

        return file;
}

void bl_file_close(bl_file_t file)
{
	if (!file || !file->handle || !file->handle->fs || !file->handle->fs->close)
		return;

	file->handle->fs->close(file->fdata);
}

bl_status_t bl_file_ls(bl_fs_handle_t handle, const char *path)
{
	bl_file_t file;
	bl_status_t status;

	if (!path || path[0] != '/')
		return BL_STATUS_INVALID_PARAMETERS;

	if (!handle || !handle->fs || !handle->fs->ls)
		return BL_STATUS_INVALID_PARAMETERS;

	file = bl_file_open(handle, path);
	if (!file)
		return BL_STATUS_FILE_NOT_FOUND;

	status = handle->fs->ls(file->fdata);

	bl_file_close(file);

	return status;
}

