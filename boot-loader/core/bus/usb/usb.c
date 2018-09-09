#include "include/export.h"
#include "include/time.h"
#include "include/string.h"
#include "core/include/usb/usb.h"
#include "core/include/storage/storage.h"
#include "core/include/keyboard/keyboard.h"
#include "core/include/video/print.h"
#include "core/include/memory/heap.h"

static int bl_usb_buses = 0; // Number of host controllers.
static struct bl_usb_host_controller *bl_usb_host_controllers = NULL;

static inline void bl_usb_setup_data_fill(struct bl_usb_setup_data *setup,
	int request_type, int request, int value, int index, int length)
{
	setup->request_type = request_type;
	setup->request = request;
	setup->value = value;
	setup->index = index;
	setup->length = length;
}

void bl_usb_control_transfer(struct bl_usb_device *device, void *data,
	int request_type, int request, int value, int index, int length)
{
	struct bl_usb_setup_data setup;

	bl_usb_setup_data_fill(&setup, request_type, request, value, index, length);

	device->controller->funcs->control_transfer(device->controller,
		device->dev_desc.max_packet_size, device->address, device->speed,
		&setup, (u8 *)data, length);
}
BL_EXPORT_FUNC(bl_usb_control_transfer);

void bl_usb_get_descriptor(struct bl_usb_device *device, int flags, int type,
	int index, int language, void *data, int length)
{
	bl_usb_control_transfer(device, data,
		BL_USB_SETUP_REQUEST_TYPE_DEVICE_TO_HOST | flags,
		BL_USB_REQUEST_TYPE_GET_DESCRIPTOR,
		(type << 8) | (index & 0xff), language, length);
}
BL_EXPORT_FUNC(bl_usb_get_descriptor);

void bl_usb_bulk_transfer(struct bl_usb_device *device, int direction,
	void *data, int length)
{
	struct bl_usb_endpoint *endp;

	if (direction)
		endp = &device->specific.storage.out_endp;
	else
		endp = &device->specific.storage.in_endp;

	device->controller->funcs->bulk_transfer(device->controller,
		device->address, endp, device->speed,
		direction, (u8 *)data, length);
}
BL_EXPORT_FUNC(bl_usb_bulk_transfer);

bl_usb_transfer_info_t bl_usb_interrupt_transfer(struct bl_usb_device *device,
	struct bl_usb_endpoint *endp, void *data, int length)
{
	return device->controller->funcs->interrupt_transfer(device->controller,
		device->address, endp, device->speed,
		endp->descriptor->endpoint_address & 0x80 ? 0 : 1,
		(u8 *)data, length);
}
BL_EXPORT_FUNC(bl_usb_interrupt_transfer);

bl_status_t bl_usb_check_transfer_status(struct bl_usb_device *device,
	bl_usb_transfer_info_t info)
{
	return device->controller->funcs->check_transfer_status(info);
}
BL_EXPORT_FUNC(bl_usb_check_transfer_status);

static void bl_usb_identify_device(struct bl_usb_device *device)
{
	int i, j;
	int off;
	bl_uint8_t *config_data;
	struct bl_usb_configuration_descriptor config;

	/* Enough to get the maximum packet size.*/
	device->dev_desc.max_packet_size = 0;
	bl_usb_get_descriptor(device, 0, BL_USB_DESCRIPTOR_TYPE_DEVICE, 0, 0,
		&device->dev_desc, 8);

	/* Most devices have only one configuration. Still check it. */
	bl_usb_get_descriptor(device, 0, BL_USB_DESCRIPTOR_TYPE_DEVICE, 0, 0,
		&device->dev_desc, sizeof(struct bl_usb_device_descriptor));

#if 0
	bl_print_str("Dev : ");
	bl_print_hex(device->dev_desc.length); bl_print_str(" ");
	bl_print_hex(device->dev_desc.descriptor_type); bl_print_str(" ");
	bl_print_hex(device->dev_desc.release_number); bl_print_str(" ");
	bl_print_hex(device->dev_desc.vendor_id); bl_print_str(" ");
	bl_print_hex(device->dev_desc.product_id); bl_print_str("\n");
#endif

	if (device->dev_desc.configurations_count != 1)
		return;

	bl_usb_get_descriptor(device, 0, BL_USB_DESCRIPTOR_TYPE_CONFIG, 0, 0,
		&config, sizeof(struct bl_usb_configuration_descriptor));

	bl_memcpy(&device->config, &config, sizeof(struct bl_usb_configuration_descriptor));

#if 0
	bl_print_str("Conf : ");
	bl_print_hex(config.length); bl_print_str(" ");
	bl_print_hex(config.descriptor_type); bl_print_str(" ");
	bl_print_hex(config.total_length); bl_print_str(" ");
	bl_print_hex(config.interfaces_count); bl_print_str("\n");
#endif

	/* Configuration information such as its interfaces, endpoints
	   are not accessible by the GET_DESCRIPTOR command.
	   The configuration descriptor request returns them all.
	   Usually the interface appears first and then all its endpoints. */
	config_data = bl_heap_alloc(config.total_length);
	if (!config_data)
		return;

	bl_usb_get_descriptor(device, 0, BL_USB_DESCRIPTOR_TYPE_CONFIG, 0, 0,
		config_data, config.total_length);

	/* Allocate space for device interfaces. */
	device->interfaces = bl_heap_alloc(config.interfaces_count *
		sizeof(struct bl_usb_interface_descriptor));
	if (!device->interfaces)
		return;

	/* Iterate on available interfaces. */
	off = config.length;
	for (i = 0; i < config.interfaces_count; i++) {
		struct bl_usb_interface_descriptor *interface;
		struct bl_usb_endpoint_descriptor *endpoint;

		while (1) {
			interface = (struct bl_usb_interface_descriptor *)(config_data + off);
			if (interface->descriptor_type != BL_USB_DESCRIPTOR_TYPE_INTERFACE)
				off += interface->length;
			else
				break;

			if (off > config.total_length) {
				// TODO
			}
		}
		bl_memcpy(&device->interfaces[i].descriptor, interface,
			sizeof(struct bl_usb_interface_descriptor));

		/* Allocate space for device endpoints. */
		device->interfaces[i].endpoints = bl_heap_alloc(interface->num_endpoints *
			sizeof(struct bl_usb_endpoint_descriptor));
		if (!device->interfaces[i].endpoints)
			return;

		/* Iterate on available endpoints. */
		off += interface->length;
		for (j = 0; j < interface->num_endpoints;) {
			endpoint = (struct bl_usb_endpoint_descriptor *)(config_data + off);

			if (endpoint->descriptor_type == BL_USB_DESCRIPTOR_TYPE_ENDPOINT) {
				bl_memcpy(&device->interfaces[i].endpoints[j], endpoint,
					sizeof(struct bl_usb_endpoint_descriptor));
				j++;
			}

			off += endpoint->length; /* The start of all USB descriptors
						     is the same: Length and type. */

			if (off > config.total_length) {
				// TODO
			}
		}
	}

	bl_heap_free(config_data, config.total_length);
}

void bl_usb_dump_index_string(struct bl_usb_device *device, int index)
{
	int i;
	struct bl_usb_string_descriptor desc, *desc_ptr;

	/* Nothing to print. */
	if (index == 0)
		return;

	bl_usb_get_descriptor(device, 0, BL_USB_DESCRIPTOR_TYPE_STRING,
		index, 0x0409, &desc, 2);

	desc_ptr = bl_heap_alloc(desc.length);
	if (!desc_ptr)
		return;
	bl_usb_get_descriptor(device, 0, BL_USB_DESCRIPTOR_TYPE_STRING,
		index, 0x0409, desc_ptr, desc.length);

	char s[2];
	s[1] = 0;
	for (i = 0; i < (desc.length - 2) / 2; i++) {
		s[0] = desc_ptr->code[i] & 0xff;
		bl_print_str(s);
	}
	bl_print_str("\n");

	bl_heap_free(desc_ptr, desc.length);
}

static void bl_usb_dump_device_info(struct bl_usb_device *device)
{
	int i, j;

	bl_usb_dump_index_string(device, device->dev_desc.manufacturer_str);
	bl_usb_dump_index_string(device, device->dev_desc.product_str);
	bl_usb_dump_index_string(device, device->dev_desc.serial_number_str);

	for (i = 0; i < device->config.interfaces_count; i++) {
		for (j = 0; j < device->interfaces[i].descriptor.num_endpoints; j++) {
			bl_print_str("Endpoint Address: ");
			bl_print_hex(device->interfaces[i].endpoints[j].endpoint_address);
			bl_print_str("\n");

			bl_print_str("Attributes: ");
			bl_print_hex(device->interfaces[i].endpoints[j].attributes);
			bl_print_str("\n");

			bl_print_str("Max Packet size: ");
			bl_print_hex(device->interfaces[i].endpoints[j].max_packet_size);
			bl_print_str("\n");
		}
	}
}

static void bl_usb_dump_device_id(struct bl_usb_device *device)
{
	int i;

	switch (device->type) {
	case BL_USB_HUB_DEVICE:
		bl_print_str("Hub - ");
		break;

	case BL_USB_STORAGE_DEVICE:
		bl_print_str("Storage - ");
		break;

	case BL_USB_KEYBOARD_DEVICE:
		bl_print_str("Keyboard - ");
		break;
	}
	
	bl_print_str("Address:");
	bl_print_hex(device->address);
	bl_print_str(". ");

	bl_print_str("Port:");
	if (device->type == BL_USB_HUB_DEVICE) {
		for (i = 0; device->specific.hub.hub->port_chain[i]; i++) {
			bl_print_hex(device->specific.hub.hub->port_chain[i]);
			bl_print_str(".");
		}
	} else {
		for (i = 0; device->parent_hub->port_chain[i]; i++) {
			bl_print_hex(device->parent_hub->port_chain[i]);
			bl_print_str(".");
		}
		bl_print_hex(device->port);
		bl_print_str(".");
	}
	bl_print_str(" ");

	bl_print_str("Vendor:");
	bl_print_hex(device->dev_desc.vendor_id);
	bl_print_str(". ");

	bl_print_str("Product:");
	bl_print_hex(device->dev_desc.product_id);
	bl_print_str(".\n");
}

static void bl_usb_dump_host_controller_devices(struct bl_usb_host_controller *hc)
{
	int i;
	struct bl_usb_hub *hub;
	struct bl_usb_traversal *traversal;

	traversal = bl_usb_traversal_init();
	hub = &hc->root_hub;
	bl_usb_traversal_push(traversal, hub);

	while (!bl_usb_traversal_is_empty(traversal)) {
		hub = bl_usb_traversal_pop(traversal);

		for (i = 0; i < hub->ports; i++) {
			if (!hub->devices[i])
				continue;

			bl_usb_dump_device_id(hub->devices[i]);
#if 0
			bl_print_str("B:");
			bl_print_hex(hub);
			bl_print_str("\n");
#endif

			if (hub->devices[i]->type == BL_USB_HUB_DEVICE)
				bl_usb_traversal_push(traversal,
					 hub->devices[i]->specific.hub.hub);
		}
	}

	bl_usb_traversal_uninit(traversal);
}

void bl_usb_dump(void)
{
	struct bl_usb_host_controller *hc;

	for (hc = bl_usb_host_controllers; hc; hc = hc->next) {
		bl_print_str("Bus : ");
		bl_print_hex(hc->bus);

		switch (hc->type) {
		case BL_USB_HC_UHCI:
			bl_print_str(" - UHCI (1.5 Mb/s|12 Mb/s) :\n");
			break;

		case BL_USB_HC_OHCI:
			bl_print_str(" - OHCI (1.5 Mb/s|12 Mb/s) :\n");
			break;

		case BL_USB_HC_EHCI:
			bl_print_str(" - EHCI (480 Mb/s) :\n");
			break;

		case BL_USB_HC_XHCI:
			bl_print_str(" - XHCI (5000 Mb/s) :\n");
			break;
		}

		bl_usb_dump_host_controller_devices(hc);
	}
}

static bl_status_t bl_usb_get_hid_info(struct bl_usb_device *device)
{
	int off;
	bl_status_t status;
	struct bl_usb_interface *interface;
	struct bl_usb_hid_descriptor *hid;
	bl_uint8_t *config_data;

	interface = &device->interfaces[device->specific.keyboard.interface];

	/* Get info. */
	config_data = bl_heap_alloc(device->config.total_length);
	if (!config_data) {
		status = BL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto _exit;
	}

	bl_usb_get_descriptor(device, 0, BL_USB_DESCRIPTOR_TYPE_CONFIG, 0, 0,
		config_data, device->config.total_length);

	/* Find our interface. */
	off = device->config.length;
	while (1) {
		hid = (struct bl_usb_hid_descriptor *)(config_data + off);
		if (hid->descriptor_type == BL_USB_DESCRIPTOR_TYPE_INTERFACE) {
			if (((struct bl_usb_interface_descriptor *)hid)->interface_number
				== interface->descriptor.interface_number)
				break;
		} else
			off += hid->length;
	}	

	/* Retrieve the HID descriptor. */
	off += hid->length;
	while (1) {
		hid = (struct bl_usb_hid_descriptor *)(config_data + off);
		if (hid->descriptor_type == BL_USB_DESCRIPTOR_TYPE_HID)
			break;
		else
			off += hid->length;
	}

	bl_memcpy(&device->specific.keyboard.hid, hid, sizeof(struct bl_usb_hid_descriptor));

	status = BL_STATUS_SUCCESS;

_exit:
	if (config_data)
		bl_heap_free(config_data, device->config.total_length);

	return status;
}

static void bl_usb_check_hub(struct bl_usb_device *device)
{
	int i;
	struct bl_usb_hub *hub;
	struct bl_usb_hub_descriptor descriptor;

	hub = bl_heap_alloc(sizeof(struct bl_usb_hub));
	if (!hub)
		goto _exit;

	/* Basic info. */
	bl_usb_hub_get_hub_descriptor(device, &descriptor);

	hub->ports = descriptor.ports;
	hub->device = device;

	hub->devices = bl_heap_alloc(hub->ports * sizeof(struct bl_usb_hub));
	if (!hub->devices)
		goto _exit;

	device->type = BL_USB_HUB_DEVICE;
	device->specific.hub.hub = hub;

	/* Hub port chain depth. */
	bl_memcpy(hub->port_chain, device->parent_hub->port_chain,
		sizeof(hub->port_chain));

	for (i = 0; hub->port_chain[i]; i++) ;

	hub->port_chain[i] = device->port;

	return;

_exit:
	if (hub) {
		if (hub->devices)
			bl_heap_free(hub->devices, hub->ports * sizeof(struct bl_usb_hub));

		bl_heap_free(hub, sizeof(struct bl_usb_hub));
	}	
}

static bl_status_t bl_usb_claim_keyboard(struct bl_usb_device *device)
{
	struct bl_keyboard *kbd;

	kbd = bl_heap_alloc(sizeof(struct bl_keyboard));
	if (!kbd)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	kbd->type = BL_KEYBOARD_USB;
	kbd->data = device;
	kbd->next = NULL;

	device->specific.keyboard.device = kbd;

	bl_keyboard_register(kbd);

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_usb_check_keyboard(struct bl_usb_device *device)
{
	int i;
	bl_status_t status;

        for (i = 0; i < device->config.interfaces_count; i++) {
		if (device->interfaces[i].descriptor.interface_class != 0x3 ||
			device->interfaces[i].descriptor.interface_protocol != 0x1)
			continue;

		device->type = BL_USB_KEYBOARD_DEVICE;
		device->specific.keyboard.interface =
			device->interfaces[i].descriptor.interface_number;

		if ((status = bl_usb_get_hid_info(device)) ||
			(status = bl_usb_claim_keyboard(device)))
			return status;

		return BL_STATUS_SUCCESS;
	}

	return BL_STATUS_USB_INVALID_DEVICE;
}

static bl_status_t bl_usb_claim_storage_device(struct bl_usb_device *device)
{
	struct bl_storage_device *disk;

	disk = bl_heap_alloc(sizeof(struct bl_storage_device));
	if (!disk)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	disk->type = BL_STORAGE_TYPE_USB_DRIVE;
	disk->data = device;
	disk->next = NULL;

	bl_storage_device_register(disk);

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_usb_check_storage_device(struct bl_usb_device *device)
{
        int i;
	bl_status_t status;

        for (i = 0; i < device->config.interfaces_count; i++) {
		if (device->interfaces[i].descriptor.interface_class != 0x8)
			continue;

		device->type = BL_USB_STORAGE_DEVICE;
		device->specific.storage.interface =
			device->interfaces[i].descriptor.interface_number;

		status = bl_usb_claim_storage_device(device);
		if (status)
			return status;

		return BL_STATUS_SUCCESS;
        }

	return BL_STATUS_USB_INVALID_DEVICE;
}

void bl_usb_device_register_specific_info(struct bl_usb_device *device)
{
	switch (device->dev_desc.device_class) {
	case 0x00:
		(void)(!bl_usb_check_keyboard(device) ||
		       !bl_usb_check_storage_device(device));
		break;

	case 0x09:
		bl_usb_check_hub(device);
		break;

	default:
		break;
	}
}

struct bl_usb_device *bl_usb_device_register(struct bl_usb_host_controller *hc,
	bl_usb_speed_t speed)
{
	struct bl_usb_device *device = bl_heap_alloc(sizeof(struct bl_usb_device));
	if (!device)
		return NULL;

	bl_memset(&device->specific, 0, sizeof(device->specific));
	device->speed = speed;
	device->controller = hc;

	if (device->controller->type == BL_USB_HC_XHCI)
		device->address = device->controller->devices_count;

	bl_usb_identify_device(device);

	/* Set address & configuration. */
	if (device->controller->type != BL_USB_HC_XHCI) {
		device->address = 0; // Reset.

		bl_usb_control_transfer(device, NULL,
			BL_USB_SETUP_REQUEST_TYPE_HOST_TO_DEVICE,
			BL_USB_REQUEST_TYPE_SET_ADDRESS,
			++device->controller->devices_count, 0, 0);

		bl_time_sleep(2);
		device->address = device->controller->devices_count;
	}

	bl_usb_control_transfer(device, NULL,
		BL_USB_SETUP_REQUEST_TYPE_HOST_TO_DEVICE |
		BL_USB_SETUP_REQUEST_TYPE_STANDARD |
		BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_DEVICE,
		BL_USB_REQUEST_TYPE_SET_CONFIGURATION,
		1, 0, 0);

	bl_usb_dump_device_info(device);

	return device;
}

void bl_usb_setup(void)
{
	struct bl_usb_host_controller *hc;

	hc = bl_usb_host_controllers;
	while (hc) {
		hc->bus = ++bl_usb_buses;
		hc->devices_count = 0;
		bl_usb_hub_root_register(hc);

		hc = hc->next;
	}

	//bl_usb_poll();
}

void bl_usb_host_controller_register(struct bl_usb_host_controller *hc)
{
	hc->next = bl_usb_host_controllers;
	bl_usb_host_controllers = hc;
}
BL_EXPORT_FUNC(bl_usb_host_controller_register);

void bl_usb_host_controller_unregister(struct bl_usb_host_controller *hc)
{

}
BL_EXPORT_FUNC(bl_usb_host_controller_unregister);

