#ifndef BL_USB_H
#define BL_USB_H

#include "include/bl-types.h"
#include "core/include/storage/storage.h"
#include "core/include/keyboard/keyboard.h"

/* Actual USB data. */

/* USB packet IDs. */
typedef enum {
	BL_USB_PID_TOKEN_OUT	= 0x1,
	BL_USB_PID_TOKEN_IN	= 0x9,
	BL_USB_PID_TOKEN_SETUP	= 0xd,
} bl_usb_pid_t;

static inline bl_uint8_t bl_usb_pid_format_check(bl_uint8_t pid)
{
	return (pid & 0xf) | (~pid << 4);
}

/* USB control request type. */
enum {
	BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_DEVICE	= (0 << 0),
	BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_INTERFACE	= (1 << 0),
	BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_ENDPOINT	= (2 << 0),
	BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_OTHER	= (3 << 0),

	BL_USB_SETUP_REQUEST_TYPE_STANDARD		= (0 << 5),
	BL_USB_SETUP_REQUEST_TYPE_CLASS			= (1 << 5),
	BL_USB_SETUP_REQUEST_TYPE_VENDOR		= (2 << 5),
	BL_USB_SETUP_REQUEST_TYPE_RESERVED		= (3 << 5),

	BL_USB_SETUP_REQUEST_TYPE_HOST_TO_DEVICE	= (0 << 7),
	BL_USB_SETUP_REQUEST_TYPE_DEVICE_TO_HOST	= (1 << 7),
};

/* USB standard information requests. */
enum {
	BL_USB_REQUEST_TYPE_GET_STATUS		= 0x00,
	BL_USB_REQUEST_TYPE_CLEAR_FEATURE	= 0x01,
	BL_USB_REQUEST_TYPE_SET_FEATURE		= 0x03,
	BL_USB_REQUEST_TYPE_SET_ADDRESS		= 0x05,
	BL_USB_REQUEST_TYPE_GET_DESCRIPTOR	= 0x06,
	BL_USB_REQUEST_TYPE_SET_DESCRIPTOR	= 0x07,
	BL_USB_REQUEST_TYPE_GET_CONFIGURATION	= 0x08,
	BL_USB_REQUEST_TYPE_SET_CONFIGURATION	= 0x09,
	BL_USB_REQUEST_TYPE_GET_INTERFACE	= 0x0a,
	BL_USB_REQUEST_TYPE_SET_INTERFACE	= 0x0b,
	BL_USB_REQUEST_TYPE_SYNC_FRAME		= 0x0c,
};

/* USB HID requests. */
enum {
	BL_USB_HID_REQUEST_TYPE_GET_REPORT	= 0x01,
	BL_USB_HID_REQUEST_TYPE_GET_IDLE	= 0x02,
	BL_USB_HID_REQUEST_TYPE_GET_PROTOCOL	= 0x03,
	BL_USB_HID_REQUEST_TYPE_SET_REPORT	= 0x09,
	BL_USB_HID_REQUEST_TYPE_SET_IDLE	= 0x0a,
	BL_USB_HID_REQUEST_TYPE_SET_PROTOCOL	= 0x0b,
};

/* USB hub class requests. */
enum {
	BL_USB_HUB_REQUEST_TYPE_GET_STATUS	= 0x00,
	BL_USB_HUB_REQUEST_TYPE_CLEAR_FEATURE	= 0x01,
	BL_USB_HUB_REQUEST_TYPE_RESERVED	= 0x02,
	BL_USB_HUB_REQUEST_TYPE_SET_FEATURE	= 0x03,
	BL_USB_HUB_REQUEST_TYPE_GET_DESCRIPTOR	= 0x06,
	BL_USB_HUB_REQUEST_TYPE_SET_DESCRIPTOR	= 0x07,
	BL_USB_HUB_REQUEST_TYPE_CLEAR_TT_BUFFER	= 0x08,
	BL_USB_HUB_REQUEST_TYPE_RESET_TT	= 0x09,
	BL_USB_HUB_REQUEST_TYPE_GET_TT_STATE	= 0x0a,
	BL_USB_HUB_REQUEST_TYPE_STOP_TT		= 0x0b,
};

/* USB standard descriptor types. */
enum {
	BL_USB_DESCRIPTOR_TYPE_DEVICE					= 0x01,
	BL_USB_DESCRIPTOR_TYPE_CONFIG,
	BL_USB_DESCRIPTOR_TYPE_STRING,
	BL_USB_DESCRIPTOR_TYPE_INTERFACE,
	BL_USB_DESCRIPTOR_TYPE_ENDPOINT,
	BL_USB_DESCRIPTOR_TYPE_HID					= 0x21,
	BL_USB_DESCRIPTOR_TYPE_HUB					= 0x29,
	BL_USB_DESCRIPTOR_TYPE_SUPERSPEED_USB_ENDPOINT_COMPANION	= 0x30,
};

/* USB hub port status. */
enum {
	BL_USB_HUB_PORT_STATUS_CONNECT_STATUS		= (1 << 0),
	BL_USB_HUB_PORT_STATUS_ENABLED			= (1 << 1),
	BL_USB_HUB_PORT_STATUS_SUSPEND			= (1 << 2),
	BL_USB_HUB_PORT_STATUS_OVER_CURRENT		= (1 << 3),
	BL_USB_HUB_PORT_STATUS_RESET			= (1 << 4),
	BL_USB_HUB_PORT_STATUS_PORT_POWER		= (1 << 8),
	BL_USB_HUB_PORT_STATUS_LOW_SPEED_DEVICE		= (1 << 9),
	BL_USB_HUB_PORT_STATUS_HIGH_SPEED_DEVICE	= (1 << 10),
	BL_USB_HUB_PORT_STATUS_TEST_MODE		= (1 << 11),
	BL_USB_HUB_PORT_STATUS_INDICATOR_CONTROL	= (1 << 12),
};

/* USB hub port status change. */
enum {
	BL_USB_HUB_PORT_CHANGE_CONNECT_STATUS	= (1 << 0),
	BL_USB_HUB_PORT_CHANGE_ENABLED		= (1 << 1),
	BL_USB_HUB_PORT_CHANGE_SUSPEND		= (1 << 2),
	BL_USB_HUB_PORT_CHANGE_OVER_CURRENT	= (1 << 3),
	BL_USB_HUB_PORT_CHANGE_RESET		= (1 << 4),
};

/* USB hub class feature selectors. */
enum {
	BL_USB_HUB_FEATURE_C_HUB_LOCAL_POWER	= 0x00,
	BL_USB_HUB_FEATURE_C_HUB_OVER_CURRENT	= 0x01,

	BL_USB_HUB_FEATURE_PORT_CONNECTION	= 0x00,
	BL_USB_HUB_FEATURE_PORT_ENABLE		= 0x01,
	BL_USB_HUB_FEATURE_PORT_SUSPEND		= 0x02,
	BL_USB_HUB_FEATURE_PORT_OVER_CURRENT	= 0x03,
	BL_USB_HUB_FEATURE_PORT_RESET		= 0x04,
	BL_USB_HUB_FEATURE_PORT_POWER		= 0x08,
	BL_USB_HUB_FEATURE_PORT_LOW_SPEED	= 0x09,

	BL_USB_HUB_FEATURE_C_PORT_CONNECTION	= 0x10,
	BL_USB_HUB_FEATURE_C_PORT_ENABLE	= 0x11,
	BL_USB_HUB_FEATURE_C_PORT_SUSPEND	= 0x12,
	BL_USB_HUB_FEATURE_C_PORT_OVER_CURRENT	= 0x13,
	BL_USB_HUB_FEATURE_C_PORT_RESET		= 0x14,

	BL_USB_HUB_FEATURE_PORT_TEST		= 0x15,
	BL_USB_HUB_FEATURE_PORT_INDICATOR	= 0x16,
};

/* USB core control info. */
struct bl_usb_setup_data {
	__u8	request_type;
	__u8	request;
	__u16	value;
	__u16	index;
	__u16	length;
} __attribute__((packed));

static inline bl_usb_pid_t bl_usb_direction_pid(struct bl_usb_setup_data *setup)
{
	if (setup->request_type & BL_USB_SETUP_REQUEST_TYPE_DEVICE_TO_HOST)
		return BL_USB_PID_TOKEN_IN;
	else
		return BL_USB_PID_TOKEN_OUT;
}

static inline bl_usb_pid_t bl_usb_status_pid(bl_usb_pid_t data_pid)
{
	if (data_pid == BL_USB_PID_TOKEN_OUT)
		return BL_USB_PID_TOKEN_IN;
	else /* data_pid == BL_USB_PID_TOKEN_IN */
		return BL_USB_PID_TOKEN_OUT;
}

/* USB device descriptor. */
struct bl_usb_device_descriptor {
	__u8	length;
	__u8	descriptor_type;
	__u16	release_number;
	__u8	device_class;
	__u8	device_subclass;
	__u8	device_protocol;
	__u8	max_packet_size;
	__u16	vendor_id;
	__u16	product_id;
	__u16	device;
	__u8	manufacturer_str;
	__u8	product_str;
	__u8	serial_number_str;
	__u8	configurations_count;
} __attribute__((packed));

/* USB configuration descriptor. */
struct bl_usb_configuration_descriptor {
	__u8	length;
	__u8	descriptor_type;
	__u16	total_length;
	__u8	interfaces_count;
	__u8	configuration_value;
	__u8	configuration_str;
	__u8	attributes;
	__u8	max_power;
} __attribute__((packed));

/* USB interface descriptor. */
struct bl_usb_interface_descriptor {
	__u8	length;
	__u8	descriptor_type;
	__u8	interface_number;
	__u8	alternate_setting;
	__u8	num_endpoints;
	__u8	interface_class;
	__u8	interface_subclass;
	__u8	interface_protocol;
	__u8	interface_str;
} __attribute__((packed));

/* USB endpoint descriptor. */
#define BL_USB_ENDPOINT_ADDRESS(endpoint_address)	(endpoint_address & 0xf)

// 1 - IN, 0 - OUT.
#define BL_USB_ENDPOINT_DIRECTION(endpoint_address)	((endpoint_address >> 7) & 0x1)

enum {
	BL_USB_ENDPOINT_TYPE_CONTROL		= 0,
	BL_USB_ENDPOINT_TYPE_ISOCHRONOUS	= 1,
	BL_USB_ENDPOINT_TYPE_BULK		= 2,
	BL_USB_ENDPOINT_TYPE_INTERRUPT		= 3,
};

struct bl_usb_endpoint_descriptor {
	__u8	length;
	__u8	descriptor_type;
	__u8	endpoint_address;
	__u8	attributes;
	__u16	max_packet_size;
	__u8	interval;
} __attribute__((packed));

/* USB string descriptor. */
struct bl_usb_string_descriptor {
	__u8	length;
	__u8	descriptor_type;
	__u16	code[0];
} __attribute__((packed));

/* USB hub descriptor. */
struct bl_usb_hub_descriptor {
	__u8	length;
	__u8	descriptor_type;
	__u8	ports;
	__u16	characteristics;
	__u8	power_on_2_power_good;
	__u8	hub_control_current;
	__u8	device_removable[0];
} __attribute__((packed));

/* USB hub status & change. */
struct bl_usb_hub_status {
	__u16	hub_status;
	__u16	hub_change;
} __attribute__((packed));

/* USB hub port & change status. */
struct bl_usb_hub_port_status {
	__u16	port_status;
	__u16	port_change;
} __attribute__((packed));

/* USB HID class descriptor. */
struct bl_usb_hid_class_descriptor {
	__u8	descriptor_type;
	__u16	descriptor_length;
} __attribute__((packed));

/* USB HID descriptor. */
struct bl_usb_hid_descriptor {
	__u8	length;
	__u8	descriptor_type;
	__u16	hid;
	__u8	country_code;
	__u8	num_descriptors;

	struct bl_usb_hid_class_descriptor report;
	struct bl_usb_hid_class_descriptor optional[0];
} __attribute__((packed));

/* USB driver interface. */
typedef enum {
	BL_USB_LOW_SPEED,		/* Up to 1.5 Mb/s. */
	BL_USB_FULL_SPEED,		/* Up to 12 Mb/s. */
	BL_USB_HIGH_SPEED,		/* Up to 480 Mb/s. */
	BL_USB_SUPER_SPEED,		/* Up to 5 Gb/s. */
	BL_USB_SUPER_SPEED_PLUS,	/* Up to 10 Gb/s. */
} bl_usb_speed_t;

typedef enum {
	BL_USB_TRANSFER_CONTROL,
	BL_USB_TRANSFER_BULK,
	BL_USB_TRANSFER_INTERRUPT,
	BL_USB_TRANSFER_ISOCHRONOUS,
} bl_usb_transfer_t;

typedef void *bl_usb_transfer_info_t;

struct bl_usb_host_controller;
struct bl_usb_hub;
struct bl_usb_endpoint;

struct bl_usb_host_controller_functions {
	int (*port_device_connected)(struct bl_usb_host_controller *, int);

	union {
		/* UHCI, OHCI, EHCI. */
		struct {
			bl_usb_speed_t (*port_device_get_speed)(struct bl_usb_host_controller *, int);

			void (*port_reset)(struct bl_usb_host_controller *, int);
		};

		/* XHCI. */
		struct {
			bl_usb_speed_t (*port_init)(struct bl_usb_host_controller *, int, int);
		};
	};

	bl_status_t (*control_transfer)(struct bl_usb_host_controller *,
		int, int, bl_usb_speed_t,
		struct bl_usb_setup_data *, bl_uint8_t *, int);

	void (*bulk_transfer)(struct bl_usb_host_controller *,
		int, struct bl_usb_endpoint *, bl_usb_speed_t,
		int, bl_uint8_t *, int);

	bl_usb_transfer_info_t (*interrupt_transfer)(struct bl_usb_host_controller *,
		int, struct bl_usb_endpoint *, bl_usb_speed_t,
		int, bl_uint8_t *, int);

	bl_status_t (*check_transfer_status)(bl_usb_transfer_info_t);
};

/* Related to host controller work. */
struct bl_usb_endpoint {
	int toggle;
	struct bl_usb_endpoint_descriptor *descriptor;
};

struct bl_usb_interface {
	struct bl_usb_interface_descriptor descriptor;
	struct bl_usb_endpoint_descriptor *endpoints;
};

typedef enum {
	BL_USB_HUB_DEVICE = 1,
	BL_USB_STORAGE_DEVICE,
	BL_USB_KEYBOARD_DEVICE,
} bl_usb_device_t;

typedef enum {
	BL_USB_STORAGE_BBB,
} bl_usb_storage_t;

struct bl_usb_device {
	int address;
	bl_usb_speed_t speed;

	int port;
	struct bl_usb_hub *parent_hub;

	struct bl_usb_device_descriptor dev_desc;

	/* Support only for one configuration .. */
	struct bl_usb_configuration_descriptor config;
	struct bl_usb_interface *interfaces;

	struct bl_usb_host_controller *controller;

	/* Different USB devices. */
	bl_usb_device_t type;

	union {
		/* USB hub. */
		struct {
			struct bl_usb_hub *hub;
		} hub;

		/* Mass storage - SCSI based. */
		struct {
			bl_usb_storage_t type;
			int luns;

			int interface;
			struct bl_usb_endpoint in_endp, out_endp;
		} storage;

		/* HID keyboard. */
		struct {
			int interface;
			struct bl_usb_hid_descriptor hid;
			struct bl_usb_endpoint endp;

			bl_usb_transfer_info_t key_interrupt;
			struct bl_keyboard *device;
		} keyboard;
	} specific;
};

struct bl_usb_hub {
	int ports;
	struct bl_usb_device **devices;

	bl_int8_t port_chain[5];

	struct bl_usb_device *device; // Except for root hub.

	struct bl_usb_hub *next;
};

typedef enum {
	BL_USB_HC_UHCI,
	BL_USB_HC_OHCI,
	BL_USB_HC_EHCI,
	BL_USB_HC_XHCI,
} bl_usb_host_controller_t;

struct bl_usb_host_controller {
	bl_usb_host_controller_t type;

	int bus;
	int devices_count; // Assign addresses in increasing order.

	struct bl_usb_host_controller_functions *funcs;
	struct bl_usb_hub root_hub;
	void *data;

	struct bl_usb_host_controller *next;
};

struct bl_usb_traversal {
	int head, tail;
	struct bl_usb_hub *hubs[127];
};

/* USB releated functions. */
void bl_usb_poll(void);

void bl_usb_dump(void);

/* Standard descriptors. */
void bl_usb_get_descriptor(struct bl_usb_device *, int, int, int, int, void *, int);

void bl_usb_control_transfer(struct bl_usb_device *, void *,
	int, int, int, int, int);

void bl_usb_bulk_transfer(struct bl_usb_device *, int, void *, int);

bl_usb_transfer_info_t bl_usb_interrupt_transfer(struct bl_usb_device *,
	struct bl_usb_endpoint *, void *, int);

bl_status_t bl_usb_check_transfer_status(struct bl_usb_device *,
	bl_usb_transfer_info_t);

/* After its port was reseted and it responds to device address 0. */
struct bl_usb_device *bl_usb_device_register(struct bl_usb_host_controller *,
	bl_usb_speed_t);

/* The device should have a valid address. */
void bl_usb_device_register_specific_info(struct bl_usb_device *);

/* USB hub functions. */
void bl_usb_hub_root_register(struct bl_usb_host_controller *);
void bl_usb_hub_register(struct bl_usb_device *);

void bl_usb_hub_get_hub_descriptor(struct bl_usb_device *,
	struct bl_usb_hub_descriptor *);

void bl_usb_hub_get_port_status(struct bl_usb_device *, int,
	struct bl_usb_hub_port_status *);

void bl_usb_hub_clear_port_feature(struct bl_usb_device *, int, int);

void bl_usb_hub_set_port_feature(struct bl_usb_device *, int, int);

/* USB host controller functions. */
void bl_usb_setup(void);

void bl_usb_host_controller_register(struct bl_usb_host_controller *);
void bl_usb_host_controller_unregister(struct bl_usb_host_controller *);

/* USB traversal functions. */
int bl_usb_traversal_is_empty(struct bl_usb_traversal *);

void bl_usb_traversal_push(struct bl_usb_traversal *, struct bl_usb_hub *);

struct bl_usb_hub *bl_usb_traversal_pop(struct bl_usb_traversal *);

struct bl_usb_traversal *bl_usb_traversal_init(void);

void bl_usb_traversal_uninit(struct bl_usb_traversal *);

#endif

