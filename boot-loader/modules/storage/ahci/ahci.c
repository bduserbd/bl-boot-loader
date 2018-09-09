#include "ahci.h"
#include "include/time.h"
#include "include/string.h"
#include "include/bl-utils.h"
#include "core/include/loader/loader.h"
#include "core/include/storage/storage.h"
#include "core/include/pci/pci.h"
#include "core/include/memory/heap.h"
#include "core/include/video/print.h"

BL_MODULE_NAME("Advanced Host Controller Interface");

struct bl_ahci_port {
	int implemented;

	volatile struct bl_ahci_port_registers *regs;

	volatile struct bl_ahci_command_header *command_list;

	volatile struct bl_ahci_command_table *command_table;

	volatile struct bl_ahci_received_fis *rfis;
};

struct bl_ahci_controller {
	volatile struct bl_ahci_generic_host_control *ghc;

	int number_of_ports;
	struct bl_ahci_port *ports;
	volatile struct bl_ahci_port_registers *port_regs;

	int command_slots;
};

struct bl_ahci_device {
	struct bl_sata_identification id;

	struct bl_ahci_port *port;
	struct bl_ahci_controller *controller;

	struct bl_ahci_device *next;
};
static struct bl_ahci_device *bl_ahci_device_list = NULL;

static inline void bl_ahci_device_add(struct bl_ahci_device *device)
{
	device->next = bl_ahci_device_list;
	bl_ahci_device_list = device;
}

static int bl_ahci_find_free_command_slot(struct bl_ahci_device *device)
{
	int i;
	bl_uint32_t slots;

	slots = device->port->regs->sact | device->port->regs->ci;

	for (i = 0; i < device->controller->command_slots; i++) {
		if ((slots & 0x1) == 0)
			return i;

		slots >>= 1;
	}

	return -1;
}

static bl_status_t bl_ahci_do_command(struct bl_ahci_device *device, int command,
		bl_uint8_t *buf, bl_uint64_t lba, bl_uint64_t sectors)
{
	int i;
	int slot;
	struct bl_ahci_fis_host_to_device *h2d;
	static const bl_uint64_t prdt_sectors = BL_AHCI_PRDT_MAX_DBC / BL_STORAGE_SECTOR_SIZE;

	if (sectors > (BL_AHCI_PRDT_ENTRIES * prdt_sectors))
		return BL_STATUS_INVALID_PARAMETERS;

	slot = bl_ahci_find_free_command_slot(device);
	if (slot == -1)
		return BL_STATUS_INSUFFICIENT_RESOURCES;

	/* Set the slot. */
	device->port->command_list[slot].cfl = sizeof(struct bl_ahci_fis_host_to_device) /
		sizeof(bl_uint32_t);

	device->port->command_list[slot].w = 0;

	device->port->command_list[slot].prdtl = sectors / prdt_sectors + ((sectors % prdt_sectors) > 0);

	/* Set command table & PRDT. */
	bl_memset((void *)&device->port->command_table[slot], 0, sizeof(struct bl_ahci_command_table));

	bl_int64_t count = sectors;
	bl_uint64_t off = 0;

	for (i = 0; i < device->port->command_list[slot].prdtl; i++) {
		device->port->command_table[slot].prdt[i].dba = (bl_uint32_t)buf + off;

		int read_size = BL_MIN(count, prdt_sectors) * BL_STORAGE_SECTOR_SIZE;

		device->port->command_table[slot].prdt[i].dbc = read_size - 1;

		device->port->command_table[slot].prdt[i].i = 1;

		off += read_size;
		count -= prdt_sectors;
	}

	/* Specific command. */
	h2d = device->port->command_table[slot].cfis;

	h2d->fis_type = BL_AHCI_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	h2d->c = 1;

	h2d->command = command;
	switch (h2d->command) {
	case BL_SATA_COMMAND_READ_SECTORS_DMA_EXT:
		h2d->lba0 = lba & 0xff;
		h2d->lba1 = (lba >> 8) & 0xff;
		h2d->lba2 = (lba >> 16) & 0xff;
		h2d->lba3 = (lba >> 24) & 0xff;
		h2d->lba4 = (lba >> 32) & 0xff;
		h2d->lba5 = (lba >> 40) & 0xff;

		h2d->count0 = sectors & 0xff;
		h2d->count1 = (sectors >> 8) & 0xff;

		h2d->device = (1 << 6); /* LBA */

		break;

	default:
		break;
	}

	device->port->regs->ci |= (1 << slot);

	/* Status. */
	int timeout = 100;
	while (--timeout) {
		if ((device->port->regs->ci & (1 << slot)) == 0)
			break;

		bl_time_sleep(1);
	}

	if (device->port->regs->ci & (1 << slot))
		return BL_STATUS_DISK_OPERATION_TIMEOUT;

	if (device->port->regs->is & BL_AHCI_PORT_IS_TFES)
		return BL_STATUS_DISK_CONTROLLER_INTERNAL_ERROR;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ahci_read(struct bl_storage_device *disk, bl_uint8_t *buf,
		bl_uint64_t lba, bl_uint64_t sectors)
{
	bl_status_t status;
	struct bl_ahci_device *device;

	device = disk->data;

	status = bl_ahci_do_command(device, BL_SATA_COMMAND_READ_SECTORS_DMA_EXT, buf,
			lba, sectors);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ahci_identify_device(struct bl_ahci_device *device)
{
	bl_status_t status;

	status = bl_ahci_do_command(device, BL_SATA_COMMAND_IDENTIFY, (bl_uint8_t *)&device->id, 0, 1);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ahci_detect_devices(struct bl_ahci_controller *ahci)
{
	int i;

	/* Enable ports again. */
	for (i = 0; i < ahci->number_of_ports; i++) {
		if (!ahci->ports[i].implemented)
			continue;

		if ((ahci->port_regs[i].tfd & (BL_AHCI_PORT_TFD_STS_DRQ | BL_AHCI_PORT_TFD_STS_BSY)) == 0 &&
				BL_AHCI_PORT_STSS_DET(ahci->port_regs[i].ssts) == BL_AHCI_PORT_SSTS_DET_PRESENT_PHYS) {
			ahci->port_regs[i].cmd |= BL_AHCI_PORT_CMD_ST;

			struct bl_ahci_device *device = bl_heap_alloc(sizeof(struct bl_ahci_device));
			if (!device)
				return BL_STATUS_MEMORY_ALLOCATION_FAILED;

			device->port = &ahci->ports[i];
			device->controller = ahci;

			bl_ahci_identify_device(device);

			bl_ahci_device_add(device);
		}
	}

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ahci_controller_init(struct bl_ahci_controller *ahci)
{
	int i, j;
	bl_status_t status;

	/* AHCI aware. */
	ahci->ghc->ghc |= BL_AHCI_GHC_AE;

	/* Perform reset. */
	ahci->ghc->ghc |= BL_AHCI_GHC_HR;

	int timeout = 1000;
	while (--timeout) {
		if ((ahci->ghc->ghc & BL_AHCI_GHC_HR) == 0)
			break;

		bl_time_sleep(1);
	}

	if (ahci->ghc->ghc & BL_AHCI_GHC_HR)
		return BL_STATUS_DISK_CONTROLLER_INTERNAL_ERROR;

	/* AHCI aware again ? */
	ahci->ghc->ghc |= BL_AHCI_GHC_AE;

	/* Number of ports. */
	ahci->number_of_ports = 1 + (ahci->ghc->cap & BL_AHCI_CAP_NP);
	ahci->ports = bl_heap_alloc(ahci->number_of_ports * sizeof(struct bl_ahci_port));
	if (!ahci->ports)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	ahci->port_regs = (bl_uint8_t *)ahci->ghc + 0x100;

	for (i = 0; i < ahci->number_of_ports; i++) {
		if ((ahci->ghc->pi & (1 << i)) == 0) {
			ahci->ports[i].implemented = 0;
			continue;
		}

		if (ahci->port_regs[i].cmd & (BL_AHCI_PORT_CMD_ST | BL_AHCI_PORT_CMD_FRE |
					BL_AHCI_PORT_CMD_FR | BL_AHCI_PORT_CMD_CR)) {
			ahci->port_regs[i].cmd &= ~BL_AHCI_PORT_CMD_ST;
			bl_time_sleep(500);

			if (ahci->port_regs[i].cmd & BL_AHCI_PORT_CMD_CR)
				return BL_STATUS_DISK_CONTROLLER_INTERNAL_ERROR;

			if (ahci->port_regs[i].cmd & BL_AHCI_PORT_CMD_FRE) {
				ahci->port_regs[i].cmd &= ~BL_AHCI_PORT_CMD_FRE;
				bl_time_sleep(500);

				if (ahci->port_regs[i].cmd & BL_AHCI_PORT_CMD_FR)
					return BL_STATUS_DISK_CONTROLLER_INTERNAL_ERROR;
			}
		}

		ahci->ports[i].implemented = 1;
	}

	/* Number of command slots. */
	ahci->command_slots = 1 + BL_AHCI_CAP_NCS(ahci->ghc->cap);

	/* Allocate PxCLB and PxFB. */
	for (i = 0; i < ahci->number_of_ports; i++) {
		if (!ahci->ports[i].implemented)
			continue;

		ahci->ports[i].regs = &ahci->port_regs[i];

		/* Command slots. */
		ahci->ports[i].command_list = bl_heap_alloc_align(ahci->command_slots *
				sizeof(struct bl_ahci_command_header), 0x400);
		if (!ahci->ports[i].command_list)
			return BL_STATUS_MEMORY_ALLOCATION_FAILED;

		ahci->port_regs[i].clb = (bl_uint32_t)ahci->ports[i].command_list;

		/* Command table for each slot. */
		ahci->ports[i].command_table = bl_heap_alloc_align(ahci->command_slots *
				sizeof(struct bl_ahci_command_table), 0x400);
		if (!ahci->ports[i].command_table)
			return BL_STATUS_MEMORY_ALLOCATION_FAILED;

		for (j = 0; j < ahci->command_slots; j++)
			ahci->ports[i].command_list[j].ctba = (bl_uint32_t)&ahci->ports[i].command_table[j];

		/* Received FIS. */
		ahci->ports[i].rfis =  bl_heap_alloc_align(sizeof(struct bl_ahci_received_fis), 0x1000);
		if (!ahci->ports[i].rfis)
			return BL_STATUS_MEMORY_ALLOCATION_FAILED;

		ahci->port_regs[i].fb = (bl_uint32_t)ahci->ports[i].rfis;

		/* Set FRE to 1. */
		ahci->port_regs[i].cmd |= BL_AHCI_PORT_CMD_FRE;
	}

	/* Clear PxSERR register. */
	for (i = 0; i < ahci->number_of_ports; i++)
		if (ahci->ports[i].implemented)
			ahci->port_regs[i].serr = ahci->port_regs[i].serr;

	/* Checkout for connected devices. */
	status = bl_ahci_detect_devices(ahci);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ahci_pci_initialize(struct bl_pci_index i)
{
	bl_status_t status;
	bl_uint32_t base_address;
	struct bl_ahci_controller *ahci;

	/* Check PCI device. */
	status = bl_pci_check_device_class(i, BL_PCI_BASE_CLASS_STORAGE,
			BL_PCI_SUBCLASS_STORAGE_SATA, BL_PCI_PROG_IF_STORAGE_SATA_AHCI);
	if (status)
		return status;

	/* Configuration area is mapped to memory and is not perfetchable. */
	base_address = bl_pci_read_config_long(i.bus, i.dev, i.func, BL_PCI_CONFIG_REG_BAR5);
	if (base_address & (BL_AHCI_PCI_BAR5_RTE | BL_AHCI_PCI_BAR5_TP | BL_AHCI_PCI_BAR5_PF))
		return BL_STATUS_DISK_CONTROLLER_INTERNAL_ERROR;

	/* Initialize the controller. */
	ahci = bl_heap_alloc(sizeof(struct bl_ahci_controller));
	if (!ahci)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	ahci->ghc = (void *)(base_address & BL_AHCI_PCI_BAR5_BA);

	status = bl_ahci_controller_init(ahci);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static struct bl_disk_controller_functions ahci_functions = {
	.type = BL_DISK_CONTROLLER_TYPE_AHCI,
	.read = bl_ahci_read,
	.get_info = NULL,
};

BL_MODULE_INIT()
{
	struct bl_ahci_device *device;
	struct bl_disk_controller *controller;

	bl_pci_iterate_devices(bl_ahci_pci_initialize);

	controller = bl_heap_alloc(sizeof(struct bl_disk_controller));
	if (!controller)
		return;

	controller->funcs = &ahci_functions;
	controller->data = NULL;
	controller->next = NULL;

	bl_disk_controller_register(controller);

	device = bl_ahci_device_list;
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

