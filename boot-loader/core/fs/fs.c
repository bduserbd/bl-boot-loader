#include "include/export.h"
#include "core/include/fs/fs.h"
#include "core/include/memory/heap.h"

static struct bl_fs *bl_fs_list = NULL;

bl_fs_handle_t bl_fs_try_mount(struct bl_storage_device *disk, struct bl_partition *partition)
{
	bl_status_t status;
	struct bl_fs *fs;
	bl_fs_handle_t handle;

	handle = bl_heap_alloc(sizeof(*handle));
	if (!handle)
		return NULL;

	fs = bl_fs_list;
	while (fs) {
		if (fs->mount) {
			status = fs->mount(disk, partition, &handle->info);
			if (!status) {
				handle->fs = fs;
				return handle;
			}
		}

		fs = fs->next;
	}

	return NULL;
}

void bl_fs_register(struct bl_fs *fs)
{
	fs->next = bl_fs_list;
	bl_fs_list = fs;
}
BL_EXPORT_FUNC(bl_fs_register);

void bl_fs_unregister(struct bl_fs *fs)
{

}
BL_EXPORT_FUNC(bl_fs_unregister);

