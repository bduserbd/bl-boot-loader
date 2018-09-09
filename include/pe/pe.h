#ifndef PE_H
#define PE_H

#ifndef __ASSEMBLER__
#include "include/types.h"
#endif

/* MZ magic. */
#define MZ_DOS_MAGIC	0x5a4d

/* PE image headers magic. */
#define PE_MAGIC	0x00004550

/* PE supported machine types. */
#define PE_IMAGE_FILE_MACHINE_I386	0x14c

/* PE optional header magic. */
#define PE_IMAGE_OPTIONAL_HEADER32_MAGIC	0x10b

/* PE application subsystems. */
#define PE_IMAGE_SUBSYSTEM_EFI_APPLICATION	10

/* PE image file characteristics. */
#define PE_IMAGE_FILE_RELOCS_STRIPPED		0x0001
#define PE_IMAGE_FILE_EXECUTABLE_IMAGE		0x0002
#define PE_IMAGE_FILE_LINE_NUMS_STRIPPED   	0x0004
#define PE_IMAGE_FILE_LOCAL_SYMS_STRIPPED  	0x0008
#define PE_IMAGE_FILE_AGGRESIVE_WS_TRIM		0x0010
#define PE_IMAGE_FILE_LARGE_ADDRESS_AWARE	0x0020
#define PE_IMAGE_FILE_16BIT_MACHINE		0x0040
#define PE_IMAGE_FILE_BYTES_REVERSED_LO		0x0080
#define PE_IMAGE_FILE_32BIT_MACHINE		0x0100
#define PE_IMAGE_FILE_DEBUG_STRIPPED		0x0200
#define PE_IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP	0x0400
#define PE_IMAGE_FILE_NET_RUN_FROM_SWAP		0x0800
#define PE_IMAGE_FILE_SYSTEM			0x1000
#define PE_IMAGE_FILE_DLL			0x2000
#define PE_IMAGE_FILE_UP_SYSTEM_ONLY		0x4000
#define PE_IMAGE_FILE_BYTES_REVERSED_HI		0x8000

/* PE section characteristics. */
#define PE_IMAGE_SCN_CNT_CODE			0x00000020
#define PE_IMAGE_SCN_CNT_INITIALIZED_DATA	0x00000040
#define PE_IMAGE_SCN_CNT_UNINITIALIZED_DATA	0x00000080

#define	PE_IMAGE_SCN_LNK_OTHER			0x00000100
#define	PE_IMAGE_SCN_LNK_INFO			0x00000200
#define	PE_IMAGE_SCN_LNK_REMOVE			0x00000800
#define	PE_IMAGE_SCN_LNK_COMDAT			0x00001000

#define	PE_IMAGE_SCN_MEM_FARDATA		0x00008000

#define	PE_IMAGE_SCN_MEM_PURGEABLE		0x00020000
#define	PE_IMAGE_SCN_MEM_16BIT			0x00020000
#define	PE_IMAGE_SCN_MEM_LOCKED			0x00040000
#define	PE_IMAGE_SCN_MEM_PRELOAD		0x00080000

#define	PE_IMAGE_SCN_ALIGN_1BYTES		0x00100000
#define	PE_IMAGE_SCN_ALIGN_2BYTES		0x00200000
#define	PE_IMAGE_SCN_ALIGN_4BYTES		0x00300000
#define	PE_IMAGE_SCN_ALIGN_8BYTES		0x00400000
#define	PE_IMAGE_SCN_ALIGN_16BYTES		0x00500000
#define	PE_IMAGE_SCN_ALIGN_32BYTES		0x00600000
#define	PE_IMAGE_SCN_ALIGN_64BYTES		0x00700000
#define	PE_IMAGE_SCN_ALIGN_128BYTES		0x00800000
#define	PE_IMAGE_SCN_ALIGN_256BYTES		0x00900000
#define	PE_IMAGE_SCN_ALIGN_512BYTES		0x00A00000
#define	PE_IMAGE_SCN_ALIGN_1024BYTES		0x00B00000
#define	PE_IMAGE_SCN_ALIGN_2048BYTES		0x00C00000
#define	PE_IMAGE_SCN_ALIGN_4096BYTES		0x00D00000
#define	PE_IMAGE_SCN_ALIGN_8192BYTES		0x00E00000

#define PE_IMAGE_SCN_LNK_NRELOC_OVFL		0x01000000

#define PE_IMAGE_SCN_MEM_DISCARDABLE		0x02000000
#define PE_IMAGE_SCN_MEM_NOT_CACHED		0x04000000
#define PE_IMAGE_SCN_MEM_NOT_PAGED		0x08000000
#define PE_IMAGE_SCN_MEM_SHARED			0x10000000
#define PE_IMAGE_SCN_MEM_EXECUTE		0x20000000
#define PE_IMAGE_SCN_MEM_READ			0x40000000
#define PE_IMAGE_SCN_MEM_WRITE			0x80000000

/* PE i386 relocation types */
#define	PE_IMAGE_REL_I386_ABSOLUTE	0
#define	PE_IMAGE_REL_I386_DIR16		1
#define	PE_IMAGE_REL_I386_REL16		2
#define	PE_IMAGE_REL_I386_DIR32		6
#define	PE_IMAGE_REL_I386_DIR32NB	7
#define	PE_IMAGE_REL_I386_SEG12		9
#define	PE_IMAGE_REL_I386_SECTION	10
#define	PE_IMAGE_REL_I386_SECREL	11
#define	PE_IMAGE_REL_I386_TOKEN		12
#define	PE_IMAGE_REL_I386_SECREL7	13
#define	PE_IMAGE_REL_I386_REL32		20

#ifndef __ASSEMBLER__

/* MZ DOS header. */
struct mz_dos_header {
	uint16_t magic;
	uint16_t cblp;
	uint16_t cp;
	uint16_t crlc;
	uint16_t cparhdr;
	uint16_t minalloc;
	uint16_t maxalloc;
	uint16_t ss;
	uint16_t sp;
	uint16_t csum;
	uint16_t ip;
	uint16_t cs;
	uint16_t lfarlc;
	uint16_t ovno;
	uint16_t res[4];
	uint16_t oemid;
	uint16_t oeminfo;
	uint16_t res2[10];
	uint32_t lfanew;
};

/* PE image file header. */
struct pe_image_file_header {
	uint16_t machine;
	uint16_t number_of_sections;
	uint32_t time_date_stamp;
	uint32_t pointer_to_symbol_table;
	uint32_t number_of_symbols;
	uint16_t size_of_optional_header;
	uint16_t characteristics;
};

/* PE image data directory. */
#define PE_IMAGE_DATA_DIRECTORY_ENTRIES	16

struct pe_image_data_directory {
	uint32_t virtual_address;
	uint32_t size;
};

/* PE image optional header. */
struct pe_image_optional_header32 {
	/* Standard fields. */
	uint16_t magic;
	uint8_t major_linker_version;
	uint8_t minor_linker_version;
	uint32_t size_of_code;
	uint32_t size_of_initialized_data;
	uint32_t size_of_uninitialized_data;
	uint32_t address_of_entry_point;
	uint32_t base_of_code;
	uint32_t base_of_data;

	/* NT additional fields. */
	uint32_t image_base;
	uint32_t section_alignment;
	uint32_t file_alignment;
	uint16_t major_operating_system_version;
	uint16_t minor_operating_system_version;
	uint16_t major_image_version;
	uint16_t minor_image_version;
	uint16_t major_subsystem_version;
	uint16_t minor_subsystem_version;
	uint32_t win32_version_value;
	uint32_t size_of_image;
	uint32_t size_of_headers;
	uint32_t check_sum;
	uint16_t subsystem;
	uint16_t dll_characteristics;
	uint32_t size_of_stack_reserve;
	uint32_t size_of_stack_commit;
	uint32_t size_of_heap_reserve;
	uint32_t size_of_heap_commit;
	uint32_t loader_flags;
	uint32_t number_of_rva_and_sizes;
	struct pe_image_data_directory data_directory[PE_IMAGE_DATA_DIRECTORY_ENTRIES];
};

/* PE image headers. */
struct pe_image_headers32 {
	uint32_t signature;
	struct pe_image_file_header file_header;
	struct pe_image_optional_header32 optional_header;
};

/* PE image section header. */
struct pe_image_section_header {
	char name[8];
	uint32_t virtual_size;
	uint32_t virtual_address;
	uint32_t size_of_raw_data;
	uint32_t pointer_to_raw_data;
	uint32_t pointer_to_relocations;
	uint32_t pointer_to_line_numbers;
	uint16_t number_of_relocations;
	uint16_t number_of_line_numbers;
	uint32_t characteristics;
};

#endif

#endif

