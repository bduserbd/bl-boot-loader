#include "pata.h"
#include "include/time.h"
#include "core/include/loader/loader.h"
#include "core/include/storage/storage.h"
#include "core/include/pci/pci.h"
#include "core/include/memory/heap.h"

BL_MODULE_NAME("Parallel ATA");

/* Device by channel. */
struct bl_pata_device {
	bl_port_t io;

	int type;
	int present;

	struct bl_pata_identification id;

	struct bl_pata_device *next;
};
static struct bl_pata_device *bl_pata_device_list = NULL;

static inline void bl_pata_device_add(struct bl_pata_device *device)
{
	device->next = bl_pata_device_list;
	bl_pata_device_list = device;
}

static inline void bl_pata_write_reg(struct bl_pata_device *device, int reg, int val)
{
	bl_outb(val, device->io + reg);
}

static inline bl_uint8_t bl_pata_read_reg(struct bl_pata_device *device, int reg)
{
	return bl_inb(device->io + reg);
}

static inline bl_uint16_t bl_pata_read_data(struct bl_pata_device *device)
{
	return bl_inw(device->io + BL_PATA_REG__READ__DPATA);
}

static bl_status_t bl_pata_wait_ready(struct bl_pata_device *device)
{
	int timeout = 100;

	while (--timeout) {
		if ((bl_pata_read_reg(device, BL_PATA_REG__READ__STATUS) &
					BL_PATA_REG_STATUS_BSY) == 0)
			return BL_STATUS_SUCCESS;

		bl_time_sleep(1);
	}

	return BL_STATUS_DISK_OPERATION_TIMEOUT;
}

static void bl_pata_buffer_read_sector(struct bl_pata_device *device, bl_uint16_t *buf)
{
	int i;

	for (i = 0; i < BL_STORAGE_SECTOR_SIZE / 2; i++)
		buf[i] = bl_pata_read_data(device);
}

static bl_status_t bl_pata_device_command(struct bl_pata_device *device, bl_uint8_t command,
		bl_uint8_t *buf, bl_uint64_t buf_sectors, bl_uint64_t lba, bl_uint64_t sectors)
{
	bl_uint64_t i;
	bl_status_t status;
	bl_uint8_t read_status;

	/* Either way, use the 48-bit address feature set */
	/* "Previous content" */
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__FEATURES, 0);

	bl_pata_write_reg(device, BL_PATA_REG__WRITE__SECTOR_COUNT, (sectors >> 8) & 0xff);

	bl_pata_write_reg(device, BL_PATA_REG__WRITE__LBA48_LOW, (lba >> 24) & 0xff);
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__LBA48_MID, (lba >> 32) & 0xff);
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__LBA48_HIGH, (lba >> 40) & 0xff);

	/* "Current content" */
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__SECTOR_COUNT, (sectors & 0xff));

	bl_pata_write_reg(device, BL_PATA_REG__WRITE__LBA_LOW, lba & 0xff);
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__LBA_MID, (lba >> 8) & 0xff);
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__LBA_HIGH, (lba >> 16) & 0xff);

	bl_pata_write_reg(device, BL_PATA_REG__WRITE__DEVICE,
			BL_PATA_REG_DEVICE_SET(1, device->type));

	/* Start command */
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__COMMAND, command);
	status = bl_pata_wait_ready(device);
	if (status)
		return status;

	/* Check status */
	read_status = bl_pata_read_reg(device, BL_PATA_REG__READ__STATUS);

	for (i = 0; i < buf_sectors; i++) {
		if ((read_status & (BL_PATA_REG_STATUS_DRQ | BL_PATA_REG_STATUS_ERR)) ==
				BL_PATA_REG_STATUS_ERR)
			break;

		bl_pata_buffer_read_sector(device, (bl_uint16_t *)(buf + i *
					BL_STORAGE_SECTOR_SIZE));

		status = bl_pata_wait_ready(device);
		if (status)
			return status;

		read_status = bl_pata_read_reg(device, BL_PATA_REG__READ__STATUS);
	}

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_pata_read(struct bl_storage_device *disk, bl_uint8_t *buf,
		bl_uint64_t lba, bl_uint64_t sectors)
{
	bl_status_t status;
	struct bl_pata_device *device;

	device = disk->data;

	status = bl_pata_device_command(device, BL_PATA_CMD_READ_SECTORS, buf, sectors,
			lba, sectors);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_pata_device_connected(struct bl_pata_device *device)
{
	bl_uint8_t status;

	/* Select device. */
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__DEVICE,
			BL_PATA_REG_DEVICE_SET(1, device->type));
	bl_time_sleep(1);

	/* Execute command. */
	bl_pata_write_reg(device, BL_PATA_REG__WRITE__COMMAND,
			BL_PATA_CMD_IDENTIFY_DEVICE);
	bl_time_sleep(1);

	/* Check for connection. */
	status = bl_pata_read_reg(device, BL_PATA_REG__READ__STATUS);
	if (status == 0)
		return BL_STATUS_FAILURE;

	int timeout = 50;
	while (--timeout) {
		status = bl_pata_read_reg(device, BL_PATA_REG__READ__STATUS);
		if ((status & BL_PATA_REG_STATUS_BSY) == 0 &&
				status & BL_PATA_REG_STATUS_DRQ)
			break;

		bl_time_sleep(1);
	}

	if (!timeout)
		return BL_STATUS_FAILURE;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_pata_identify_device(struct bl_pata_device *device,
		bl_uint8_t *buf)
{
	return bl_pata_device_command(device, BL_PATA_CMD_IDENTIFY_DEVICE, buf, 1, 0, 0);
}

static bl_status_t bl_pata_check_channel_device(int type, bl_port_t io)
{
	bl_status_t status;
	struct bl_pata_device *device;

	device = bl_heap_alloc(sizeof(struct bl_pata_device));
	if (!device) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	device->type = type;
	device->io = io;

	status = bl_pata_device_connected(device);
	if (status)
		goto _exit;

	device->present = 1;

	status = bl_pata_identify_device(device, (bl_uint8_t *)&device->id);
	if (status)
		goto _exit;

	bl_pata_device_add(device);

	return BL_STATUS_SUCCESS;

_exit:
	if (device)
		bl_heap_free(device, sizeof(struct bl_pata_device));

	return status;
}

static int bl_pata_get_port(int channel)
{
	switch (channel) {
		case 0:
			return BL_PATA_PRIMARY_PORT;

		case 1:
			return BL_PATA_SECONDARY_PORT;

		default:
			return -1;
	}
}

static bl_status_t bl_pata_pci_initialize(struct bl_pci_index i)
{
	int type, channel;
	bl_status_t status;
	bl_uint32_t bar, control_bar;

	/* Check PCI device. */
	status = bl_pci_check_device_class(i, BL_PCI_BASE_CLASS_STORAGE,
			BL_PCI_SUBCLASS_STORAGE_IDE, -1);
	if (status)
		return status;

	/* Check channels. */
	for (channel = 0; channel < 2; channel++) {
		int offset = channel * 2 * sizeof(bl_uint32_t);

		bar = bl_pci_read_config_long(i.bus, i.dev, i.func,
				BL_PCI_CONFIG_REG_BAR0 + offset);

		control_bar = bl_pci_read_config_long(i.bus, i.dev, i.func,
				BL_PCI_CONFIG_REG_BAR1 + offset);

		if (bar != 0x00 || control_bar != 0x00)
			continue;

		int port = bl_pata_get_port(channel);

		/* Support only master devices. */
		for (type = 0; type < 1; type++) {
			status = bl_pata_check_channel_device(type, port);
			if (status)
				return status;
		}
	}

	return BL_STATUS_SUCCESS;
}

static struct bl_disk_controller_functions pata_functions = {
	.type = BL_DISK_CONTROLLER_TYPE_PATA,
	.read = bl_pata_read,
	.get_info = NULL,
};

BL_MODULE_INIT()
{
	struct bl_pata_device *device;
	struct bl_disk_controller *controller;

	bl_pci_iterate_devices(bl_pata_pci_initialize);

	controller = bl_heap_alloc(sizeof(struct bl_disk_controller));
	if (!controller)
		return;

	controller->funcs = &pata_functions;
	controller->data = NULL;
	controller->next = NULL;

	bl_disk_controller_register(controller);

	device = bl_pata_device_list;
	while (device) {
		struct bl_storage_device *disk;

		disk = bl_heap_alloc(sizeof(struct bl_storage_device));
		if (!disk)
			return;

		disk->type = BL_STORAGE_TYPE_HARD_DRIVE;
		disk->controller = controller;
		disk->sector_size = BL_STORAGE_SECTOR_SIZE;
		disk->sector_count = device->id.sectors_lba48;
		disk->data = device;
		disk->next = NULL;

		bl_storage_device_register(disk);

		device = device->next;
	}
}

BL_MODULE_UNINIT()
{

}

