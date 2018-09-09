#ifndef BL_UHCI_H
#define BL_UHCI_H

#include "include/bl-types.h"

/* UHCI PCI BAR4 register info. */
enum {
	BL_UHCI_PCI_BAR4_RTE		= 0x01,
	BL_UHCI_PCI_BAR4_BASE_ADDRESS	= 0xffe0,
};

/* UHCI host controller I/O registers. */
typedef enum {
	BL_UHCI_IO_REG_USBCMD		= 0x00,
	BL_UHCI_IO_REG_USBSTS		= 0x02,
	BL_UHCI_IO_REG_USBINTR		= 0x04,
	BL_UHCI_IO_REG_FRNUM		= 0x06,
	BL_UHCI_IO_REG_FRBASEADD	= 0x08,
	BL_UHCI_IO_REG_SOFMOD		= 0x0c,
	BL_UHCI_IO_REG_PORTSC1		= 0x10,
	BL_UHCI_IO_REG_PORTSC2		= 0x12,
} bl_uhci_reg_t;

/* Relevant UHCI USBCMD register values. */
enum {
	BL_UHCI_USBCMD_STOP		= (0 << 0),
	BL_UHCI_USBCMD_RUN		= (1 << 0),
	BL_UHCI_USBCMD_HCRESET		= (1 << 1),
};

/* Port status flags. */
enum {
	BL_UHCI_PORTSC_CURRENT_CONNECT_STATUS	= (1 << 0),
	BL_UHCI_PORTSC_PORT_ENABLED		= (1 << 2),
	BL_UHCI_PORTSC_LOW_SPEED_DEVICE		= (1 << 8),
	BL_UHCI_PORTSC_PORT_RESET		= (1 << 9),
};

/* UHCI Frame List Pointer. */
typedef __u32	bl_uhci_frame_list_pointer_t;

enum {
	BL_UHCI_FRAME_LIST_POINTER_T	= (1 << 0), /* 1 = Empty frame. 0 = valid. */
	BL_UHCI_FRAME_LIST_POINTER_Q	= (1 << 1), /* 1 = QH. 0 = TD. */
};

/* UHCI Transfer Decriptor (TD). */
struct bl_uhci_td {
	__u32 link_pointer;
	__u32 control_status;
	__u32 token;
	__u32 buffer_pointer;

	/* Software use. */
	int used;

	__u32 reserved[3];
} __attribute__((packed));

enum {
	BL_UHCI_TD_LINK_POINTER_T	= (1 << 0), /* 1 = Empty frame. 0 = valid. */
	BL_UHCI_TD_LINK_POINTER_Q	= (1 << 1), /* 1 = QH. 0 = TD. */
	BL_UHCI_TD_LINK_POINTER_VF	= (1 << 2), /* 1 = Depth first. 0 = Breadth first. */
	BL_UHCI_TD_LINK_POINTER		= (~0xf),

	BL_UHCI_TD_CONTROL_STATUS_BITSTUFF_ERROR	= (1 << 17),
	BL_UHCI_TD_CONTROL_STATUS_TIMEOUT_ERROR		= (1 << 18),
	BL_UHCI_TD_CONTROL_STATUS_NAK_RECEIVED		= (1 << 19),
	BL_UHCI_TD_CONTROL_STATUS_BABBLE_DETECTED	= (1 << 20),
	BL_UHCI_TD_CONTROL_STATUS_DATA_BUFFER_ERROR	= (1 << 21),
	BL_UHCI_TD_CONTROL_STATUS_STALLED		= (1 << 22),
	BL_UHCI_TD_CONTROL_STATUS_ACTIVE		= (1 << 23),
	BL_UHCI_TD_CONTROL_STATUS_LOW_SPEED_DEVICE	= (1 << 26),
};

#define BL_UHCI_TD_LINK_POINTER_NEXT(td)	\
	((volatile struct bl_uhci_td *)(td->link_pointer & BL_UHCI_TD_LINK_POINTER))

#define BL_UHCI_TD_CONTROL_STATUS_ACTUAL_LENGTH(length)	((length - 1) & 0x7ff)
#define BL_UHCI_TD_CONTROL_STATUS(control_status)	(control_status & 0xff0000)
#define BL_UHCI_TD_CONTROL_STATUS_C_ERR(count)		((count & 0x3) << 27)

#define BL_UHCI_TD_TOKEN_DEVICE_ADDRESS(address)	(address << 8)
#define BL_UHCI_TD_TOKEN_ENDPOINT(endpoint)		(endpoint << 15)
#define BL_UHCI_TD_TOKEN_TOGGLE(address)		(toggle << 19)
#define BL_UHCI_TD_TOKEN_MAXIMUM_LENGTH(length)		(((length -1) & 0x7ff) << 21)

/* UHCI Queue Head (QH). */
struct bl_uhci_qh {
	__u32 head_link_pointer;
	__u32 element_link_pointer;

	/* Software use. */
	int used;
	bl_uint32_t original_element;
} __attribute__((packed));

enum {
	BL_UHCI_QH_HEAD_LINK_POINTER_T	= (1 << 0), /* 1 = Empty frame. 0 = valid. */
	BL_UHCI_QH_HEAD_LINK_POINTER_Q	= (1 << 1), /* 1 = QH. 0 = TD. */
};

enum {
	BL_UHCI_QH_ELEMENT_LINK_POINTER_T	= (1 << 0), /* 1 = Empty frame. 0 = valid. */
	BL_UHCI_QH_ELEMENT_LINK_POINTER_Q	= (1 << 1), /* 1 = QH. 0 = TD. */
};

#define BL_UHCI_QH_HEAD_LINK_POINTER(qh)	\
	((volatile struct bl_uhci_qh *)(qh->head_link_pointer & (~0xf)))

#define BL_UHCI_QH_ELEMENT_LINK_POINTER(qh)	\
	((volatile struct bl_uhci_td *)(qh->element_link_pointer & (~0xf)))

#endif

