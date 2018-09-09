#include "include/export.h"
#include "core/include/storage/storage.h"

static struct bl_partition_table_functions *bl_partition_table_list = NULL;

bl_status_t bl_partition_table_probe(struct bl_storage_device *disk)
{
	struct bl_partition_table_functions *table;

	table = bl_partition_table_list;
	while (table) {
		if (!table->iterate(disk))
			return BL_STATUS_SUCCESS;

		table = table->next;
	}

	return BL_STATUS_UNPROPER_DISK;
}

#if 0
void bl_partition_table_free_map(struct bl_partition *table)
{
	struct bl_partition *curr, *prev;

	if (!table)
		return;

	curr = table;
	while (curr) {
		prev = curr;
		curr = curr->next;
		bl_heap_free(prev);
	}
}
BL_EXPORT_FUNC(bl_partition_table_free_map);
#endif

void bl_partition_table_register(struct bl_partition_table_functions *table)
{
	table->next = bl_partition_table_list;
	bl_partition_table_list = table;
}
BL_EXPORT_FUNC(bl_partition_table_register);

void bl_partition_table_unregister(struct bl_partition_table_functions *table)
{

}
BL_EXPORT_FUNC(bl_partition_table_unregister);

