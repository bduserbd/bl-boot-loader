/*
 * Meaningless .reloc section to help EFI boot us.
 */
	.section .reloc, "a"

	# IMAGE_BASE_RELOCATION
pe_image_reloc:
	.long	_padding	# VirtualAddress
	.long	10		# SizeOfBlock

	# TypeOffset
	.word	0x0

_padding:
	.skip	SECTION_ALIGNMENT - (_padding - pe_image_reloc)

