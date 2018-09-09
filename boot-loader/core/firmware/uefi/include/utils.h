#ifndef BL_UEFI_UTILS_H
#define BL_UEFI_UTILS_H

#include "include/uefi/uefi.h"

efi_status_t bl_efi_locate_handle_buffer(efi_locate_search_type_t, efi_guid_t *,
	void *, efi_uintn_t *, efi_handle_t **);
efi_status_t bl_efi_open_protocol(efi_handle_t, efi_guid_t *, void **, efi_uint32_t);
efi_status_t bl_efi_get_loaded_image(struct efi_loaded_image_protocol **);

#endif

