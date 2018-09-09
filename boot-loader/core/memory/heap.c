#include "include/string.h"
#include "core/include/memory/heap.h"
#include "core/include/loader/module.h"

/* Very simple heap implementation. */

#define BL_HEAP_BLOCK_HEAD_USED_MAGIC	0x44455355 /* "USED" */
#define BL_HEAP_BLOCK_HEAD_FREE_MAGIC	0x45455246 /* "FREE" */

#define BL_HEAP_ALIGN_LOG2	4
#define BL_HEAP_MEM_ALIGN	(1 << BL_HEAP_ALIGN_LOG2)

#ifndef FIRMWARE_BIOS
extern bl_uint8_t __bl_modules_start[];
#endif

struct bl_heap_block_head {
        struct bl_heap_block_head *next;
        __u32 magic;
        __u32 size;
        __u8 pad[4];
} __attribute__((packed));

static bl_uint8_t *bl_heap = NULL;
static bl_int32_t occupied_blocks = 0;
static bl_int32_t free_blocks = 0;

void bl_heap_init(void)
{
	if (bl_heap)
		return;

#ifdef FIRMWARE_BIOS
	/* Heap implementation under 1MB is pretty limited .. */
	bl_heap = (bl_uint8_t *)0x100000;
#else
	int i;
	struct bl_module_list_header *mod_list;
	struct bl_module_header *mod;

	mod_list = (struct bl_module_list_header *)__bl_modules_start;
	if (mod_list->magic == BL_MODULE_LIST_HEADER_MAGIC) {
		for (mod = (struct bl_module_header *)(mod_list + 1), i = 0; i < mod_list->count;
			mod = (struct bl_module_header *)((bl_addr_t)(mod + 1) + mod->size), i++)
			if (mod->magic != BL_MODULE_HEADER_MAGIC)
				return;

		bl_heap = (void *)BL_MEMORY_ALIGN_UP((bl_addr_t)mod, BL_HEAP_MEM_ALIGN);
	} else
		bl_heap = (void *)BL_MEMORY_ALIGN_UP((bl_addr_t)__bl_modules_start, BL_HEAP_MEM_ALIGN);
#endif
}

static void *__bl_heap_alloc_align(bl_uint8_t ptr[], bl_size_t sz, bl_size_t align)
{
	void *alloc;
	struct bl_heap_block_head *p, *q;

	for (q = (struct bl_heap_block_head *)ptr, p = q->next; p; q = p, p = p->next)
		if (q->magic == BL_HEAP_BLOCK_HEAD_FREE_MAGIC) {
			if (!((bl_addr_t)(q + 1) & (align - 1)) && q->size >= sz) {
				q->magic = BL_HEAP_BLOCK_HEAD_USED_MAGIC;
				return (void *)(q + 1);
			}	
		} else if (q->magic != BL_HEAP_BLOCK_HEAD_USED_MAGIC)
			return NULL;

	alloc = (void *)BL_MEMORY_ALIGN_UP((bl_addr_t)(q + 1) + q->size +
			sizeof(struct bl_heap_block_head),  BL_HEAP_MEM_ALIGN);

	if ((bl_addr_t)alloc & (align - 1))
		alloc = (void *)BL_MEMORY_ALIGN_UP((bl_addr_t)alloc, align);

	p = (void *)((bl_addr_t)alloc - sizeof(struct bl_heap_block_head));
	p->magic = BL_HEAP_BLOCK_HEAD_USED_MAGIC;
	p->size = sz;
	p->next = NULL;
	q->next = p;

	return alloc;
}

void *bl_heap_alloc_align(bl_size_t sz, bl_size_t align)
{
	if (!bl_heap)
		return NULL;

	if (align == 0x0)
		align = 0x1;

	/* Must be power of 2 */
	if (align & (align - 1))
		return NULL;

	if (!occupied_blocks && !free_blocks) {
		struct bl_heap_block_head head;

		head.next = NULL;
		head.magic = BL_HEAP_BLOCK_HEAD_USED_MAGIC;
		head.size = sz;

		// TODO : Fix this.
		if ((bl_addr_t)bl_heap & (align - 1))
			bl_heap = (bl_uint8_t *)BL_MEMORY_ALIGN_UP((bl_addr_t)bl_heap, align);

		occupied_blocks++;
		bl_memcpy(bl_heap, &head, sizeof(struct bl_heap_block_head));
		return (void *)(bl_heap + sizeof(struct bl_heap_block_head));
	}

	return __bl_heap_alloc_align(bl_heap, sz, align);
}
BL_EXPORT_FUNC(bl_heap_alloc_align);

void *bl_heap_alloc(bl_size_t sz)
{
	return bl_heap_alloc_align(sz, 0x1);
}
BL_EXPORT_FUNC(bl_heap_alloc);

void bl_heap_free(void *block, bl_size_t sz)
{
	struct bl_heap_block_head *p;

	if ((bl_addr_t)block & (BL_HEAP_MEM_ALIGN - 1))
		return;

	p = (struct bl_heap_block_head *)block - 1;
	if (p->magic == BL_HEAP_BLOCK_HEAD_USED_MAGIC) {
		p->magic = BL_HEAP_BLOCK_HEAD_FREE_MAGIC;
		free_blocks++;
	}
}
BL_EXPORT_FUNC(bl_heap_free);

