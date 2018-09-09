#include "firmware/uefi/include/tables.h"
#include "include/export.h"

/* Common functions. */
efi_status_t bl_efi_locate_handle_buffer(efi_locate_search_type_t type,
	efi_guid_t *protocol, void *search_key, efi_uintn_t *no_handles,
	efi_handle_t **buffer)
{
	return bl_system_table->boot_services->locate_handle_buffer(type,
		protocol, search_key, no_handles, buffer);
}
BL_EXPORT_FUNC(bl_efi_locate_handle_buffer);

efi_status_t bl_efi_open_protocol(efi_handle_t handle, efi_guid_t *protocol,
	void **interface, efi_uint32_t attributes)
{
	return bl_system_table->boot_services->open_protocol(handle,
		protocol, interface, bl_image_handle, NULL, attributes);
}
BL_EXPORT_FUNC(bl_efi_open_protocol);

efi_status_t bl_efi_get_loaded_image(struct efi_loaded_image_protocol **loaded_image)
{
	efi_guid_t loaded_image_protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

	return bl_efi_open_protocol(bl_image_handle, &loaded_image_protocol_guid,
		(void **)loaded_image, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
}
BL_EXPORT_FUNC(bl_efi_get_loaded_image);

