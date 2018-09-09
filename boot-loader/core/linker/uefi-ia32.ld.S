OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
	__bl_image_start = .;

	/* PE headers. */
	.efi_pe	: { *(.efi_pe) }
	. = ALIGN(SECTION_ALIGNMENT);
	__bl_efi_pe_end = .;

	/* Text section. */
	__bl_text_start = .;
	.text	: { *(.text*) }
	__bl_text_raw_size = SIZEOF(.text);
	. = ALIGN(SECTION_ALIGNMENT);
	__bl_text_size = . - __bl_text_start;

	/* Dummy relocation section. */
	__bl_reloc_start = .;
	.reloc	: { *(.reloc) }
	__bl_reloc_raw_size = SIZEOF(.reloc);

	/* Data section. */
	__bl_data_start = .;
	.data	: { *(.data) }
	.rodata	: { *(.rodata*) }
	.bss	: { *(.bss) }

	__bl_hash_start = .;
	.hash		: { *(.hash) }

	__bl_dynsym_start = .;
	.dynsym		: { *(.dynsym) }
	__bl_dynsym_end = .;

	__bl_dynstr_start = .;
	.dynstr		: { *(.dynstr) }

	.dynamic	: { *(.dynamic) }

	.rel.text	: { *(.rel.text) }
	.rel.rodata	: { *(.rel.rodata*) }

	__bl_data_raw_size = . - __bl_data_start;
	. = ALIGN(SECTION_ALIGNMENT);
	__bl_data_size = . - __bl_data_start;

	/* Image end. */
	__bl_image_size = . - __bl_image_start;

	/* Unneeded sections. */
	/DISCARD/ : {
		*(.rel.efi_pe)
		*(.rel.reloc)
		*(.eh_frame)
		*(.comment)
	}
}

