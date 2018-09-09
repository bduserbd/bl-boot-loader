#include "usb-keyboard.h"
#include "include/string.h"
#include "include/time.h"
#include "core/include/usb/usb.h"
#include "core/include/keyboard/keyboard.h"
#include "core/include/loader/loader.h"
#include "core/include/memory/heap.h"
#include "core/include/video/print.h"

BL_MODULE_NAME("USB HID Keyboard (Boot protocol)");

static int bl_usb_keyboard_wait_endpoint(struct bl_usb_device *device,
	bl_uint8_t *modifier_key)
{
	bl_status_t status;
	struct bl_usb_endpoint *endp;
	bl_uint8_t report[8];
	bl_usb_transfer_info_t info;

	endp = &device->specific.keyboard.endp;

	info = bl_usb_interrupt_transfer(device, endp, report, 8);

	/* Wait on the endpoint. */
	while ((status = bl_usb_check_transfer_status(device, info)) ==
		BL_STATUS_USB_ACTION_NOT_FINISHED) {
		bl_time_sleep(endp->descriptor->interval);
	}

	*modifier_key = report[0];
	return report[2];
}

static int bl_usb_keyboard_make_char(int key, bl_uint8_t modifier_key)
{
	if ((modifier_key & BL_USB_MODIFIER_KEY_LEFT_SHIFT) ||
		(modifier_key & BL_USB_MODIFIER_KEY_RIGHT_SHIFT))
		return bl_keyboard_layout_lookup_shift(key);
	else
		return bl_keyboard_layout_lookup(key);
}

static int bl_usb_keyboard_getchar(struct bl_keyboard *kbd)
{
	int key;
	struct bl_usb_device *device;
	bl_uint8_t modifier_key;

	device = kbd->data;

	/* Wait for key action. */
	key = bl_usb_keyboard_wait_endpoint(device, &modifier_key);
	if (key)
		return bl_usb_keyboard_make_char(key, modifier_key);
	else
		return -1;
}

static void bl_usb_keyboard_set_boot_protocol(struct bl_usb_device *device,
	struct bl_usb_interface *interface)
{
	/* Set boot protocol. */
	bl_usb_control_transfer(device, NULL,
		BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_INTERFACE |
		BL_USB_SETUP_REQUEST_TYPE_CLASS |
		BL_USB_SETUP_REQUEST_TYPE_HOST_TO_DEVICE,
		BL_USB_HID_REQUEST_TYPE_SET_PROTOCOL, 0,
		interface->descriptor.interface_number, 0);
}

static void bl_usb_keyboard_set_idle_request(struct bl_usb_device *device,
	struct bl_usb_interface *interface)
{
	/* Set idle request. */
	bl_usb_control_transfer(device, NULL,
		BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_INTERFACE |
		BL_USB_SETUP_REQUEST_TYPE_CLASS |
		BL_USB_SETUP_REQUEST_TYPE_HOST_TO_DEVICE,
		BL_USB_HID_REQUEST_TYPE_SET_IDLE, 0,
		interface->descriptor.interface_number, 0);
}

static void bl_usb_keyboard_configure(struct bl_usb_device *device,
	struct bl_usb_interface *interface)
{
	bl_usb_keyboard_set_boot_protocol(device, interface);
	bl_usb_keyboard_set_idle_request(device, interface);
}

static bl_status_t bl_usb_keyboard_initialize(struct bl_keyboard *kbd)
{
	int i;
	struct bl_usb_device *device;
	struct bl_usb_interface *interface;

	device = kbd->data;

	/* Do we support interrupt transfers ? */
	if (!device->controller->funcs->interrupt_transfer ||
		!device->controller->funcs->check_transfer_status)
		return BL_STATUS_UNSUPPORTED;

	interface = &device->interfaces[device->specific.keyboard.interface];

	/* Check device type. */
	if (interface->descriptor.interface_class != 0x3 ||
		interface->descriptor.interface_subclass != 0x1 ||
		interface->descriptor.interface_protocol != 0x1)
		return BL_STATUS_USB_INVALID_DEVICE;

	/* Not localized keyboard. */
	if (device->specific.keyboard.hid.country_code != 0x00)
		return BL_STATUS_USB_INVALID_DEVICE;

	/* Proper endpoint - IN & INTERRUPT. */
	for (i = 0; i < interface->descriptor.num_endpoints; i++)
		if (interface->endpoints[i].endpoint_address & (1 << 7) &&
		    interface->endpoints[i].attributes == 0x3) {
			device->specific.keyboard.endp.toggle = 0;
			device->specific.keyboard.endp.descriptor = &interface->endpoints[i];
			break;
		}

	bl_usb_keyboard_configure(device, interface);

	return BL_STATUS_SUCCESS;
}

static struct bl_keyboard_controller usb_keyboard_controller = {
	.type = BL_KEYBOARD_USB,
	.initialize = bl_usb_keyboard_initialize,
	.getchar = bl_usb_keyboard_getchar,
};

BL_MODULE_INIT()
{
	bl_keyboard_controller_register(&usb_keyboard_controller);
}

BL_MODULE_UNINIT()
{

}

