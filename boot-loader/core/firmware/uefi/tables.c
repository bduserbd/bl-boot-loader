#include "include/uefi/uefi.h"

/* EFI image handle & system table passed on boot. */
efi_handle_t *bl_image_handle = NULL;
struct efi_system_table *bl_system_table = NULL;

