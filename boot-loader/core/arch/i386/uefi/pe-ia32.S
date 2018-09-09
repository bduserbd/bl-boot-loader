#include "include/pe/pe.h"

# Start of EFI IA32 PE header.
	.section ".efi_pe", "a"

	# IMAGE_DOS_HEADER
	.word	MZ_DOS_MAGIC		# e_magic
	.skip	0x3a
	.long	pe_image_headers	# e_lfanew

	# IMAGE_NT_HEADERS
pe_image_headers:
	.long	PE_MAGIC	# Signature

	# IMAGE_FILE_HEADER
	.word	PE_IMAGE_FILE_MACHINE_I386		# Machine
	.word	number_of_sections			# NumberOfSections
	.long	0					# TimeDateStamp
	.long	0					# PointerToSymbolTable
	.long	0					# NumberOfSymbols
	.word	pe_image_sections - pe_optional_header	# SizeOfOptionalHeader
	.word	PE_IMAGE_FILE_EXECUTABLE_IMAGE |	\
		PE_IMAGE_FILE_LOCAL_SYMS_STRIPPED |	\
		PE_IMAGE_FILE_LINE_NUMS_STRIPPED |	\
		PE_IMAGE_FILE_32BIT_MACHINE |		\
		PE_IMAGE_FILE_DEBUG_STRIPPED		# Characteristics

pe_optional_header:

	# IMAGE_OPTIONAL_HEADER
	.word	PE_IMAGE_OPTIONAL_HEADER32_MAGIC	# Magic
	.byte	0					# MajorLinkerVersion
	.byte	0					# MinorLinkerVersion
	.long	__bl_text_size				# SizeOfCode
	.long	__bl_data_size				# SizeOfInitializedData
	.long	0					# SizeOfUninitializedData
	.long	__bl_text_start				# AddressOfEntryPoint
	.long	__bl_text_start				# BaseOfCode
	.long	__bl_data_start				# BaseOfData

	.long	0					# ImageBase
	.long	SECTION_ALIGNMENT			# SectionAlignment
	.long	SECTION_ALIGNMENT			# FileAlignment
	.word	0					# MajorOperatingSystemVersion
	.word	0					# MinorOperatingSystemVersion
	.word	0					# MajorImageVersion
	.word	0					# MinorImageVersion
	.word	0					# MajorSubsystemVersion
	.word	0					# MinorSubsystemVersion
	.long	0					# Win32VersionValue
	.long	__bl_image_size				# SizeOfImage
	.long	__bl_efi_pe_end				# SizeOfHeaders
	.long	0					# CheckSum
	.word	PE_IMAGE_SUBSYSTEM_EFI_APPLICATION	# Subsystem
	.word	0					# DllCharacteristics
	.long	0					# SizeOfStackReserve
	.long	0					# SizeOfStackCommit
	.long	0					# SizeOfHeapReserve
	.long	0					# SizeOfHeapCommit
	.long	0					# LoaderFlags
	.long	(pe_image_sections - .) / 8		# NumberOfRvaAndSizes

	# DataDirectory
	.quad	0					# ExportTable
	.quad	0					# ImportTable
	.quad	0					# ResourceTable
	.quad	0					# ExceptionTable
	.quad	0					# CertificationTable
	.quad	0					# BaseRelocationTable

pe_image_sections:

	# IMAGE_SECTION_HEADER
	.ascii	".text\0\0\0"				# Name
	.long	__bl_text_raw_size			# VirtualSize
	.long	__bl_text_start				# VirtualAddress
	.long	__bl_text_raw_size			# SizeOfRawData
	.long	__bl_text_start				# PointerToRawData
	.long	0					# PointerToRelocations
	.long	0					# PointerToLineNumbers
	.word	0					# NumberOfRelocations
	.word	0					# NumberOfLineNumbers
	.long	PE_IMAGE_SCN_CNT_CODE |			\
		PE_IMAGE_SCN_MEM_EXECUTE |		\
		PE_IMAGE_SCN_MEM_READ 			# Characteristics

	# As the loaded EFI application is relocatable, a .reloc
	# section must be present. We put a useless section with one
	# relocation entry. It is smaller than section alignment size.

	# IMAGE_SECTION_HEADER
	.ascii	".reloc\0\0"				# Name
	.long	SECTION_ALIGNMENT			# VirtualSize
	.long	__bl_reloc_start			# VirtualAddress
	.long	SECTION_ALIGNMENT			# SizeOfRawData
	.long	__bl_reloc_start			# PointerToRawData
	.long	0					# PointerToRelocations
	.long	0					# PointerToLineNumbers
	.word	0					# NumberOfRelocations
	.word	0					# NumberOfLineNumbers
	.long	PE_IMAGE_SCN_CNT_INITIALIZED_DATA |	\
		PE_IMAGE_SCN_ALIGN_1BYTES |		\
		PE_IMAGE_SCN_MEM_DISCARDABLE |		\
		PE_IMAGE_SCN_MEM_READ			# Characteristics

	# IMAGE_SECTION_HEADER
	.ascii	".data\0\0\0"				# Name
	.long	__bl_data_raw_size			# VirtualSize
	.long	__bl_data_start				# VirtualAddress
	.long	__bl_data_raw_size			# SizeOfRawData
	.long	__bl_data_start				# PointerToRawData
	.long	0					# PointerToRelocations
	.long	0					# PointerToLineNumbers
	.word	0					# NumberOfRelocations
	.word	0					# NumberOfLineNumbers
	.long	PE_IMAGE_SCN_CNT_INITIALIZED_DATA |	\
		PE_IMAGE_SCN_MEM_NOT_PAGED |		\
		PE_IMAGE_SCN_MEM_READ |			\
		PE_IMAGE_SCN_MEM_WRITE 			# Characteristics

	.set	number_of_sections, (. - pe_image_sections) / 40

