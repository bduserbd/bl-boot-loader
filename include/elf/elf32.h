#ifndef BL_ELF32_H
#define BL_ELF32_H

/* 32-bit ELF types */
typedef unsigned short		Elf32_Half;
typedef unsigned int		Elf32_Word;
typedef signed int		Elf32_Sword;
typedef unsigned long long	Elf32_Xword;
typedef signed long long	Elf32_Sxword;
typedef unsigned int		Elf32_Addr;
typedef unsigned int		Elf32_Off;
typedef unsigned short		Elf32_Section;
typedef unsigned short		Elf32_Versym;

/* 32-bit ELF file header */
#define EI_NIDENT	16

typedef struct {
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

/* Fields in the e_ident array (EI_* are indices into the array) */
#define EI_MAG0		0
#define ELFMAG0		0x7f

#define EI_MAG1		1
#define ELFMAG1		'E'

#define EI_MAG2		2
#define ELFMAG2		'L'

#define EI_MAG3		3
#define ELFMAG3		'F'

#define EI_CLASS	4
#define ELFCLASSNON	0
#define ELFCLASS32	1
#define ELFCLASS64	2
#define ELFCLASSNUM	3

#define EI_DATA		5
#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2
#define ELFDATANUM	3

#define EI_VERSION	6	/* Must be EV_CURRENT */

#define EI_OSABI	7
#define ELFOSABI_NONE	0
#define ELFOSABI_SYSV	0

#define EI_ABIVERSION	8

#define EI_PAD		9

/* Values for e_type */
#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_NUM		5
#define ET_LOOS		0xfe00
#define ET_HIOS		0xfeff
#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

/* Needed values for e_machine */
#define EM_NONE		0
#define EM_386		3

/* Values for e_version */
#define EV_NONE		0
#define EV_CURRENT	1
#define EV_NUM		2

/* 32-bit section header */
typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

/* Special section indexes */
#define SHN_UNDEF	0
#define SNH_ABS		0xfff1

/* Section types (sh_type) */
#define SHT_NULL		0
#define SHT_PROGBITS		1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_INIT_ARRAY		14
#define SHT_FINI_ARRAY		15
#define SHT_PREINIT_ARRAY	16
#define SHT_GROUP		17
#define SHT_SYMTAB_SHNDX	18
#define	SHT_NUM			19

/* Bit mask of sh_flags */
#define SHF_WRITE		(1 << 0)
#define SHF_ALLOC		(1 << 1)
#define SHF_EXECINSTR		(1 << 2)
#define SHF_MERGE		(1 << 4)
#define SHF_STRINGS		(1 << 5)
#define SHF_INFO_LINK		(1 << 6)
#define SHF_LINK_ORDER		(1 << 7)
#define SHF_OS_NONCONFORMING	(1 << 8)
#define SHF_GROUP		(1 << 9)
#define SHF_TLS			(1 << 10)

/* 32-bit symbol table entry */
typedef struct {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	unsigned char	st_info;
	unsigned char	st_other;
	Elf32_Section	st_shndx;
} Elf32_Sym;

/* st_info fields */
#define ELF32_ST_BIND(val)		(((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)		((val) & 0xf)
#define ELF32_ST_INFO(bind, type)	(((bind) << 4) + ((type) & 0xf))

/* Symbol bind types */
#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_NUM		3
#define STB_LOOS	10
#define STB_HIOS	12
#define STB_LOPROC	13
#define STB_HIPROC	15

/* Symbol types */
#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_COMMON	5
#define STT_TLS		6
#define STT_NUM		7
#define STT_LOOS	10
#define STT_HIOS	12
#define STT_LOPROC	13
#define STT_HIPROC	15

/* Symbol visibility in st_other field */
#define ELF32_ST_VISIBILITY(o)	((o) & 0x03)

/* Symbol visibility types */
#define STV_DEFAULT	0
#define STV_INTERNAL	1
#define STV_HIDDEN	2
#define STV_PROTECTED	3

/* 32-bit relocation table entry (without addend) */
typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
} Elf32_Rel;

/* 32-bit relocation table entry (with addend) */
typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
	Elf32_Sword	r_addend;
} Elf32_Rela;
	
/* r_info field info. */
#define ELF32_R_SYM(val)		((val) >> 8)
#define ELF32_R_TYPE(val)		((val) & 0xff)
#define ELF32_R_INFO(sym, type)		(((sym) << 8) + ((type) & 0xff))

/* i386 relocation types */
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2
#define R_386_RELATIVE	8
#define R_386_16	20
#define R_386_PC16	21

/* 32-bit dynamic section entry. */
typedef struct {
	Elf32_Sword	d_tag;
	union {
		Elf32_Word	d_val;
		Elf32_Addr	d_ptr;
	} d_un;
} Elf32_Dyn;

/* d_tag values. */
#define DT_NULL			0
#define DT_NEEDED		1
#define DT_PLTRELSZ		2
#define DT_PLTGOT		3
#define DT_HASH			4
#define DT_STRTAB		5
#define DT_SYMTAB		6
#define DT_RELA			7
#define DT_RELASZ		8
#define DT_RELAENT		9
#define DT_STRSZ		10
#define DT_SYMENT		11
#define DT_INIT			12
#define DT_FINI			13
#define DT_SONAME		14
#define DT_RPATH		15
#define DT_SYMBOLIC		16
#define DT_REL			17
#define DT_RELSZ		18
#define DT_RELENT		19
#define DT_PLTREL		20
#define DT_DEBUG		21
#define DT_TEXTREL		22
#define DT_JMPREL		23
#define	DT_BIND_NOW		24
#define	DT_INIT_ARRAY		25
#define	DT_FINI_ARRAY		26
#define	DT_INIT_ARRAYSZ		27
#define	DT_FINI_ARRAYSZ		28
#define DT_RUNPATH		29
#define DT_FLAGS		30
#define DT_ENCODING		32
#define DT_PREINIT_ARRAY	32
#define DT_PREINIT_ARRAYSZ	33
#define	DT_NUM			34
#define DT_LOOS			0x6000000d
#define DT_HIOS			0x6ffff000
#define DT_LOPROC		0x70000000
#define DT_HIPROC		0x7fffffff

#endif

