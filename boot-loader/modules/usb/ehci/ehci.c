#include "ehci.h"
#include "include/export.h"
#include "include/string.h"
#include "include/time.h"
#include "include/bl-utils.h"
#include "core/include/loader/loader.h"
#include "core/include/pci/pci.h"
#include "core/include/usb/usb.h"
#include "core/include/video/print.h"
#include "core/include/memory/heap.h"

BL_MODULE_NAME("USB EHCI");

#define BL_EHCI_NUM_QTDS	256
#define BL_EHCI_NUM_QHS		64

struct bl_ehci_controller {
	/* EHCI capability registers. */
	volatile struct bl_ehci_capability_registers *cap_regs;

	/* EHCI operational registers. */
	volatile struct bl_ehci_operational_registers *op_regs;

	/* Periodic frame list. */
	struct bl_ehci_qh *periodic_qh;
	volatile bl_ehci_frame_list_pointer_t *framelist_ptrs;

	/* Available queue heads. */
	volatile struct bl_ehci_qh *qh_head;
	volatile struct bl_ehci_qh *qhs;

	/* Available queue transfer descriptors. */
	volatile struct bl_ehci_qtd *qtds;

	struct bl_ehci_controller *next;
};
static struct bl_ehci_controller *ehci_list = NULL;

static inline void bl_ehci_controller_add(struct bl_ehci_controller *ehci)
{
        ehci->next = ehci_list;
        ehci_list = ehci;
}

static inline void bl_ehci_reset_qh(volatile struct bl_ehci_qh *qh)
{
	qh->overlay.next_qtd = BL_EHCI_QTD_LINK_T;
	qh->overlay.alternate_next_qtd = BL_EHCI_QTD_LINK_T;

	bl_memset((void *)&qh->overlay, 0, sizeof(struct bl_ehci_qtd));

	qh->overlay.token &= ~BL_EHCI_QTD_TOKEN_ACTIVE;
	qh->overlay.token |= BL_EHCI_QTD_TOKEN_HALTED;
}

static inline int bl_ehci_number_of_ports(struct bl_ehci_controller *ehci)
{
	return ehci->cap_regs->hc_sparams & BL_EHCI_CAP_REG_HCSPARAMS_N_PORTS;
}

static int bl_ehci_port_device_connected(struct bl_usb_host_controller *hc,
	int port)
{
	struct bl_ehci_controller *ehci;

	ehci = hc->data;

	if (port > bl_ehci_number_of_ports(ehci) ||  port < 0)
		return 0;

	if (ehci->op_regs->port_sc[port] & BL_EHCI_OP_REG_PORTSC_CONNECT_STATUS) {
		ehci->op_regs->port_sc[port] |= BL_EHCI_OP_REG_PORTSC_CONNECT_STATUS;
		return 1;
	} else
		return 0;
}

static bl_usb_speed_t bl_ehci_port_device_get_speed(struct bl_usb_host_controller *hc,
	int port)
{
	struct bl_ehci_controller *ehci;

	ehci = hc->data;

	if (port > bl_ehci_number_of_ports(ehci) ||  port < 0)
		return -1;

	/* Companion host controller owns the port. */
	if (ehci->op_regs->port_sc[port] & BL_EHCI_OP_REG_PORTSC_PORT_OWNER)
		return -1;

	/* If the port is already enabled there is a connected high-speed device. */
	if (ehci->op_regs->port_sc[port] & BL_EHCI_OP_REG_PORTSC_ENABLED)
		return BL_USB_HIGH_SPEED;
	else {
		/* Check anyway. */
		if (bl_ehci_port_device_connected(hc, port)) {
			if ((ehci->op_regs->port_sc[port] & BL_EHCI_OP_REG_PORTSC_LINE_STATUS)
				== BL_EHCI_OP_REG_PORTSC_LINE_STATUS_K)
				return -1;
			else
				return BL_USB_HIGH_SPEED;
		} else
			return -1;
	}
}

static void bl_ehci_port_reset(struct bl_usb_host_controller *hc, int port)
{
	struct bl_ehci_controller *ehci;

	ehci = hc->data;

	if (port > bl_ehci_number_of_ports(ehci) ||  port < 0)
		return;

	/* First disable the port. */
	ehci->op_regs->port_sc[port] &= ~BL_EHCI_OP_REG_PORTSC_ENABLED;

	int timeout = 10;
	while (--timeout) {
		if ((ehci->op_regs->port_sc[port] & BL_EHCI_OP_REG_PORTSC_ENABLED)
			== 0)
			break;

		bl_time_sleep(1);
	}

	if (ehci->op_regs->port_sc[port] & BL_EHCI_OP_REG_PORTSC_ENABLED_CHANGE)
		return;

	/* Reset the port. */
	ehci->op_regs->port_sc[port] |= BL_EHCI_OP_REG_PORTSC_PORT_RESET;
	bl_time_sleep(50);
	ehci->op_regs->port_sc[port] &= ~BL_EHCI_OP_REG_PORTSC_PORT_RESET;

	/* EHCI specification says we should wait for the status change to be applied.
	   Perform reset recovery. */
	bl_time_sleep(10);
	
	if ((ehci->op_regs->port_sc[port] & BL_EHCI_OP_REG_PORTSC_ENABLED) == 0)
		return;
}

static void bl_ehci_set_qh(volatile struct bl_ehci_qh *qh, bl_usb_transfer_t transfer,
	int address, int endpoint, bl_usb_speed_t speed, int max_packet_length)
{
	/* Endpoint characteristics. */
	qh->chars |= BL_EHCI_MAKE_QH_ADDRESS(address);
	qh->chars |= BL_EHCI_MAKE_QH_ENDPOINT(endpoint);

	switch (speed) {
	case BL_USB_LOW_SPEED:
		qh->chars |= BL_EHCI_QH_CHARS_LOW_SPEED;
		break;

	case BL_USB_FULL_SPEED:
		qh->chars |= BL_EHCI_QH_CHARS_FULL_SPEED;
		break;

	case BL_USB_HIGH_SPEED:
		qh->chars |= BL_EHCI_QH_CHARS_HIGH_SPEED;
		break;

	default:
		return;
	}

	qh->chars |= BL_EHCI_QH_CHARS_DTC;
	qh->chars |= BL_EHCI_MAKE_QH_MAX_PACKET_LEN(max_packet_length);

	if (speed != BL_USB_HIGH_SPEED && transfer == BL_USB_TRANSFER_CONTROL)
		qh->chars |= BL_EHCI_QH_CHARS_C;
}

static inline void bl_ehci_link_qh(struct bl_ehci_controller *ehci, volatile struct bl_ehci_qh *qh)
{
	qh->link = ehci->qh_head->link | BL_EHCI_QH_LINK_TYPE_QH;
	ehci->qh_head->link = (bl_uint32_t)qh | BL_EHCI_QH_LINK_TYPE_QH;
}

static volatile struct bl_ehci_qh *bl_ehci_alloc_qh(struct bl_ehci_controller *ehci,
	int address, int endpoint)
{
	int i;
	bl_uint32_t qh_address, qh_endpoint;

	qh_address = BL_EHCI_MAKE_QH_ADDRESS(address);
	qh_endpoint = BL_EHCI_MAKE_QH_ENDPOINT(endpoint);

	for (i = 1; i < BL_EHCI_NUM_QHS; i++) {
		if ((ehci->qhs[i].chars & BL_EHCI_QH_CHARS_ADDRESS) == qh_address &&
			(ehci->qhs[i].chars & BL_EHCI_QH_CHARS_ENDPOINT) == qh_endpoint) {
			//bl_print_hex(i);
			//bl_print_str("a");
			return &ehci->qhs[i];
		}

		/* Free endpoint. */
		if ((ehci->qhs[i].chars & BL_EHCI_QH_CHARS_ADDRESS) == 0 &&
			(ehci->qhs[i].chars & BL_EHCI_QH_CHARS_ENDPOINT) == 0) {
			//bl_print_hex(i);
			//bl_print_str("b");
			return &ehci->qhs[i];
		}
	}

	return NULL;
}

/* Reserve the first queue head (0) for default control pipe. */
static volatile struct bl_ehci_qh *bl_ehci_init_qh(struct bl_ehci_controller *ehci,
	bl_usb_transfer_t transfer, int address, int endpoint, bl_usb_speed_t speed,
	int max_packet_length)
{
	volatile struct bl_ehci_qh *qh;

	if (transfer == BL_USB_TRANSFER_CONTROL) {
		if (address == 0 && endpoint == 0)
			qh = &ehci->qhs[0];
		else
			qh = bl_ehci_alloc_qh(ehci, address, endpoint);
	} else if (transfer == BL_USB_TRANSFER_BULK) {

	} else
		qh = NULL;

	if (!qh)
		return NULL;

	if (!qh->used) {
		bl_ehci_set_qh(qh, transfer, address, endpoint, speed,
			max_packet_length);

		bl_ehci_link_qh(ehci, qh);

		qh->used = 1;
	}

	return qh;
}

static volatile struct bl_ehci_qtd *bl_ehci_alloc_qtd(struct bl_ehci_controller *ehci)
{
	int i;

	for (i = 0; i <  BL_EHCI_NUM_QTDS; i++) {
		if (!ehci->qtds[i].used) {
			ehci->qtds[i].used = 1;

			//bl_print_hex(i); bl_print_str(" ");

			return &ehci->qtds[i];
		}
	}

	return NULL;
}

static volatile struct bl_ehci_qtd *bl_ehci_init_qtd(struct bl_ehci_controller *ehci,
	bl_uint8_t *buf, int length, int toggle, bl_usb_pid_t pid)
{
	volatile struct bl_ehci_qtd *qtd;

	if (length > BL_EHCI_QTD_TOKEN_MAX_BUFFER_SIZE || length < 0)
		return NULL;

	qtd = bl_ehci_alloc_qtd(ehci);
	if (!qtd)
		return NULL;

	qtd->original_next_qtd = qtd->next_qtd = qtd->alternate_next_qtd = BL_EHCI_QTD_LINK_T;

	qtd->token = 0;

	qtd->token |= BL_EHCI_QTD_TOKEN_ACTIVE;

	switch (pid) {
	case BL_USB_PID_TOKEN_SETUP:
		qtd->token |= BL_EHCI_QTD_TOKEN_PID_SETUP;
		break;

	case BL_USB_PID_TOKEN_OUT:
		qtd->token |= BL_EHCI_QTD_TOKEN_PID_OUT;
		break;

	case BL_USB_PID_TOKEN_IN:
		qtd->token |= BL_EHCI_QTD_TOKEN_PID_IN;
		break;
	}

	qtd->token |= BL_EHCI_MAKE_QTD_CERR(3);

	qtd->token |= BL_EHCI_MAKE_QTD_C_PAGE(0);

	bl_memset((void *)qtd->buffer_pointer, 0, sizeof(qtd->buffer_pointer));
	bl_memset((void *)qtd->extended_buffer_pointer, 0, sizeof(qtd->extended_buffer_pointer));

	if (buf && length) {
		qtd->token |= BL_EHCI_MAKE_QTD_BUFFER_SIZE(length);

		int i, left;

		left = length;
		qtd->buffer_pointer[0] = (bl_uint32_t)buf;

	#define TD_BUFFER_ALIGNMENT(index)							\
		if (qtd->buffer_pointer[index] % BL_EHCI_QTD_TOKEN_BUFFER_SIZE != 0)		\
			left -= BL_MEMORY_ALIGN_UP(qtd->buffer_pointer[index],			\
				BL_EHCI_QTD_TOKEN_BUFFER_SIZE) - qtd->buffer_pointer[index];	\
		else										\
			left -= BL_EHCI_QTD_TOKEN_BUFFER_SIZE;

		TD_BUFFER_ALIGNMENT(0);

		i = 1;
		while (left > 0) {
			qtd->buffer_pointer[i] = BL_MEMORY_ALIGN_UP(qtd->buffer_pointer[i - 1],
				BL_EHCI_QTD_TOKEN_BUFFER_SIZE);

			TD_BUFFER_ALIGNMENT(i);
		}
	} else
		qtd->token |= BL_EHCI_MAKE_QTD_BUFFER_SIZE(0);

	qtd->token |= BL_EHCI_MAKE_QTD_TOGGLE(toggle);

	return qtd;
}

static void bl_ehci_free_qtd_list(volatile struct bl_ehci_qh *qh)
{
	volatile struct bl_ehci_qtd *qtd;

	bl_ehci_reset_qh(qh);

	qtd = (volatile struct bl_ehci_qtd *)qh->first_qtd;

	while (1) {
		qtd->used = 0;

		if (qtd->original_next_qtd & BL_EHCI_QTD_LINK_T)
			break;

		qtd = (volatile struct bl_ehci_qtd *)qtd->original_next_qtd;
	}
}

#if 0
static void foo(volatile struct bl_ehci_qh *qh)
{
	volatile struct bl_ehci_qtd *qtd;

	qtd = (volatile struct bl_ehci_qtd *)qh->first_qtd;

	bl_print_hex(qh->link);
	bl_print_str(" ");
	bl_print_hex(qh->chars);
	bl_print_str(" ");
	bl_print_hex(qh->caps);
	bl_print_str(" ");
	bl_print_hex(qh->overlay.next_qtd);
	bl_print_str(" ");
	bl_print_hex(qh->overlay.original_next_qtd);
	bl_print_str(" ");
	bl_print_hex(qh->overlay.token);
	bl_print_str(" ");
	bl_print_hex(qh->overlay.buffer_pointer[0]);
	bl_print_str("\n");

	while (1) {
		bl_print_hex(qtd->next_qtd);
		bl_print_str(" ");
		bl_print_hex(qtd->original_next_qtd);
		bl_print_str(" ");
		bl_print_hex(qtd->token);
		bl_print_str(" ");
		bl_print_hex(qtd->buffer_pointer[0]);
		bl_print_str("\n");

		if (qtd->original_next_qtd & BL_EHCI_QTD_LINK_T)
			break;

		qtd = (volatile struct bl_ehci_qtd *)qtd->original_next_qtd;
	}
}
#endif

static bl_status_t bl_ehci_transfer_status(volatile struct bl_ehci_qh *qh)
{
	bl_status_t status;

	switch (BL_EHCI_QTD_TOKEN_STATUS(qh->overlay.token)) {
	case BL_EHCI_QTD_TOKEN_BABBLE_DETECTED:
		status = BL_STATUS_USB_BABBLE_DETECTED;
		break;

	case BL_EHCI_QTD_TOKEN_MISSED_MICRO_FRAME:
	case BL_EHCI_QTD_TOKEN_XACTERR:
	case BL_EHCI_QTD_TOKEN_DATA_BUFFER_ERROR:
		status = BL_STATUS_USB_DATA_BUFFER_ERROR;
		break;

	default:
		status = BL_STATUS_USB_NAK_RECEIVED;
		break;
	}

	return status;
}

static bl_status_t bl_ehci_transfer_finished(volatile struct bl_ehci_qh *qh)
{
#if 0
	bl_print_hex(qh->overlay.token);
	bl_print_str(" ");
	bl_print_hex(qh->overlay.next_qtd);
	bl_print_str(" ");
	bl_print_hex(qh->current_qtd);
	bl_print_str("\n");
#endif

	if (qh->overlay.token & BL_EHCI_QTD_TOKEN_HALTED)
		return BL_STATUS_USB_SHOULD_CHECK_STATUS;

	if (((qh->overlay.token & BL_EHCI_QTD_TOKEN_ACTIVE) == 0) &&
		qh->overlay.next_qtd == BL_EHCI_QTD_LINK_T &&
		qh->last_qtd == qh->current_qtd)
		return BL_STATUS_SUCCESS;

	return BL_STATUS_USB_ACTION_NOT_FINISHED;
}

static bl_status_t bl_ehci_check_transfer_timeout(volatile struct bl_ehci_qh *qh,
	int timeout)
{
	bl_status_t status;

	while (--timeout) {
		bl_time_sleep(1);

		status = bl_ehci_transfer_finished(qh);
		if (!status) {
			bl_ehci_free_qtd_list(qh);
			return status;
		} else if (status == BL_STATUS_USB_SHOULD_CHECK_STATUS)
			break;
	}

	bl_print_str("EHCI transfer error\n");
	return bl_ehci_transfer_status(qh);
}

static bl_status_t bl_ehci_control_transfer(struct bl_usb_host_controller *hc,
	int packet_size, int address, bl_usb_speed_t speed,
	struct bl_usb_setup_data *setup, bl_uint8_t *data, int length)
{
	struct bl_ehci_controller *ehci;
	volatile struct bl_ehci_qh *qh;
	volatile struct bl_ehci_qtd *qtd_head, *qtd, *qtd_prev;
	int toggle = 0;
	int off = 0;

	ehci = hc->data;

	if (!packet_size)
		packet_size = 8;

	qh = bl_ehci_init_qh(ehci, BL_USB_TRANSFER_CONTROL, address, 0, speed,
		packet_size);
	if (!qh)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* SETUP */
	qtd_head = qtd = qtd_prev = bl_ehci_init_qtd(ehci, (bl_uint8_t *)setup,
		sizeof(struct bl_usb_setup_data), toggle, BL_USB_PID_TOKEN_SETUP);
	if (!qtd)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* DATA IN/OUT */
	if (length == 0) {
		qtd_prev = qtd;
		toggle = !toggle;

		qtd = bl_ehci_init_qtd(ehci, NULL, 0, toggle,
			/* TODO: Works for requests like SET_ADDRESS.
			   Does this always work ? */
			   BL_USB_PID_TOKEN_IN);
		if (!qtd)
			return BL_STATUS_USB_INTERNAL_ERROR;

		qtd_prev->original_next_qtd = qtd_prev->next_qtd = (bl_uint32_t)qtd;
        } else {
		while (length > 0) {
			qtd_prev = qtd;
			toggle = !toggle;

			qtd = bl_ehci_init_qtd(ehci, data + off, BL_MIN(length, packet_size), toggle,
				setup->request_type & BL_USB_SETUP_REQUEST_TYPE_DEVICE_TO_HOST ?
				BL_USB_PID_TOKEN_IN : BL_USB_PID_TOKEN_OUT);
			if (!qtd)
				return BL_STATUS_USB_INTERNAL_ERROR;

			qtd_prev->original_next_qtd = qtd_prev->next_qtd = (bl_uint32_t)qtd;
			length -= packet_size;
			off += packet_size;
		}

		/* STATUS */
		qtd_prev = qtd;
		qtd = bl_ehci_init_qtd(ehci, NULL, 0, 1,
			setup->request_type & BL_USB_SETUP_REQUEST_TYPE_DEVICE_TO_HOST ?
			BL_USB_PID_TOKEN_IN : BL_USB_PID_TOKEN_OUT);
		if (!qtd)
			return BL_STATUS_USB_INTERNAL_ERROR;

		qtd_prev->original_next_qtd = qtd_prev->next_qtd = (bl_uint32_t)qtd;
	}

	qh->first_qtd = qh->overlay.next_qtd = (bl_uint32_t)qtd_head;
	qh->last_qtd = (bl_uint32_t)qtd;

	/* Start transfer. */
	qh->overlay.token &= ~(BL_EHCI_QTD_TOKEN_HALTED | BL_EHCI_QTD_TOKEN_ACTIVE);

	return bl_ehci_check_transfer_timeout(qh, 100);
}

static void bl_ehci_controller_uninit(struct bl_ehci_controller *ehci)
{
	if (!ehci)
		return;

	if (ehci->qh_head) {
		bl_heap_free((void *)ehci->qh_head, sizeof(struct bl_ehci_qh));
		ehci->qh_head = NULL;
	}

	if (ehci->qhs) {
		bl_heap_free((void *)ehci->qhs, BL_EHCI_NUM_QHS * sizeof(struct bl_ehci_qh));
		ehci->qhs = NULL;
	}

	bl_heap_free(ehci, sizeof(struct bl_ehci_controller));
}

static bl_status_t bl_ehci_init_data_structures(struct bl_ehci_controller *ehci)
{
	int i;

	/* Frame list pointers & periodic processing. */
	ehci->framelist_ptrs = bl_heap_alloc_align(0x1000, 0x1000);
	if (!ehci->framelist_ptrs)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

#if 0
	ehci->periodic_qh = bl_heap_alloc_align(sizeof(struct bl_ehci_qh), 0x40);
	if (!ehci->periodic_qh)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	bl_memset(ehci->periodic_qh, 0, sizeof(struct bl_ehci_qh));
	ehci->periodic_qh->link |= BL_EHCI_QH_LINK_T | BL_EHCI_QH_LINK_TYPE_QH;
	ehci->periodic_qh->overlay.next_qtd |= BL_EHCI_QTD_LINK_T;
	ehci->periodic_qh->overlay.alternate_next_qtd |= BL_EHCI_QTD_LINK_T;
#endif

	for (i = 0; i < 1024; i++)
		ehci->framelist_ptrs[i] = BL_EHCI_FRAME_LIST_PTR_T;


	/* Queue head. */
	ehci->qh_head = bl_heap_alloc_align(sizeof(struct bl_ehci_qh), 0x20);
	if (!ehci->qh_head)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	ehci->qh_head->link = (bl_uint32_t)ehci->qh_head | BL_EHCI_QH_LINK_TYPE_QH |
		BL_EHCI_QH_LINK_T;

	bl_ehci_reset_qh(ehci->qh_head);

	ehci->qh_head->chars = BL_EHCI_QH_CHARS_H;

	/* Allocate queue head pool. */
	ehci->qhs = bl_heap_alloc_align(BL_EHCI_NUM_QHS * sizeof(struct bl_ehci_qh), 0x20);
	if (!ehci->qhs)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_EHCI_NUM_QHS; i++) {
		ehci->qhs[i].used = 0;

		ehci->qhs[i].link = (bl_uint32_t)&ehci->qhs[i] | BL_EHCI_QH_LINK_TYPE_QH |
			BL_EHCI_QH_LINK_T;

		bl_ehci_reset_qh(&ehci->qhs[i]);
	}

	/* Allocate transfer descriptors. */
	ehci->qtds = bl_heap_alloc_align(BL_EHCI_NUM_QTDS * sizeof(struct bl_ehci_qtd), 0x20);
	if (!ehci->qtds)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_EHCI_NUM_QTDS; i++)
		ehci->qtds[i].original_next_qtd = ehci->qtds[i].next_qtd =
			ehci->qtds[i].alternate_next_qtd = BL_EHCI_QTD_LINK_T;

	/* Enable both periodic & async. list processing. */
	ehci->op_regs->periodic_list_base = (bl_uint32_t)ehci->framelist_ptrs;

	ehci->op_regs->async_list_addr = (bl_uint32_t)ehci->qh_head;

	ehci->op_regs->usb_cmd |= BL_EHCI_OP_REG_USBCMD_PERIODIC_ENABLE |
		BL_EHCI_OP_REG_USBCMD_ASYNC_ENABLE;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ehci_controller_init(struct bl_ehci_controller *ehci)
{
	bl_status_t status;

	if (ehci->cap_regs->hci_version != BL_EHCI_CAP_REG_HCI_VERSION)
		return BL_STATUS_USB_INVALID_HOST_CONTROLLER;

	/* Operational registers. */
	ehci->op_regs = (volatile struct bl_ehci_operational_registers *)
		((char *)ehci->cap_regs + ehci->cap_regs->cap_length);

	/* Stop the controller -> HCHalted should be set then. */
	ehci->op_regs->usb_cmd &= ~BL_EHCI_OP_REG_USBCMD_RUN_STOP;
	bl_time_sleep(2); // Maximum of 16 micro-frames (125 micro-seconds each) = 2ms.
	if ((ehci->op_regs->usb_sts & BL_EHCI_OP_REG_USBSTS_HC_HALTED) == 0)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* Reset EHCI. */
	ehci->op_regs->usb_cmd |= BL_EHCI_OP_REG_USBCMD_HC_RESET;

	int timeout = 50;
	while (--timeout) {
		if ((ehci->op_regs->usb_cmd & BL_EHCI_OP_REG_USBCMD_HC_RESET) == 0)
			break;

		bl_time_sleep(1);
	}

	if (ehci->op_regs->usb_cmd & BL_EHCI_OP_REG_USBSTS_HC_HALTED)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* Initialize relevant info. */
	status = bl_ehci_init_data_structures(ehci);
	if (status)
		return status;

	/* Route all ports to EHCI. */
	ehci->op_regs->config_flag |= BL_EHCI_OP_REG_CONFIGFLAG_HC_ROUTE;

	/* Run again the contorller. */
	ehci->op_regs->usb_cmd |= BL_EHCI_OP_REG_USBCMD_RUN_STOP;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ehci_pci_init(struct bl_pci_index i)
{
	bl_status_t status;
	bl_uint8_t sbrn;
	bl_uint32_t base_address;
	struct bl_ehci_controller *ehci;

	/* Check PCI device. */
	status = bl_pci_check_device_class(i, BL_PCI_BASE_CLASS_SERIAL,
		BL_PCI_SUBCLASS_SERIAL_USB_EHCI, BL_PCI_PROG_IF_SERIAL_USB_EHCI);
	if (status)
                return status;

	/* Check serial bus release number (release 2.0). */
	sbrn = bl_pci_read_config_byte(i.bus, i.dev, i.func, BL_PCI_CONFIG_REG_60H);
	if (sbrn != BL_EHCI_PCI_SBRN)
		return BL_STATUS_USB_INVALID_HOST_CONTROLLER;

	/* Check base address. */
	base_address = bl_pci_read_config_long(i.bus, i.dev, i.func, BL_PCI_CONFIG_REG_BAR0);
	if ((base_address & BL_EHCI_PCI_BAR0_TYPE) != BL_EHCI_PCI_BAR0_TYPE_32_BIT)
		return BL_STATUS_USB_INVALID_HOST_CONTROLLER;

	/* Initialize the controller. */
	ehci = bl_heap_alloc(sizeof(struct bl_ehci_controller));
	if (!ehci)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	/* Capability registers. */
	ehci->cap_regs = (void *)(base_address & BL_EHCI_PCI_BAR0_BASE_ADDRESS);

	status = bl_ehci_controller_init(ehci);
	if (status) {
		bl_ehci_controller_uninit(ehci);
		return status;
	}

	bl_ehci_controller_add(ehci);

	return BL_STATUS_SUCCESS;
}

static struct bl_usb_host_controller_functions ehci_functions = {
	.port_device_connected = bl_ehci_port_device_connected,
	.port_device_get_speed = bl_ehci_port_device_get_speed,
	.port_reset = bl_ehci_port_reset,
	.control_transfer = bl_ehci_control_transfer,
};

BL_MODULE_INIT()
{
	struct bl_ehci_controller *ehci;

	bl_pci_iterate_devices(bl_ehci_pci_init);

	ehci = ehci_list;
	while (ehci) {
		struct bl_usb_host_controller *hc =
			bl_heap_alloc(sizeof(struct bl_usb_host_controller));
		if (!hc)
			return;

		hc->type = BL_USB_HC_EHCI,
		hc->funcs = &ehci_functions;
		hc->data = (void *)ehci;
		hc->root_hub.ports = bl_ehci_number_of_ports(ehci);

		bl_usb_host_controller_register(hc);

		ehci = ehci->next;
	}
}

BL_MODULE_UNINIT()
{

}

