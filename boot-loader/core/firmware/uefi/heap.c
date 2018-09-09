#include "core/include/memory/heap.h"
#include "firmware/uefi/include/tables.h"

/* Use this one for UEFI environment. */

void bl_heap_init(void)
{

}

void *bl_heap_alloc_align(bl_size_t sz, bl_size_t align)
{
	efi_status_t status;
	efi_physical_address_t addr;
	efi_uintn_t new_sz;

	if (align == 0x0)
		align = 0x1;

	/* Must be power of 2. */
	if (align & (align - 1))
		return NULL;

	if (align <= 0x8)
		return bl_heap_alloc(sz);

	/* Just allocate on 4-KB alignment .. */
	new_sz = BL_MEMORY_ALIGN_UP(sz, 0x1000);
	status = bl_system_table->boot_services->allocate_pages(EFI_ALLOCATE_ANY_PAGES,
		EFI_LOADER_DATA,  new_sz / 0x1000, &addr);
	if (EFI_FAILED(status))
		return NULL;

#if _BL_SIZEOF_PTR == 4
	void *ptr = (void *)(efi_uintn_t)(addr & 0xffffffff);
	bl_system_table->boot_services->set_mem(ptr, new_sz, 0);

	return ptr;
#endif
}
BL_EXPORT_FUNC(bl_heap_alloc_align);

/* 8-byte alignment. */
void *bl_heap_alloc(bl_size_t sz)
{
	efi_status_t status;
	void *ptr;

	status = bl_system_table->boot_services->allocate_pool(EFI_LOADER_DATA,
		sz, &ptr);
	if (EFI_FAILED(status))
		return NULL;

	bl_system_table->boot_services->set_mem(ptr, sz, 0);

	return ptr;
}
BL_EXPORT_FUNC(bl_heap_alloc);
 
void bl_heap_free(void *ptr, bl_size_t sz)
{
	efi_status_t status;

	if (!ptr)
		return;

	/* How bad is this ? */
	if ((bl_addr_t)ptr & 0xfff) {
		(void)bl_system_table->boot_services->free_pool(ptr);
	} else {
		status = bl_system_table->boot_services->free_pages((efi_uintn_t)ptr,
			BL_MEMORY_ALIGN_UP(sz, 0x1000) / 0x1000);

		/* Should have been allocated by AllocatePool(). */
		if (status == EFI_NOT_FOUND)
			(void)bl_system_table->boot_services->free_pool(ptr);
	}
}
BL_EXPORT_FUNC(bl_heap_free);

