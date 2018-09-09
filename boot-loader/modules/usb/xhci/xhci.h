#ifndef BL_XHCI_H
#define BL_XHCI_H

#include "include/bl-types.h"
#include "core/include/usb/usb.h"

/* XHCI PCI info. */
enum {
	BL_XHCI_PCI_BAR0_BASE_ADDRESS = (0xffffff << 8),

	BL_XHCI_PCI_SBRN_2_0	= 0x20,
	BL_XHCI_PCI_SBRN_3_0	= 0x30,
	BL_XHCI_PCI_SBRN_3_1	= 0x31,
};

/* XHCI HCSPARAMS1. */
enum {
	BL_XHCI_HCSPARAMS1_MAXSLOTS	= (0xff << 0),
};

#define BL_XHCI_HCSPARAMS1_MAXPORTS(hcsparams1)	((hcsparams1 >> 24) & 0xff)

/* XHCI HCSPARAMS2. */
#define BL_XHCI_HCSPARAMS2_ERSTMAX(hcsparams2)	((hcsparams2 >> 4) & 0xf)

/* XHCI HCCPARAMS1. */
#define BL_XHCI_HCCPARAMS1_XECP(hccparams1)	((hccparams1 >> 16) & 0xffff)

/* XHCI RTSOFF. */
enum {
	BL_XHCI_RTSOFF_RSVD	= ~(0x1f << 0),
};

/* XHCI DBOFF. */
enum {
	BL_XHCI_DBOFF_RSVD	= ~(0x3 << 0),
};

/* XHCI capability registers. */
struct bl_xhci_capability_registers {
	__u8	caplength;
	__u8	reserved;
	__u16	hciversion;
	__u32	hcsparams1;
	__u32	hcsparams2;
	__u32	hcsparams3;
	__u32	hccparams1;
	__u32	dboff;
	__u32	rtsoff;
	__u32	hccparams2;
} __attribute__((packed));

/* XHCI USBCMD. */
enum {
	BL_XHCI_USBCMD_RS	= (1 << 0),
	BL_XHCI_USBCMD_HCRST	= (1 << 1),
};

/* XHCI USBSTS. */
enum {
	BL_XHCI_USBSTS_HCH	= (1 << 0),
	BL_XHCI_USBSTS_CNR	= (1 << 11),
};

/* XHCI CONFIG. */
enum {
	BL_XHCI_CONFIG_MAXSLOTSEN	= (0xff << 0),
};

/* XHCI CRCR. */
enum {
	BL_XHCI_CRCR_RCS	= (1 << 0),
};

/* XHCI operational registers. */
struct bl_xhci_operational_registers {
	__u32	usbcmd;
	__u32	usbsts;
	__u32	pagesize;
	__u8	reserved0[8];
	__u32	dnctrl;
	__u64	crcr;
	__u8	reserved1[16];
	__u64	dcbaap;
	__u32	config;
} __attribute__((packed));

/* XHCI PORTSC. */
enum {
	BL_XHCI_PORTSC_CCS	= (1 << 0),
	BL_XHCI_PORTSC_PED	= (1 << 1),
	BL_XHCI_PORTSC_PR	= (1 << 4),
	BL_XHCI_PORTSC_CSC	= (1 << 17),
	BL_XHCI_PORTSC_PRC	= (1 << 21),
};

enum {
	BL_XHCI_PORTSC_PLS_U0		= 0,
};

#define BL_XHCI_PORTSC_PLS(portsc)		((portsc >> 5) & 0xf)
#define BL_XHCI_PORTSC_PORT_SPEED(portsc)	((portsc >> 10) & 0xf)

/* XHCI port register set. */
struct bl_xhci_port_register_set {
	__u32	portsc;
	__u32	portpmsc;
	__u32	portli;
	__u32	porthlpmc;
} __attribute__((packed));

/* XHCI USB speed ID mapping. */
enum {
	BL_XHCI_USB_FULL_SPEED		= 1,	/* 12 Mb/s, USB 2.0. */
	BL_XHCI_USB_LOW_SPEED		= 2,	/* 1.5 Mb/s, USB 2.0. */
	BL_XHCI_USB_HIGH_SPEED		= 3,	/* 480 Mb/s, USB 2.0. */
	BL_XHCI_USB_SUPER_SPEED		= 4,	/* 5 Gb/s, USB 3.x. */
	BL_XHCI_USB_SUPER_SPEED_PLUS	= 5,	/* 10 Gb/s, USB 3.1. */
};

/* XHCI Transfer Ring TRB types. */
enum {
	BL_XHCI_TR_TRB_NORMAL		= 1,
	BL_XHCI_TR_TRB_SETUP		= 2,
	BL_XHCI_TR_TRB_DATA		= 3,
	BL_XHCI_TR_TRB_STATUS		= 4,
	BL_XHCI_TR_TRB_ISOCH		= 5,
	BL_XHCI_TR_TRB_LINK		= 6,
	BL_XHCI_TR_TRB_EVENT_DATA	= 7,
	BL_XHCI_TR_TRB_NO_OP		= 8,
};

/* XHCI Command Ring TRB types. */
enum {
	BL_XHCI_CR_TRB_LINK				= 6,
	BL_XHCI_CR_TRB_ENABLE_SLOT			= 9,
	BL_XHCI_CR_TRB_DISABLE_SLOT			= 10,
	BL_XHCI_CR_TRB_ADDRESS_DEVICE			= 11,
	BL_XHCI_CR_TRB_CONFIGURE_ENDPOINT		= 12,
	BL_XHCI_CR_TRB_EVALUATE_CONTEXT			= 13,
	BL_XHCI_CR_TRB_RESET_ENDPOINT			= 14,
	BL_XHCI_CR_TRB_STOP_ENDPOINT			= 15,
	BL_XHCI_CR_TRB_SET_TR_DEQUEUE_POINTER		= 16,
	BL_XHCI_CR_TRB_RESET_DEVICE			= 17,
	BL_XHCI_CR_TRB_FORCE_EVENT			= 18,
	BL_XHCI_CR_TRB_NEGOTIATE_BANDWIDTH		= 19,
	BL_XHCI_CR_TRB_SET_LATENCY_TOLERANCE_VALUE	= 20,
	BL_XHCI_CR_TRB_GET_PORT_BANDWIDTH		= 21,
	BL_XHCI_CR_TRB_FORCE_HEADER			= 22,
	BL_XHCI_CR_TRB_NO_OP				= 23,
};

/* XHCI Event Ring TRB types. */
enum {
	BL_XHCI_ER_TRB_TRANSFER			= 32,
	BL_XHCI_ER_TRB_COMMAND_COMPLETION	= 33,
	BL_XHCI_ER_TRB_PORT_STATUS_CHANGE	= 34,
	BL_XHCI_ER_TRB_BANDWIDTH_REQUEST	= 35,
	BL_XHCI_ER_TRB_DOORBELL			= 36,
	BL_XHCI_ER_TRB_HOST_CONTROLLER		= 37,
	BL_XHCI_ER_TRB_DEVICE_NOTIFICATION	= 38,
	BL_XHCI_ER_TRB_MFINDEX_WRAP		= 39,
};

/* XHCI TRB completion codes. */
enum {
	BL_XHCI_TRB_CC_INVALID				= 0,
	BL_XHCI_TRB_CC_SUCCESS				= 1,
	BL_XHCI_TRB_CC_DATA_BUFFER_ERROR		= 2,
	BL_XHCI_TRB_CC_BABBLE_DETECTED_ERROR		= 3,
	BL_XHCI_TRB_CC_USB_TRANSACTION_ERROR		= 4,
	BL_XHCI_TRB_CC_TRB_ERROR			= 5,
	BL_XHCI_TRB_CC_STALL_ERROR			= 6,
	BL_XHCI_TRB_CC_RESOURCE_ERROR			= 7,
	BL_XHCI_TRB_CC_BANDWIDTH_ERROR			= 8,
	BL_XHCI_TRB_CC_NO_SLOTS_AVAILABLE_ERROR		= 9,
	BL_XHCI_TRB_CC_INVALID_STREAM_TYPE_ERROR	= 10,
	BL_XHCI_TRB_CC_SLOT_NOT_ENABLED_ERROR		= 11,
	BL_XHCI_TRB_CC_ENDPOINT_NOT_ENABLED		= 12,
	BL_XHCI_TRB_CC_SHORT_ERROR			= 13,
	BL_XHCI_TRB_CC_RING_UNDERRUN			= 14,
	BL_XHCI_TRB_CC_RING_OVERRUN			= 15,
	BL_XHCI_TRB_CC_VF_EVENT_RING_FULL_ERROR		= 16,
	BL_XHCI_TRB_CC_PARAMETER_INFO			= 17,
	BL_XHCI_TRB_CC_BANDWIDTH_OVERRUN_ERROR		= 18,
	BL_XHCI_TRB_CC_CONTEXT_STATE_ERROR		= 19,
	BL_XHCI_TRB_CC_NO_PING_RESPONSE_ERROR		= 20,
	BL_XHCI_TRB_CC_EVENT_RING_FULL_ERROR		= 21,
	BL_XHCI_TRB_CC_INCOMPATILBE_DEVICE_ERROR	= 22,
	BL_XHCI_TRB_CC_MISSED_SERVICE_ERROR		= 23,
	BL_XHCI_TRB_CC_COMMAND_RING_STOPPED		= 24,
	BL_XHCI_TRB_CC_COMMAND_ABORTED			= 25,
	BL_XHCI_TRB_CC_STOPPED				= 26,
	BL_XHCI_TRB_CC_STOPPED_LENGTH_INVALID		= 27,
	BL_XHCI_TRB_CC_STOPPED_SHORT_PACKET		= 28,
	BL_XHCI_TRB_CC_MAX_EXIT_LATENCY_TOO_LARGE_ERROR	= 29,
	BL_XHCI_TRB_CC_ISOCH_BUFFER_OVERRUN		= 31,
	BL_XHCI_TRB_CC_EVENT_LOST_ERROR			= 32,
	BL_XHCI_TRB_CC_UNDEFINED_ERROR			= 33,
	BL_XHCI_TRB_CC_INVALID_STREAM_ID_ERROR		= 34,
	BL_XHCI_TRB_CC_SECONDARY_BANDWIDTH_ERROR	= 35,
	BL_XHCI_TRB_CC_SPLIT_TRANSACTION_ERROR		= 36,
};

/* XHCI template TRB status flags. */
#define BL_XHCI_TRB_CC(status)		((status >> 24) & 0xff)

/* XHCI template TRB control flags. */
enum {
	BL_XHCI_TRB_C		= (1 << 0),
};

#define BL_XHCI_TRB_TYPE(type)		((type & 0x3f) << 10)
#define BL_XHCI_TRB_GET_TYPE(control)	((control >> 10) & 0x3f)

#define BL_XHCI_TRB_SLOT_ID(control)		((control >> 24) & 0xff)
#define BL_XHCI_TRB_SET_SLOT_ID(slot_id)	((slot_id & 0xff) << 24)

/* XHCI TRB template. */
struct bl_xhci_template_trb {
	__u64	parameter;
	__u32	status;
	__u32	control;
} __attribute__((packed));

/* XHCI Link TRB. */
struct bl_xhci_link_trb {
	__u64	ring_segment_pointer;
	__u32	reserved;
	__u32	control;
} __attribute__((packed));

/* XHCI Normal TRB. */
struct bl_xhci_normal_trb {
	__u64	data_buffer_pointer;
	__u32	transfer_length : 17;
	__u32	td_size : 5;
	__u32	interrupter_target : 10;
	__u32	c : 1;
	__u32	ent : 1;
	__u32	isp : 1;
	__u32	ns : 1;
	__u32	ch : 1;
	__u32	ioc : 1;
	__u32	idt : 1;
	__u32	reserved0 : 2;
	__u32	bei : 1;
	__u32	trb_type : 6;
	__u32	reserved1 : 16;
} __attribute__((packed));

/* XHCI Setup Stage TRB. */
enum {
	BL_XHCI_CONTROL_TRT_NO_DATA_STAGE	= 0,
	BL_XHCI_CONTROL_TRT_RESERVED		= 1,
	BL_XHCI_CONTROL_TRT_OUT_DATA_STAGE	= 2,
	BL_XHCI_CONTROL_TRT_IN_DATA_STAGE	= 3,
};

struct bl_xhci_setup_stage_trb {
	struct bl_usb_setup_data data;
	__u32	transfer_length : 17;
	__u32	reserved0 : 5;
	__u32	interrupter_target : 10;
	__u32	c : 1;
	__u32	reserved1 : 4;
	__u32	ioc : 1;
	__u32	idt : 1;
	__u32	reserved2 : 3;
	__u32	trb_type : 6;
	__u32	trt : 2;
	__u32	reserved3 : 14;
} __attribute__((packed));

/* XHCI Data Stage TRB. */
struct bl_xhci_data_stage_trb {
	__u64	data_buffer;
	__u32	transfer_length : 17;
	__u32	td_size : 5;
	__u32	interrupter_target : 10;
	__u32	c : 1;
	__u32	ent : 1;
	__u32	isp : 1;
	__u32	ns : 1;
	__u32	ch : 1;
	__u32	ioc : 1;
	__u32	idt : 1;
	__u32	reserved0 : 3;
	__u32	trb_type : 6;
	__u32	dir : 1;
	__u32	reserved1 : 15;
} __attribute__((packed));

/* XHCI Status Stage TRB. */
struct bl_xhci_status_stage_trb {
	__u64	reserved0;
	__u32	reserved1 : 22;
	__u32	interrupter_target : 10;
	__u32	c : 1;
	__u32	ent : 1;
	__u32	reserved2 : 2;
	__u32	ch : 1;
	__u32	ioc : 1;
	__u32	reserved3 : 4;
	__u32	trb_type : 6;
	__u32	dir : 1;
	__u32	reserved4 : 15;
} __attribute__((packed));

/* XHCI Address Device/Configure Endpoint Command TRB. */
enum {
	BL_XHCI_TRB_INPUT_CONTEXT_PTR_RSVD	= ~(0xf << 0),
};

enum {
	BL_XHCI_TRB_BSR	= (1 << 9),
};

enum {
	BL_XHCI_TRB_DC	= (1 << 9),		
};

/* XHCI Port Status Change Event. */
#define BL_XHCI_TRB_PORT_ID(parameter)	((parameter >> 24) & 0xff)

/* XHCI mixed TRB types. */
union bl_xhci_trb {
	struct bl_xhci_template_trb	template;
	struct bl_xhci_link_trb		link;
	struct bl_xhci_normal_trb	normal;
	struct bl_xhci_setup_stage_trb	setup;
	struct bl_xhci_data_stage_trb	data;
	struct bl_xhci_status_stage_trb	status;
};

/* XHCI Event Ring Segment Table. */
struct bl_xhci_erst {
	__u64	ring_segment_address;
	__u16	ring_segment_size;
	__u8	reserved[6];
} __attribute__((packed));

/* XHCI Interrupter Register Set. */
struct bl_xhci_interrupter_register_set {
	__u32	iman;
	__u32	imod;
	__u32	erstsz;
	__u32	reserved;
	__u64	erstba;
	__u64	erdp;
} __attribute__((packed));

/* XHCI Runtime Registers. */
struct bl_xhci_runtime_registers {
	__u32	mfindex;
	__u8	reserved[28];
	struct bl_xhci_interrupter_register_set interrupters[1024];
} __attribute__((packed));

/* XHCI Extended Capability Pointer Register. */
struct bl_xhci_ecpr {
	__u8	cap_id;
	__u8	next;
	__u8	cap_specific[0];
} __attribute__((packed));

/* XHCI Extended Capability Codes. */
enum {
	BL_XHCI_EXTENDED_CAPABILITY_USB_LEGACY_SUPPORT		= 1,
	BL_XHCI_EXTENDED_CAPABILITY_SUPPORTED_PROTOCOL		= 2,
	BL_XHCI_EXTENDED_CAPABILITY_EXTENDED_POWER_MANAGEMENT	= 3,
	BL_XHCI_EXTENDED_CAPABILITY_IO_VIRTUALIZATION		= 4,
	BL_XHCI_EXTENDED_CAPABILITY_MESSAGE_INTERRUPT		= 5,
	BL_XHCI_EXTENDED_CAPABILITY_LOCAL_MEMORY		= 6,
	BL_XHCI_EXTENDED_CAPABILITY_USB_DEBUG			= 10,
	BL_XHCI_EXTENDED_CAPABILITY_EXTENDED_MESSAGE_INTERRUPT	= 17,
};

/* XHCI Supported Protocol Capability. */
struct bl_xhci_supported_protocol {
	__u8	revision_minor;
	__u8	revision_major;
	__u32	name_string;
	__u8	compatible_port_offset;
	__u8	compatible_port_count;
	__u16	protocol_defined : 12;
	__u16	psic : 4;
	__u32	protocol_slot_type : 5;
	__u32	reserved : 27;
} __attribute__((packed));

/* XHCI Slot Context. */
struct bl_xhci_slot_context {
	__u32	route_string : 20;
	__u32	speed : 4;
	__u32	reserved0 : 1;
	__u32	mtt : 1;
	__u32	hub : 1;
	__u32	context_entries : 5;
	__u16	max_exit_latency;
	__u8	root_hub_port_number;
	__u8	number_of_ports;
	__u8	tt_hub_slot_id;
	__u8	tt_port_number;
	__u16	ttt : 2;
	__u16	reserved1 : 4;
	__u16	interrupter_target : 10;
	__u32	usb_device_address : 8;
	__u32	reserved2 : 19;
	__u32	slot_state : 5;
	__u32	reserved3[4];
} __attribute__((packed));

/* XHCI Endpoint Context. */
enum {
	BL_XHCI_EP_STATE_DISABLED	= 0,
	BL_XHCI_EP_STATE_RUNNING	= 1,
	BL_XHCI_EP_STATE_HALTED		= 2,
	BL_XHCI_EP_STATE_STOPPED	= 3,
	BL_XHCI_EP_STATE_ERROR		= 4,
};

enum {
	BL_XHCI_EP_TYPE_NOT_VALID	= 0,
	BL_XHCI_EP_TYPE_ISOCH_OUT	= 1,
	BL_XHCI_EP_TYPE_BULK_OUT	= 2,
	BL_XHCI_EP_TYPE_INTERRUPT_OUT	= 3,
	BL_XHCI_EP_TYPE_CONTROL		= 4,
	BL_XHCI_EP_TYPE_ISOCH_IN	= 5,
	BL_XHCI_EP_TYPE_BULK_IN		= 6,
	BL_XHCI_EP_TYPE_INTERRUPT_IN	= 7,
};

struct bl_xhci_endpoint_context {
	__u8	ep_state : 3;
	__u8	reserved0 : 5;
	__u8	mult : 2;
	__u8	max_pstreams : 5;
	__u8	lsa : 1;
	__u8	interval;
	__u8	max_esit_payload_hi;
	__u8	reserved1 : 1;
	__u8	cerr : 2;
	__u8	ep_type : 3;
	__u8	reserved2 : 1;
	__u8	hid : 1;
	__u8	max_burst_size;
	__u16	max_packet_size;
	__u64	tr_dequeue_pointer;
	__u16	average_trb_length;
	__u16	max_esit_payload_lo;
	__u32	reserved3[3];
} __attribute__((packed));

/* XHCI Device Context. */
struct bl_xhci_device_context {
	struct bl_xhci_slot_context		slot;
	struct bl_xhci_endpoint_context		ep_context0;
	struct bl_xhci_endpoint_context		ep_context[30];
} __attribute__((packed));

/* XHCI Input Control Context. */
#define BL_XHCI_INPUT_CONTROL_CONTEXT_A(n)	(1 << n)

struct bl_xhci_input_control_context {
	__u32	drop_context_flags;
	__u32	add_context_flags;
	__u32	reserved0[5];
	__u8	configuration_value;
	__u8	interface_number;
	__u8	alternate_setting;
	__u8	reserved1;
} __attribute__((packed));

/* XHCI Input Context. */
struct bl_xhci_input_context {
	struct bl_xhci_input_control_context	icc;
	struct bl_xhci_slot_context		slot;
	struct bl_xhci_endpoint_context		ep_context0;
	struct bl_xhci_endpoint_context		ep_context[30];
} __attribute__((packed));

#endif

