#include "include/uefi/uefi.h"
#include "include/elf/elf32.h"
#include "include/pe/pe.h"

efi_status_t bl_efi_relocate_self(Elf32_Addr base_address, Elf32_Dyn *entry)
{
	int i;
	efi_status_t status;
	Elf32_Rel *reloc = NULL;
	Elf32_Word relocs_size = 0, entry_size = 0;

	if (*((efi_uint16_t *)base_address) != MZ_DOS_MAGIC) {
		status = EFI_INVALID_PARAMETER;
		goto _exit;
	}

	for (i = 0; entry[i].d_tag != DT_NULL; i++) {
		switch (entry[i].d_tag) {
		case DT_REL:
			reloc = (Elf32_Rel *)(base_address + entry[i].d_un.d_ptr);
			break;

		case DT_RELSZ:
			relocs_size = entry[i].d_un.d_val;
			break;

		case DT_RELENT:
			entry_size = entry[i].d_un.d_val;
			break;

		default:
			break;
		}
	}

	if (!reloc || !entry_size || entry_size != sizeof(Elf32_Rel)) {
		status = EFI_LOAD_ERROR;
		goto _exit;
	}

	for (i = 0; i < relocs_size / entry_size; i++) {
		switch (ELF32_R_TYPE(reloc[i].r_info)) {
		case R_386_RELATIVE:
			*(Elf32_Addr *)(base_address + reloc[i].r_offset) += base_address;
			break;

		default:
			break;
		}
	}

	status = EFI_SUCCESS;

_exit:
	return status;
}

