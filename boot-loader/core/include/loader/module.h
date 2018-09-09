#ifndef BL_MODULE_H
#define BL_MODULE_H

#include "include/bl-types.h"
#include "include/elf/elf32.h"
#include "core/include/loader/loader.h"

/* Modules info. */
#define BL_MODULE_LIST_HEADER_MAGIC     0x54534c444f4d4c42
#define BL_MODULE_HEADER_MAGIC          0x000000444f4d4c42

struct bl_module_list_header {
        bl_uint64_t magic;
        bl_uint32_t count;
        bl_uint32_t total_size;
} __attribute__((packed));

struct bl_module_header {
        bl_uint64_t magic;
        bl_uint32_t size;
} __attribute__((packed));

/* Loaded module. */
struct bl_segment {
        bl_uint8_t *address;
        bl_size_t size;

        Elf32_Section index;
};

struct bl_module {
        char name[BL_MODULE_MAX_NAME_LEN];

        int count;
        struct bl_segment *segments;

        void (*init)(void);
        void (*uninit)(void);

        struct bl_module *next;
};

void bl_loader_set_module_list(void *);

struct bl_module *bl_loader_get_module(const char *);

bl_status_t bl_loader_init(void);

bl_status_t bl_elf_load(Elf32_Ehdr *, bl_size_t);

#endif

