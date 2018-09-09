#include "include/export.h"
#include "core/include/pci/pci.h"
#include "core/include/video/print.h"

static void bl_pci_dump_device(u32 bus, u32 dev, u32 func)
{
	u8 class, subclass, prog_if;
	u16 vendor, device_id;

	vendor = bl_pci_read_config_word(bus, dev, func, BL_PCI_CONFIG_REG_VENDOR_ID);
	device_id = bl_pci_read_config_word(bus, dev, func, BL_PCI_CONFIG_REG_DEVICE_ID);
	class = bl_pci_read_config_byte(bus, dev, func, BL_PCI_CONFIG_REG_BASECLASS);
	subclass = bl_pci_read_config_byte(bus, dev, func, BL_PCI_CONFIG_REG_SUBCLASS);
	prog_if = bl_pci_read_config_byte(bus, dev, func, BL_PCI_CONFIG_REG_PROG_IF);

	bl_print_str("Vendor: ");
	bl_print_hex(vendor);

	bl_print_str("  Device id: ");
	bl_print_hex(device_id);

	bl_print_str("  Class: ");
	bl_print_hex(class);

	bl_print_str("  Subclass: ");
	bl_print_hex(subclass);

	bl_print_str("  PROG-IF: ");
	bl_print_hex(prog_if);
	
	bl_print_str("\n");
}

static void bl_pci_discover_devices(pci_init_device_t hook, int dump)
{
	u8 header;
	u16 vendor;
	struct bl_pci_index i;

	for (i.bus = 0; i.bus < BL_PCI_NBUSES; i.bus++) {
		for (i.dev = 0; i.dev < BL_PCI_NDEVICES; i.dev++) {
			for (i.func = 0; i.func < BL_PCI_NFUNCTIONS; i.func++) {
				vendor = bl_pci_read_config_word(i.bus, i.dev, i.func,
					BL_PCI_CONFIG_REG_VENDOR_ID);

				/* Invalid device */
				if (vendor == 0xffff)
					continue;

				if (dump)
					bl_pci_dump_device(i.bus, i.dev, i.func);

				if (hook)
					hook(i);

				if (i.func == 0) {
					header = bl_pci_read_config_byte(i.bus, i.dev, i.func,
						BL_PCI_CONFIG_REG_HEADER_TYPE);

					/* If the 7th bit is 0, the device has a single function */
					if (!(header & 0x80))
						break;
				}
			}
		}
	}
}

void bl_pci_iterate_devices(pci_init_device_t hook)
{
	bl_pci_discover_devices(hook, 0);
}
BL_EXPORT_FUNC(bl_pci_iterate_devices);

void bl_pci_dump_devices(void)
{
	bl_pci_discover_devices(NULL, 1);
}
BL_EXPORT_FUNC(bl_pci_dump_devices);

bl_status_t bl_pci_check_device_class(struct bl_pci_index i, int class, int subclass,
	int interface)
{
	if (class != -1)
		if (class != bl_pci_read_config_byte(i.bus, i.dev, i.func,
			BL_PCI_CONFIG_REG_BASECLASS))
			return BL_STATUS_FAILURE;

	if (subclass != -1)
        	if (subclass != bl_pci_read_config_byte(i.bus, i.dev, i.func,
			BL_PCI_CONFIG_REG_SUBCLASS))
			return BL_STATUS_FAILURE;

	if (interface != -1)
        	if (interface != bl_pci_read_config_byte(i.bus, i.dev, i.func,
			BL_PCI_CONFIG_REG_PROG_IF))
			return BL_STATUS_FAILURE;

	return BL_STATUS_SUCCESS;
}
BL_EXPORT_FUNC(bl_pci_check_device_class);
 
