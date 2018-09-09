#include "include/string.h"
#include "include/elf/elf32.h"
#include "core/include/loader/module.h"
#include "core/include/loader/symbols.h"
#include "core/include/memory/heap.h"
#include "core/include/video/print.h"

extern struct bl_module *bl_mods;
extern bl_uint8_t __bl_image_start[];

static bl_status_t bl_elf_check(Elf32_Ehdr *elf, bl_size_t size)
{
	/* Verify stats. */
	if (elf->e_ident[EI_MAG0] != ELFMAG0 ||
		elf->e_ident[EI_MAG1] != ELFMAG1 ||
		elf->e_ident[EI_MAG2] != ELFMAG2 ||
		elf->e_ident[EI_MAG3] != ELFMAG3 ||
		elf->e_ident[EI_CLASS] != ELFCLASS32 ||
		elf->e_ident[EI_DATA] != ELFDATA2LSB ||
		elf->e_ident[EI_VERSION] != EV_CURRENT ||
		elf->e_type != ET_REL ||
		elf->e_machine != EM_386 ||
		elf->e_version != EV_CURRENT)
		return BL_STATUS_INVALID_ELF;

	/* Verify size */
	if (size != elf->e_shoff + elf->e_shnum * elf->e_shentsize)
		return BL_STATUS_INVALID_MODULE_SIZE;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_elf_load_image(Elf32_Ehdr *elf, struct bl_module *mod)
{
	int i, j, count;
	bl_status_t status;
	const Elf32_Shdr *section;
	bl_size_t image_size, alignment;
	bl_uint8_t *image_ptr;

	image_size = 0;
	alignment = 1;
	section = (const Elf32_Shdr *)((bl_uint8_t *)elf + elf->e_shoff);
	for (i = 0, count = 0; i < elf->e_shnum; i++) {
		/* Get driver size & alignment info */
		image_size = BL_MEMORY_ALIGN_UP(image_size, alignment) + section[i].sh_size;
		if (section[i].sh_addralign > alignment)
			alignment = section[i].sh_addralign;

		/* In order to have coherent view of mapped sections save their info */
		if (section[i].sh_flags & SHF_ALLOC && section[i].sh_size)
			count++;
	}

        /* Now allocate space to store info for the allocated sections */
        mod->count = count;
        mod->segments = bl_heap_alloc(mod->count * sizeof(struct bl_segment));
        if (!mod->segments) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	image_ptr = bl_heap_alloc_align(image_size, alignment);
	if (!image_ptr) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	for (i = 0, j = 0; i < elf->e_shnum; i++) {
		if (section[i].sh_flags & SHF_ALLOC && section[i].sh_size) {
			image_ptr = (bl_uint8_t *)BL_MEMORY_ALIGN_UP((bl_addr_t)image_ptr,
				section[i].sh_addralign);

                        if (section[i].sh_type == SHT_PROGBITS)
                                bl_memcpy(image_ptr, (bl_uint8_t *)elf + section[i].sh_offset,
					section[i].sh_size);
			else if (section[i].sh_type == SHT_NOBITS)
				bl_memset(image_ptr, 0, section[i].sh_size);

			mod->segments[j].index = i;
			mod->segments[j].address = image_ptr;
			mod->segments[j].size = section[i].sh_size;
			j++;

			image_ptr += section[i].sh_size;
                }
        }

	return BL_STATUS_SUCCESS;

_exit:
	if (mod->segments)
		bl_heap_free(mod->segments, mod->count * sizeof(struct bl_segment));

	return status;
}

static bl_addr_t bl_elf_section_by_name(Elf32_Ehdr *elf, const char *name)
{
	int i;
	const Elf32_Shdr *section, *strtab;
	char *strings;

	section = (const Elf32_Shdr *)((bl_uint8_t *)elf + elf->e_shoff);
	strtab = (const Elf32_Shdr *)((bl_uint8_t *)elf + elf->e_shoff + elf->e_shstrndx *
		elf->e_shentsize);
	strings = (char *)elf + strtab->sh_offset;

	for (i = 0; i < elf->e_shnum; i++)
		if (!bl_strncmp(strings + section[i].sh_name, name, BL_MODULE_MAX_NAME_LEN))
			return (bl_addr_t)elf + section[i].sh_offset;

	return 0;
}


static bl_addr_t bl_elf_section_by_index(struct bl_module *mod, Elf32_Section index)
{
        int i;

        for (i = 0; i < mod->count; i++)
                if (mod->segments[i].index == index)
                        return (bl_addr_t)mod->segments[i].address;

        return 0;
}

static bl_status_t bl_elf_resolve_symbol(Elf32_Sym *symbol, char *name,
	struct bl_module *mod)
{
	switch (ELF32_ST_TYPE(symbol->st_info)) {
	case STT_FUNC:
		symbol->st_value += bl_elf_section_by_index(mod, symbol->st_shndx);

		if (!bl_strcmp(name, "bl_module_initialize"))
			mod->init = (void (*)(void))symbol->st_value;

		if (!bl_strcmp(name, "bl_module_uninitialize"))
			mod->uninit = (void (*)(void))symbol->st_value;

		break;

	case STT_NOTYPE:
	case STT_OBJECT:
		if (symbol->st_name && symbol->st_shndx == SHN_UNDEF) {
			bl_status_t status;
			struct bl_export_symbol *exsymbol;

			status = bl_symbols_find(name, &exsymbol);
			if (status) {
				bl_print_str("`");
				bl_print_str(name);
				bl_print_str("\' is either unexported or doesn't exist\n");

				return status;
			}

#ifdef BL_EXECUTABLE
			symbol->st_value = exsymbol->address;
#elif BL_SHARED_LIBRARY
			symbol->st_value = exsymbol->address + (Elf32_Addr)__bl_image_start;
#endif

			if (exsymbol->type == BL_EXPORT_TYPE_FUNC)
				symbol->st_info = ELF32_ST_INFO(ELF32_ST_TYPE(symbol->st_info),
					STT_FUNC);
		} else
			symbol->st_value += bl_elf_section_by_index(mod, symbol->st_shndx);

		break;

	case STT_SECTION:
		symbol->st_value = bl_elf_section_by_index(mod, symbol->st_shndx);
		break;

	case STT_FILE:
		symbol->st_value = 0;
		break;

	default:
		return BL_STATUS_INVALID_ELF;
	}

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_elf_load_symbols(Elf32_Ehdr *elf, struct bl_module *mod)
{
	int i, j;
	bl_status_t status;
	const Elf32_Shdr *section, *strtab;
	char *strings;
	Elf32_Sym *symbol;

	section = (const Elf32_Shdr *)((bl_addr_t)elf + elf->e_shoff);

	for (i = 0; i < elf->e_shnum; i++) {
		if (section[i].sh_type != SHT_SYMTAB)
			continue;

		symbol = (Elf32_Sym *)((bl_uint8_t *)elf + section[i].sh_offset);
		strtab = (const Elf32_Shdr *)((bl_uint8_t *)elf + elf->e_shoff + section[i].sh_link *
			elf->e_shentsize);
		strings = (char *)elf + strtab->sh_offset;

		for (j = 0; j < section[i].sh_size / section[i].sh_entsize; j++) {
			status = bl_elf_resolve_symbol(&symbol[j], strings + symbol[j].st_name, mod);
			if (status)
				goto _exit;
		}
	}

	status = BL_STATUS_SUCCESS;

_exit:
	return status;
}

static bl_status_t bl_elf_relocate_section(struct bl_module *mod, Elf32_Ehdr *elf,
        const Elf32_Shdr *rel_section, const Elf32_Shdr *sym_section)
{
	int i;
	const Elf32_Rel *rel;
	const Elf32_Sym *symbol;
	Elf32_Word *rel32_addr;
	Elf32_Half *rel16_addr;
	bl_uint8_t *applied_section;

	applied_section = (bl_uint8_t *)bl_elf_section_by_index(mod, rel_section->sh_info);
	rel = (const Elf32_Rel *)((bl_uint8_t *)elf + rel_section->sh_offset);
	symbol = (const Elf32_Sym *)((bl_uint8_t *)elf + sym_section->sh_offset);

	for (i = 0; i < rel_section->sh_size / rel_section->sh_entsize; i++) {
		rel16_addr = (Elf32_Half *)(applied_section + rel[i].r_offset);
		rel32_addr = (Elf32_Word *)(applied_section + rel[i].r_offset);

		switch (ELF32_R_TYPE(rel[i].r_info)) {
		case R_386_16:
			*rel16_addr += symbol[ELF32_R_SYM(rel[i].r_info)].st_value;
			break;

		case R_386_PC16:
			*rel16_addr += symbol[ELF32_R_SYM(rel[i].r_info)].st_value -
				(bl_addr_t)rel16_addr;
			break;

		case R_386_32:
			*rel32_addr += symbol[ELF32_R_SYM(rel[i].r_info)].st_value;
			break;

		case R_386_PC32:
			*rel32_addr += symbol[ELF32_R_SYM(rel[i].r_info)].st_value -
				(bl_addr_t)rel32_addr;
			break;

		default:
			break;
		}
	}

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_elf_relocate_symbols(Elf32_Ehdr *elf, struct bl_module *mod)
{
	int i;
	bl_status_t status;
	const Elf32_Shdr *section;

	section = (const Elf32_Shdr *)((bl_uint8_t *)elf + elf->e_shoff);

	for (i = 0; i < elf->e_shnum; i++)
		if (section[i].sh_type == SHT_REL)
			if ((status = bl_elf_relocate_section(mod, elf, &section[i],
				&section[section[i].sh_link])))
				goto _exit;

	status = BL_STATUS_SUCCESS;

_exit:
	return status;
}

bl_status_t bl_elf_load(Elf32_Ehdr *elf, bl_size_t size)
{
	bl_status_t status;
	char *name;
	struct bl_module *mod = NULL;

        status = bl_elf_check(elf, size);
	if (status)
		goto _exit;

	/* Check if loaded already. */
	name = (char *)bl_elf_section_by_name(elf, ".bl_mod_name");
	if (!name) {
		bl_print_str(".bl_mod_name ELF section doesn't exist\n");
		status = BL_STATUS_INVALID_ELF;
		goto _exit;
	}

	if (bl_loader_get_module(name)) {
		status = BL_STATUS_MODULE_ALREADY_LOADED;
		goto _exit;
	}

	mod = bl_heap_alloc(sizeof(struct bl_module));
	if (!mod) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}
	bl_strncpy(mod->name, name, BL_MODULE_MAX_NAME_LEN);
	mod->init = NULL;
	mod->uninit = NULL;
	mod->next = NULL;

	/* Load & initialize image. */
	if ((status = bl_elf_load_image(elf, mod)))
		goto _exit;

	if ((status = bl_elf_load_symbols(elf, mod)))
		goto _exit;

	if ((status = bl_elf_relocate_symbols(elf, mod)))
		goto _exit;

	if (!mod->init) {
		status = BL_STATUS_INVALID_ELF;
		goto _exit;
	} else
		mod->init();
	
	/* Add to module list. */
	mod->next = bl_mods;
	bl_mods = mod;

	return BL_STATUS_SUCCESS;

_exit:
	if (mod)
		bl_heap_free(mod, sizeof(struct bl_module));

	return status;
}

