#include "include/string.h"
#include "include/error.h"
#include "core/include/loader/loader.h"
#include "core/include/loader/module.h"
#include "core/include/loader/symbols.h"

/* Binary module list. */
struct bl_module_list_header *bl_mod_list = NULL;

/* After we loaded modules. */
struct bl_module *bl_mods = NULL;

void bl_loader_set_module_list(void *ptr)
{
	if (ptr && !bl_mod_list)
		bl_mod_list = (struct bl_module_list_header *)ptr;
}

struct bl_module *bl_loader_get_module(const char *name)
{
	struct bl_module *mod;

	if (!bl_mods)
		return NULL;

	mod = bl_mods;
	while (mod) {
		if (!bl_strncmp(mod->name, name, BL_MODULE_MAX_NAME_LEN))
			return mod;

		mod = mod->next;
	}

	return NULL;
}

static bl_status_t bl_loader_load_module(void *ptr, bl_size_t size)
{
	return bl_elf_load((Elf32_Ehdr *)ptr, size);
}

bl_status_t bl_loader_init(void)
{
	int i;
	bl_status_t status;
	struct bl_module_header *mod;

	if (!bl_mod_list || bl_mod_list->magic != BL_MODULE_LIST_HEADER_MAGIC) {
		status = BL_STATUS_INVALID_MODULE_LIST;
		goto _exit;
	}

	mod = (struct bl_module_header *)(bl_mod_list + 1);
	for (i = 0; i < bl_mod_list->count; i++) {
		if (mod->magic != BL_MODULE_HEADER_MAGIC) {
			status = BL_STATUS_INVALID_MODULE;
			goto _exit;
		}

		status = bl_loader_load_module((void *)(mod + 1), mod->size);
		if (status)
			goto _exit;

		mod = (struct bl_module_header *)((char *)(mod + 1) + mod->size);
	}

	status = BL_STATUS_SUCCESS;

_exit:
	return status;
}

