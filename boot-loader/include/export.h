#ifndef BL_EXPORT_H
#define BL_EXPORT_H

#include "bl-types.h"

typedef enum {
	BL_EXPORT_TYPE_UNKNOWN = -1,
	BL_EXPORT_TYPE_FUNC,
	BL_EXPORT_TYPE_VAR,
} bl_export_t;

struct bl_export_symbol {
	bl_addr_t address;
	bl_export_t type;
	const char *name;
};

#ifdef BL_EXECUTABLE

extern struct bl_export_symbol __bl_symtab_start[];
extern struct bl_export_symbol __bl_symtab_end[];

#define __BL_EXPORT_SYMBOL(sym, type)					\
	extern typeof(sym) sym;						\
	static const char __bl_strtab_##sym[]				\
	__attribute__((section(".bl_strtab"), aligned(1))) = #sym;	\
	static const struct bl_export_symbol __bl_symtab_##sym		\
	__attribute__((section(".bl_symtab"), used))			\
	= { (bl_addr_t)&sym, type, __bl_strtab_##sym }

#define BL_EXPORT_FUNC(sym)	\
	__BL_EXPORT_SYMBOL(sym, BL_EXPORT_TYPE_FUNC)

#define BL_EXPORT_VAR(sym)	\
	__BL_EXPORT_SYMBOL(sym, BL_EXPORT_TYPE_VAR)

#elif BL_SHARED_LIBRARY

extern bl_uint8_t __bl_hash_start[];
extern Elf32_Sym __bl_dynsym_start[];
extern Elf32_Sym __bl_dynsym_end[];
extern char __bl_dynstr_start[];

/* Symbol visibility. */
#define BL_VIS_DEFAULT		__attribute__((visibility("default")))
#define BL_VIS_HIDDEN		__attribute__((visibility("hidden")))

/* Exporting boot loader symbols. */
#define __BL_EXPORT_SYMBOL(sym)	\
	extern BL_VIS_DEFAULT typeof(sym) sym;

#define BL_EXPORT_FUNC(sym)	__BL_EXPORT_SYMBOL(sym)
#define BL_EXPORT_VAR(sym)	__BL_EXPORT_SYMBOL(sym)

#endif

#endif

