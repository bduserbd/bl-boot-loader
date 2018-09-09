#ifndef BL_FS_H
#define BL_FS_H

#include "include/bl-types.h"
#include "core/include/storage/storage.h"

struct bl_fs;

/* File system specific info */
typedef void *bl_fs_info_t;

struct bl_fs_handle {
	struct bl_fs *fs;
	bl_fs_info_t info;
};
typedef struct bl_fs_handle *bl_fs_handle_t;

/* File. */
typedef void *bl_file_data_t;

struct bl_file {
	char *name;
	bl_fs_handle_t handle;
	bl_file_data_t fdata;
};
typedef struct bl_file *bl_file_t;


/* File system. */
struct bl_fs {
	bl_status_t (*mount)(struct bl_storage_device *, struct bl_partition *,
		bl_fs_info_t *);

	bl_status_t (*umount)(bl_fs_info_t);

	bl_status_t (*open)(bl_fs_handle_t, const char *, bl_file_t *);

	bl_status_t (*close)(bl_file_data_t);

	/* `ls -al`. Let it be file system proprietary. */
	bl_status_t (*ls)(bl_file_data_t);

	bl_status_t (*read)(void);

	struct bl_fs *next;
};

/* General file type. */
enum {
	BL_FILE_TYPE_UNKNOWN,
	BL_FILE_TYPE_REGULAR,
	BL_FILE_TYPE_DIRECTORY,
};

/* Used for file path iteration. */
struct bl_file_tree_node {
	bl_file_data_t fdata;
	struct bl_file_tree_node *prev;
};

bl_fs_handle_t bl_fs_try_mount(struct bl_storage_device *, struct bl_partition *);

bl_file_t bl_file_open(bl_fs_handle_t, const char *);
void bl_file_close(bl_file_t);

bl_status_t bl_file_ls(bl_fs_handle_t, const char *);

/* Iterate directories callback. */
typedef bl_status_t (*bl_fs_iterate_directory_callback_t)(const char *, int,
	bl_file_data_t, struct bl_file_tree_node **);

bl_status_t bl_file_iterate_path(struct bl_fs *,const char *, struct bl_file_tree_node *,
	bl_fs_iterate_directory_callback_t, bl_file_data_t **);

void bl_fs_register(struct bl_fs *);
void bl_fs_unregister(struct bl_fs *);

#endif

