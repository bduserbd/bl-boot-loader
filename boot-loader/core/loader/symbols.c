#include "include/string.h"
#include "include/elf/elf32.h"
#include "core/include/loader/symbols.h"
#include "core/include/loader/module.h"

static struct bl_export_symbol _exsymbol;

#ifdef BL_EXECUTABLE

bl_status_t bl_symbols_find(const char *name, struct bl_export_symbol **exsymbol)
{
	int i;

	if (!name || !exsymbol)
		return BL_STATUS_INVALID_PARAMETERS;

	*exsymbol = NULL;

	for (i = 0; &__bl_symtab_start[i] != __bl_symtab_end; i++)
		if (bl_strncmp(__bl_symtab_start[i].name, name, BL_MODULE_MAX_NAME_LEN)
			== 0) {
			bl_memcpy(&_exsymbol, &__bl_symtab_start[i],
				sizeof(struct bl_export_symbol));

			*exsymbol = &_exsymbol;

			return BL_STATUS_SUCCESS;
		}

	return BL_STATUS_UNKNOWN_SYMBOL;
}

#elif BL_SHARED_LIBRARY

static bl_uint32_t bl_symbols_elf_hash(const bl_uint8_t *name)
{
	int i;
	bl_uint32_t h = 0, g;

	for (i = 0; name[i]; i++) {
		h = (h << 4) + name[i];
		if ((g = h & 0xf0000000))
			h ^= g >> 24;
		h &= ~g;
	}

	return h;
}

struct bl_elf_hash_table {
	Elf32_Word nbucket;
	Elf32_Word nchain;
	Elf32_Word info[0];
};

static Elf32_Sym *bl_symbols_elf_find_symbol(const char *name)
{
	bl_uint32_t i, bucket;
	struct bl_elf_hash_table *hash_table;

	hash_table = (struct bl_elf_hash_table *)__bl_hash_start;
	bucket = hash_table->info[bl_symbols_elf_hash((const bl_uint8_t *)name) %
		hash_table->nbucket];

	for (i = bucket; i; i = hash_table->info[i + hash_table->nbucket])
		if (!bl_strcmp(__bl_dynstr_start + __bl_dynsym_start[i].st_name, name))
			return &__bl_dynsym_start[i];

	return NULL;
}

bl_status_t bl_symbols_find(const char *name, struct bl_export_symbol **exsymbol)
{
	Elf32_Sym *elfsymbol;

	if (!name || !exsymbol)
		return BL_STATUS_INVALID_PARAMETERS;

	*exsymbol = NULL;

	elfsymbol = bl_symbols_elf_find_symbol(name);
	if (!elfsymbol)
		return BL_STATUS_UNKNOWN_SYMBOL;

	bl_memset(&_exsymbol, 0, sizeof(struct bl_export_symbol));

	_exsymbol.address = elfsymbol->st_value;
	switch (ELF32_ST_TYPE(elfsymbol->st_info)) {
	case STT_FUNC:
		_exsymbol.type = BL_EXPORT_TYPE_FUNC;
		break;

	case STT_OBJECT:
		_exsymbol.type = BL_EXPORT_TYPE_VAR;
		break;

	default:
		return BL_STATUS_UNKNOWN_SYMBOL;
	}
	_exsymbol.name = __bl_dynstr_start + elfsymbol->st_name;

	*exsymbol = &_exsymbol;

	return BL_STATUS_SUCCESS;
}

#endif

