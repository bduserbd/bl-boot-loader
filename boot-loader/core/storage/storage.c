#include "include/export.h"
#include "include/string.h"
#include "core/include/storage/storage.h"
#include "core/include/video/print.h"
#include "core/include/memory/heap.h"

static struct bl_storage_device *bl_storage_devices = NULL;

struct bl_storage_device *bl_storage_device_get(int index)
{
	int i;
	struct bl_storage_device *disk;

	if (index < 0)
		return NULL;

	for (i = 0, disk = bl_storage_devices; disk; i++, disk = disk->next)
		if (i == index)
			return disk;

	return NULL;
}

struct bl_partition *bl_storage_partition_get(struct bl_storage_device *disk, int index)
{
	int i;
	struct bl_partition *partition;

	if (index < 0)
		return NULL;

	for (i = 0, partition = disk->partitions; partition; i++, partition = partition->next)
		if (i == index)
			return partition;

	return NULL;
}

bl_status_t bl_storage_probe(void)
{
	bl_status_t status;
	struct bl_storage_device *disk;

	disk = bl_storage_devices;
	while (disk) {
		if (!disk->controller) {
			struct bl_disk_controller *controller =
				bl_disk_controller_match_type(BL_DISK_CONTROLLER_TYPE_USB_SCSI);
			if (!controller)
				continue;

			disk->controller = controller;
		}

		if (disk->controller->funcs->get_info) {
			status = disk->controller->funcs->get_info(disk);
			if (status)
				return status;
		}
			
		bl_partition_table_probe(disk);

		disk = disk->next;
	}

	return BL_STATUS_SUCCESS;
}

void bl_storage_dump_devices(void)
{
	struct bl_storage_device *disk;

	disk = bl_storage_devices;
	while (disk) {
		bl_print_str("Disk: ");

		bl_print_str("Sectors: ");
		bl_print_hex64(disk->sector_count);
		bl_print_str(" ");

		bl_print_str("Sector size: ");
		bl_print_hex(disk->sector_size);
		bl_print_str(" ");

		bl_print_str("Total size (bytes): ");
		bl_print_hex64(disk->sector_size * disk->sector_count);
		bl_print_str("\n");

		disk = disk->next;
	}
}

void bl_storage_device_register(struct bl_storage_device *disk)
{
	disk->next = bl_storage_devices;
	bl_storage_devices = disk;
}
BL_EXPORT_FUNC(bl_storage_device_register);

void bl_storage_device_unregister(struct bl_storage_device *disk)
{

}
BL_EXPORT_FUNC(bl_storage_device_unregister);

bl_status_t bl_storage_device_read(struct bl_storage_device *disk, bl_uint8_t *buf,
	bl_uint64_t lba, bl_size_t size, bl_offset_t offset)
{
	bl_status_t status;
	bl_uint8_t *raw_disk_data;
	bl_size_t sectors_size;

	sectors_size = BL_MEMORY_ALIGN_UP(offset + size, BL_STORAGE_SECTOR_SIZE);

	//if (offset && ((offset + size) & (BL_STORAGE_SECTOR_SIZE - 1))) {
		raw_disk_data = bl_heap_alloc(sectors_size);
		if (!raw_disk_data) {
			status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
			goto _exit;
		}

		status = disk->controller->funcs->read(disk, raw_disk_data, lba,
			sectors_size / BL_STORAGE_SECTOR_SIZE);
		if (status)
			goto _exit;

		bl_memcpy(buf, raw_disk_data + offset, size);
	/*} else {
		raw_disk_data = NULL;

		status = disk->controller->funcs->read(disk, buf, lba, sectors_size /
			BL_STORAGE_SECTOR_SIZE);
		if (status)
			goto _exit;
	}*/

_exit:
	if (raw_disk_data)
		bl_heap_free(raw_disk_data, sectors_size);

	return status;
}
BL_EXPORT_FUNC(bl_storage_device_read);

