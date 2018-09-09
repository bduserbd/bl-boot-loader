#include "gpt.h"
#include "include/string.h"
#include "include/mbr.h"
#include "core/include/loader/loader.h"
#include "core/include/storage/storage.h"
#include "core/include/video/print.h"
#include "core/include/memory/heap.h"

BL_MODULE_NAME("GUID Partition Table");

static bl_status_t bl_gpt_iterate(struct bl_storage_device *disk)
{
	int i;
	struct bl_mbr mbr;
	struct bl_gpt_header gpt;
	struct bl_gpt_partition_entry entry;
	bl_uint64_t entry_offset;
	bl_status_t status;

	status = bl_storage_device_read(disk, (bl_uint8_t *)&mbr, 0, sizeof(struct bl_mbr), 0);
	if (status)
		return status;

	/* MBR signature. */
	if (mbr.signature != BL_MBR_SIGNATURE)
		return BL_STATUS_UNSUPPORTED;

	/* Is it a GPT device ?*/
	for (i = 0; i < 4; i++)
		if (mbr.entries[i].partition_type == BL_MBR_PARTITION_TYPE_GPT_DISK)
			break;

	if (i == 4)
		return BL_STATUS_UNSUPPORTED;

	/* Read GPT header. */
	status = bl_storage_device_read(disk, (bl_uint8_t *)&gpt, 1, sizeof(struct bl_gpt_header), 0);
	if (status)
		return status;

	if (bl_memcmp(gpt.signature, BL_GPT_SIGNATURE, sizeof(gpt.signature)))
		return BL_STATUS_UNSUPPORTED;

	/* Read GPT partition entries. Each entry is 128 bytes. */
	for (i = 0, entry_offset = 0; i < gpt.partitions_count; i++, entry_offset +=
			gpt.partition_entry_size) {
		/* Record every non-empty partition. */
		status = bl_storage_device_read(disk, (bl_uint8_t *)&entry, gpt.partitions_lba +
				entry_offset / BL_STORAGE_SECTOR_SIZE, sizeof(struct bl_gpt_partition_entry),
				entry_offset % BL_STORAGE_SECTOR_SIZE);
		if (status)
			return status;

		bl_guid_t empty_guid = BL_GPT_UNUSED_ENTRY;
		if (!bl_memcmp((void *)&entry.partition_type_guid, (void *)&empty_guid, sizeof(bl_guid_t)))
			continue;

		struct bl_partition *partition = bl_heap_alloc(sizeof(struct bl_partition));
		if (!partition)
			return BL_STATUS_MEMORY_ALLOCATION_FAILED;

		partition->lba = entry.first_lba;
		partition->sectors = entry.last_lba - entry.first_lba;

		partition->next = disk->partitions;
		disk->partitions = partition;
	}

	return BL_STATUS_SUCCESS;
}

static struct bl_partition_table_functions gpt_functions = {
	.iterate = bl_gpt_iterate,
};

BL_MODULE_INIT()
{
	bl_partition_table_register(&gpt_functions);
}

BL_MODULE_UNINIT()
{

}

