#include "include/string.h"
#include "include/export.h"
#include "include/error.h"
#include "firmware/uefi/include/tables.h"
#include "firmware/uefi/include/utils.h"
#include "core/include/loader/loader.h"
#include "core/include/memory/heap.h"

#define EFI_MODULES_FILE_PATH	L"\\BLMODLST"

extern void bl_efi_set_console(void);
extern bl_status_t bl_init(void);

extern struct bl_module_list_header *bl_mod_list;

static efi_status_t bl_efi_read_file(struct efi_file_protocol *file, void **addr)
{
	efi_status_t status;
	efi_guid_t file_info_guid = EFI_FILE_INFO_ID_GUID;
	struct efi_file_info file_info;
	void *ptr;

	/* Get boot file info. */
	efi_uintn_t buffer_size = sizeof(struct efi_file_info);
	status = file->get_info(file, &file_info_guid, &buffer_size, &file_info);
	if (EFI_FAILED(status))
		goto _exit;

	/* Read file. Make it 4 KiB page aligned. */
	ptr = bl_heap_alloc_align(file_info.file_size, 0x1000);
	if (!ptr)
		goto _exit;

	efi_uintn_t file_size = file_info.file_size;
	status = file->read(file, &file_size, ptr);
	if (EFI_FAILED(status))
		goto _exit;

	*addr = ptr;

_exit:
	return status;
}

static efi_status_t bl_efi_get_boot_fs(struct efi_file_protocol **root_dir)
{
	efi_status_t status;
	efi_guid_t simple_fs_protocol_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	struct efi_loaded_image_protocol *loaded_image;
	struct efi_simple_file_system_protocol *fs_info;

	/* Get device handle. */
	status = bl_efi_get_loaded_image(&loaded_image);
 	if (EFI_FAILED(status))
		goto _exit;

	/* File system volume. */
	status = bl_efi_open_protocol(loaded_image->device_handle, &simple_fs_protocol_guid,
		(void **)&fs_info, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	if (EFI_FAILED(status))
		goto _exit;

	status = fs_info->open_volume(fs_info, root_dir);
	if (EFI_FAILED(status))
		goto _exit;

_exit:
	return status;
}

static efi_status_t bl_efi_get_file(efi_wchar_t *path, struct efi_file_protocol **modules_file)
{
	efi_status_t status;
	struct efi_file_protocol *root_dir;

	/* File system reference. */
	status = bl_efi_get_boot_fs(&root_dir);
	if (EFI_FAILED(status))
		goto _exit;

	/* Open modules file (should be). */
	status = root_dir->open(root_dir, modules_file, path, EFI_FILE_MODE_READ,
		EFI_FILE_READ_ONLY);
	if (EFI_FAILED(status))
		goto _exit;

_exit:
	return status;
}

static void bl_set_efi_info(efi_handle_t *image_handle, struct efi_system_table *system_table)
{
	bl_image_handle = image_handle;
	bl_system_table = system_table;
}

efi_status_t bl_main(efi_handle_t image_handle, struct efi_system_table *system_table)
{
	efi_status_t status;
	void *addr = NULL;
	struct efi_file_protocol *modules_file;

	bl_set_efi_info(image_handle, system_table);

	bl_efi_set_console();

	bl_heap_init();

	status = bl_efi_get_file(EFI_MODULES_FILE_PATH, &modules_file);
	if (EFI_FAILED(status))
		goto _exit;

	status = bl_efi_read_file(modules_file, &addr);
	if (EFI_FAILED(status))
		goto _exit;

	bl_mod_list = (struct bl_module_list_header *)addr;

	if (bl_init()) {
		status = EFI_LOAD_ERROR;
		goto _exit;
	}

_exit:
	return status;
}

