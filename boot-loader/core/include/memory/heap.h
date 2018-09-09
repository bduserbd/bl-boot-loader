#ifndef BL_HEAP_H
#define BL_HEAP_H

#include "include/export.h"
#include "include/bl-types.h"

#define BL_MEMORY_ALIGN_UP(addr, align) \
	((addr + (typeof(addr))align - 1) & ~((typeof(addr))align - 1))

void bl_heap_init(void);
void *bl_heap_alloc(bl_size_t);
void *bl_heap_alloc_align(bl_size_t, bl_size_t);
void bl_heap_free(void *, bl_size_t);

#endif

