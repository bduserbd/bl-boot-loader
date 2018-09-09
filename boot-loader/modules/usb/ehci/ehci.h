#ifndef BL_EHCI_H
#define BL_EHCI_H

#include "include/bl-types.h"

/* EHCI PCI info. */
enum {
	BL_EHCI_PCI_BAR0_TYPE		= (0x3 << 1),
	BL_EHCI_PCI_BAR0_TYPE_32_BIT	= (0x0 << 1),
	BL_EHCI_PCI_BAR0_TYPE_64_BIT	= (0x2 << 1),
	BL_EHCI_PCI_BAR0_BASE_ADDRESS	= (0xffffff << 8),

	BL_EHCI_PCI_SBRN	= 0x20
};

/* EHCI HCIVERSION. */
enum {
	BL_EHCI_CAP_REG_HCI_VERSION	= 0x100,
};

/* EHCI HCSPARAMS. */
enum {
	BL_EHCI_CAP_REG_HCSPARAMS_N_PORTS	= (0xf << 0),
};

/* EHCI capability registers. */
struct bl_ehci_capability_registers {
	__u8	cap_length;
	__u8	reserved;
	__u16	hci_version;
	__u32	hc_sparams;
	__u32	hc_cparams;
	__u64	hcsp_portroute;
} __attribute__((packed));

/* EHCI USBCMD. */
enum {
	BL_EHCI_OP_REG_USBCMD_RUN_STOP		= (1 << 0),
	BL_EHCI_OP_REG_USBCMD_HC_RESET		= (1 << 1),
	BL_EHCI_OP_REG_USBCMD_PERIODIC_ENABLE	= (1 << 4),
	BL_EHCI_OP_REG_USBCMD_ASYNC_ENABLE	= (1 << 5),
};

/* EHCI USBSTS. */
enum {
	BL_EHCI_OP_REG_USBSTS_HC_HALTED		= (1 << 12),
	BL_EHCI_OP_REG_USBSTS_ASYNC_STATUS	= (1 << 15),
};

/* EHCI CONFIGFLAG. */
enum {
	BL_EHCI_OP_REG_CONFIGFLAG_HC_ROUTE	= (1 << 0),
};

/* EHCI PORTCSC. */
enum {
	BL_EHCI_OP_REG_PORTSC_CONNECT_STATUS	= (1 << 0),
	BL_EHCI_OP_REG_PORTSC_ENABLED		= (1 << 2),
	BL_EHCI_OP_REG_PORTSC_ENABLED_CHANGE	= (1 << 3),
	BL_EHCI_OP_REG_PORTSC_PORT_RESET	= (1 << 8),
	BL_EHCI_OP_REG_PORTSC_LINE_STATUS	= (3 << 10),
	BL_EHCI_OP_REG_PORTSC_LINE_STATUS_SE0	= (0 << 10),
	BL_EHCI_OP_REG_PORTSC_LINE_STATUS_J	= (2 << 10),
	BL_EHCI_OP_REG_PORTSC_LINE_STATUS_K	= (1 << 10),
	BL_EHCI_OP_REG_PORTSC_PORT_OWNER	= (1 << 13),
};

/* EHCI operational registers. */
struct bl_ehci_operational_registers {
	__u32	usb_cmd;
	__u32	usb_sts;
	__u32	usb_intr;
	__u32	fr_index;
	__u32	ctrl_ds_segment;
	__u32	periodic_list_base;
	__u32	async_list_addr;
	__u8	reserved[0x40 - 0x1c];
	__u32	config_flag;
	__u32	port_sc[0];
} __attribute__((packed));

/* EHCI frame list link pointer. */
typedef __u32	bl_ehci_frame_list_pointer_t;

enum {
	BL_EHCI_FRAME_LIST_PTR_T	= (1 << 0),
};

/* EHCI queue element transfer descriptor. */
enum {
	BL_EHCI_QTD_LINK_T			= (1 << 0),

	BL_EHCI_QTD_TOKEN_PING_STATE		= (1 << 0),
	BL_EHCI_QTD_TOKEN_SPLITXSTATE		= (1 << 1),
	BL_EHCI_QTD_TOKEN_MISSED_MICRO_FRAME	= (1 << 2),
	BL_EHCI_QTD_TOKEN_XACTERR		= (1 << 3),
	BL_EHCI_QTD_TOKEN_BABBLE_DETECTED	= (1 << 4),
	BL_EHCI_QTD_TOKEN_DATA_BUFFER_ERROR	= (1 << 5),
	BL_EHCI_QTD_TOKEN_HALTED		= (1 << 6),
	BL_EHCI_QTD_TOKEN_ACTIVE		= (1 << 7),
	BL_EHCI_QTD_TOKEN_PID_SETUP		= (2 << 8),
	BL_EHCI_QTD_TOKEN_PID_OUT		= (0 << 8),
	BL_EHCI_QTD_TOKEN_PID_IN		= (1 << 8),
	BL_EHCI_QTD_TOKEN_CERR			= (0x3 << 10),
	BL_EHCI_QTD_TOKEN_C_PAGE		= (0x5 << 12),
	BL_EHCI_QTD_TOKEN_BUFFER_SIZE		= (0x1000),
	BL_EHCI_QTD_TOKEN_MAX_BUFFER_SIZE	= (0x5000),
	BL_EHCI_QTD_TOKEN_TOGGLE		= (1 << 31),
};

#define BL_EHCI_QTD_TOKEN_STATUS(token)	(token & 0xff)

#define BL_EHCI_MAKE_QTD_CERR(cerr)		\
	((cerr << 10) & BL_EHCI_QTD_TOKEN_CERR)

#define BL_EHCI_MAKE_QTD_C_PAGE(page)		\
	((page << 12) & BL_EHCI_QTD_TOKEN_C_PAGE)

#define BL_EHCI_MAKE_QTD_BUFFER_SIZE(size)	(size << 16)

#define BL_EHCI_MAKE_QTD_TOGGLE(toggle)		\
	((toggle << 31) & BL_EHCI_QTD_TOKEN_TOGGLE)

struct bl_ehci_qtd {
	__u32	next_qtd;
	__u32	alternate_next_qtd;
	__u32	token;
	__u32	buffer_pointer[5];
	__u32	extended_buffer_pointer[5];

	/* Software use. */
	int	used;
	__u32	original_next_qtd;

	__u32	padding[1];
} __attribute__((packed));

/* EHCI queue head. */
enum {
	BL_EHCI_QH_LINK_T		= (1 << 0),
	BL_EHCI_QH_LINK_TYPE_QH		= (1 << 1),

	BL_EHCI_QH_CHARS_ADDRESS	= (0x7f << 0),
	BL_EHCI_QH_CHARS_ENDPOINT	= (0xf << 8),
	BL_EHCI_QH_CHARS_FULL_SPEED	= (0 << 12),
	BL_EHCI_QH_CHARS_LOW_SPEED	= (1 << 12),
	BL_EHCI_QH_CHARS_HIGH_SPEED	= (2 << 12),
	BL_EHCI_QH_CHARS_DTC		= (1 << 14),
	BL_EHCI_QH_CHARS_H		= (1 << 15),
	BL_EHCI_QH_CHARS_MAX_PACKET_LEN	= (0x3ff << 16),
	BL_EHCI_QH_CHARS_C		= (1 << 27),
};

#define BL_EHCI_MAKE_QH_ADDRESS(address)	\
	((address << 0) & BL_EHCI_QH_CHARS_ADDRESS)

#define BL_EHCI_MAKE_QH_ENDPOINT(endpoint)	\
	((endpoint << 8) & BL_EHCI_QH_CHARS_ENDPOINT)

#define BL_EHCI_MAKE_QH_MAX_PACKET_LEN(len)	\
	((len << 16) & BL_EHCI_QH_CHARS_MAX_PACKET_LEN)

struct bl_ehci_qh {
	__u32			link;
	__u32			chars;
	__u32			caps;
	__u32			current_qtd;
	struct bl_ehci_qtd	overlay;

	/* Software use. */
	int			used;
	__u32			first_qtd;
	__u32			last_qtd;

	__u32			padding[1];
} __attribute__((packed));

#endif

