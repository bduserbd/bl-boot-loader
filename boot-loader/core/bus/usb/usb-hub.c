#include "include/time.h"
#include "include/string.h"
#include "core/include/usb/usb.h"
#include "core/include/video/print.h"
#include "core/include/memory/heap.h"

/*
 * USB hub main parts:
 * 1) Hub repeater.
 * 2) Hub controller.
 * 3) Transaction translator - High speed split transactions to full & low
 *    transactions.
 *
 * Downstream info is broadcasted while upstream mode info goes through
 * relevant connected hubs.
 *
 * USB hub chain length is limited to 5 hubs connected.
 */

static struct bl_usb_hub *bl_root_hubs = NULL;
//static struct bl_usb_hub *bl_hubs = NULL;

void bl_usb_hub_get_hub_descriptor(struct bl_usb_device *device,
	struct bl_usb_hub_descriptor *descriptor)
{
	bl_usb_control_transfer(device, (void *)descriptor,
                BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_DEVICE |
                BL_USB_SETUP_REQUEST_TYPE_CLASS |
		BL_USB_SETUP_REQUEST_TYPE_DEVICE_TO_HOST,
		BL_USB_HUB_REQUEST_TYPE_GET_DESCRIPTOR,
		BL_USB_DESCRIPTOR_TYPE_HUB << 8, 0,
		sizeof(struct bl_usb_hub_descriptor));
}

void bl_usb_hub_get_port_status(struct bl_usb_device *device, int port,
	struct bl_usb_hub_port_status *info)
{
	bl_usb_control_transfer(device, (void *)info,
		BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_OTHER |
		BL_USB_SETUP_REQUEST_TYPE_CLASS |
		BL_USB_SETUP_REQUEST_TYPE_DEVICE_TO_HOST,
		BL_USB_HUB_REQUEST_TYPE_GET_STATUS,
		0, port, sizeof(struct bl_usb_hub_port_status));
}

void bl_usb_hub_clear_port_feature(struct bl_usb_device *device, int port, int feature)
{
	bl_usb_control_transfer(device, NULL,
		BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_OTHER |
		BL_USB_SETUP_REQUEST_TYPE_CLASS |
		BL_USB_SETUP_REQUEST_TYPE_HOST_TO_DEVICE,
		BL_USB_HUB_REQUEST_TYPE_CLEAR_FEATURE,
		feature, port, 0);
}

void bl_usb_hub_set_port_feature(struct bl_usb_device *device, int port, int feature)
{
	bl_usb_control_transfer(device, NULL,
		BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_OTHER |
		BL_USB_SETUP_REQUEST_TYPE_CLASS |
		BL_USB_SETUP_REQUEST_TYPE_HOST_TO_DEVICE,
		BL_USB_HUB_REQUEST_TYPE_SET_FEATURE,
		feature, port, 0);
}

static void bl_usb_hub_reset_port(struct bl_usb_device *device, int port)
{
	int i;
	struct bl_usb_hub_port_status info;

	bl_usb_hub_set_port_feature(device, port, BL_USB_HUB_FEATURE_PORT_RESET);

#define CHECK_TIMES	10
	for (i = 0; i < CHECK_TIMES; i++) {
		bl_usb_hub_get_port_status(device, port, &info);

		if (info.port_change & BL_USB_HUB_PORT_CHANGE_RESET)
			break;

		bl_time_sleep(10);			
	}

	if (i == CHECK_TIMES) {
		return;
	}

	bl_usb_hub_clear_port_feature(device, port, BL_USB_HUB_FEATURE_C_PORT_RESET);
}

#if 0
	bl_usb_hub_get_port_status(hub, i, &info);
	bl_print_hex(info.port_status);
	bl_print_str("\n");
	bl_print_hex(info.port_change);
	bl_print_str("\n");
#endif

static bl_usb_speed_t bl_usb_hub_get_port_device_speed(struct bl_usb_device *device,
	int port)
{
	struct bl_usb_hub_port_status info;

	bl_usb_hub_get_port_status(device, port, &info);

	if (info.port_status & BL_USB_HUB_PORT_STATUS_LOW_SPEED_DEVICE)
		return BL_USB_LOW_SPEED;
	else if (info.port_status & BL_USB_HUB_PORT_STATUS_HIGH_SPEED_DEVICE)
		return BL_USB_HIGH_SPEED;
	else
		return BL_USB_FULL_SPEED;
}

static void bl_usb_hub_link_device(struct bl_usb_hub *hub, struct bl_usb_device *device,
	int port)
{
	device->port = port;
	device->parent_hub = hub;

	hub->devices[port - 1] = device;
}

static void bl_usb_hub_poll(struct bl_usb_hub *hub)
{
	int i;
	struct bl_usb_traversal *traversal;
	struct bl_usb_device *port_device;
	struct bl_usb_hub_port_status info;

	traversal = bl_usb_traversal_init();
	bl_usb_traversal_push(traversal, hub);

	while (!bl_usb_traversal_is_empty(traversal)) {
		hub = bl_usb_traversal_pop(traversal);

		for (i = 1; i <= hub->ports; i++) {
			bl_usb_hub_get_port_status(hub->device, i, &info);

			if (info.port_change & BL_USB_HUB_PORT_CHANGE_CONNECT_STATUS) {
				bl_usb_hub_clear_port_feature(hub->device, i,
					BL_USB_HUB_FEATURE_C_PORT_CONNECTION);

				if (info.port_status & BL_USB_HUB_PORT_STATUS_CONNECT_STATUS) {
					bl_usb_hub_reset_port(hub->device, i);

					port_device = bl_usb_device_register(hub->device->controller,
						bl_usb_hub_get_port_device_speed(hub->device, i));

					if (port_device) {
						bl_usb_hub_link_device(hub, port_device, i);
						bl_usb_device_register_specific_info(port_device);

						if (port_device->type == BL_USB_HUB_DEVICE)
							bl_usb_traversal_push(traversal,
								port_device->specific.hub.hub);

						continue;
					}
				}
			}
		}
	}

	bl_usb_traversal_uninit(traversal);
}

void bl_usb_hub_root_register(struct bl_usb_host_controller *hc)
{
	int i, ports;
	bl_usb_speed_t speed;
	struct bl_usb_device *device;

	/* Set root hub. */
	hc->root_hub.device = NULL; // Not actually a USB device.

	ports = hc->root_hub.ports;

	hc->root_hub.devices = bl_heap_alloc(ports * sizeof(struct bl_usb_device));
	if (!hc->root_hub.devices)
		return;

	bl_memset(hc->root_hub.port_chain, 0, sizeof(hc->root_hub.port_chain));

	/* Iterate root hub ports. */
	for (i = 0; i < ports; i++) {
		/* Do we have a proper device ? */
		if (!hc->funcs->port_device_connected(hc, i)) {
			hc->root_hub.devices[i] = NULL;
			continue;
		}


		if (hc->type == BL_USB_HC_XHCI)
			/* Use Address Device Command. */
			speed = hc->funcs->port_init(hc, i, ++hc->devices_count);
		else {
			/* Do this so that the device port will respond to address 0. */
			speed = hc->funcs->port_device_get_speed(hc, i);
			hc->funcs->port_reset(hc, i);
		}

		if (speed == -1) {
			hc->root_hub.devices[i] = NULL;
			continue;
		}

		/* Assign address to USB device. */
		device = bl_usb_device_register(hc, speed);
		if (device) {
			bl_usb_hub_link_device(&hc->root_hub, device, i + 1);
			bl_usb_device_register_specific_info(device);
		} else
			hc->root_hub.devices[i] = NULL;
	}

	hc->root_hub.next = bl_root_hubs;
	bl_root_hubs = &hc->root_hub;
}

void bl_usb_poll(void)
{
	int i;
	struct bl_usb_hub *root_hub;

	for (root_hub = bl_root_hubs; root_hub; root_hub = root_hub->next) {
		/* TODO: Check root hub ports first. */

		for (i = 0; i < root_hub->ports; i++)
			if (root_hub->devices[i]->type == BL_USB_HUB_DEVICE)
				bl_usb_hub_poll(root_hub->devices[i]->specific.hub.hub);
	}
}

