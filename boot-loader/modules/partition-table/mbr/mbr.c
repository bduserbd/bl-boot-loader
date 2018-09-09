#include "include/mbr.h"
#include "core/include/loader/loader.h"
#include "core/include/storage/storage.h"
#include "core/include/memory/heap.h"

BL_MODULE_NAME("Master Boot Record");

static bl_status_t bl_mbr_iterate(struct bl_storage_device *disk)
{
	int i;
	struct bl_mbr mbr;
	bl_status_t status;
	
	/* MBR Signature. */
	status = bl_storage_device_read(disk, (bl_uint8_t *)&mbr, 0, sizeof(struct bl_mbr), 0);
	if (status)
		return status;

	if (mbr.signature != BL_MBR_SIGNATURE)
		return BL_STATUS_UNSUPPORTED;

	for (i = 0; i < 4; i++) {
		if (mbr.entries[i].partition_type == BL_MBR_PARTITION_TYPE_NONE)
			continue;

		struct bl_partition *partition = bl_heap_alloc(sizeof(struct bl_partition));
		if (!partition)
			return BL_STATUS_MEMORY_ALLOCATION_FAILED;

		partition->lba = mbr.entries[i].start;
		partition->sectors = mbr.entries[i].length;

		partition->next = disk->partitions;
		disk->partitions = partition;
	}

	return BL_STATUS_SUCCESS;
}

static struct bl_partition_table_functions mbr_functions = {
	.iterate = bl_mbr_iterate,
};

BL_MODULE_INIT()
{
	bl_partition_table_register(&mbr_functions);
}

BL_MODULE_UNINIT()
{

}

