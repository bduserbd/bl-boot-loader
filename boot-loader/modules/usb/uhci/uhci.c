#include "uhci.h"
#include "include/export.h"
#include "include/time.h"
#include "include/bl-utils.h"
#include "core/include/loader/loader.h"
#include "core/include/usb/usb.h"
#include "core/include/pci/pci.h"
#include "core/include/video/print.h"
#include "core/include/memory/heap.h"

BL_MODULE_NAME("USB UHCI");

#define BL_UHCI_NUM_TD	1024
#define BL_UHCI_NUM_QH	256

#define BL_UHCI_REG(uhci, reg)	((uhci)->io + (reg))

struct bl_uhci_controller {
	bl_port_t io;

	/* Frame list. The host controller executes each entry for 1 ms.
	   Allocate it aligned to 4k boundary. */
	volatile bl_uhci_frame_list_pointer_t *framelist_ptrs;

	/* Transfer descriptors. */
	volatile struct bl_uhci_td *tds;

	/* Queue heads. */
	volatile struct bl_uhci_qh *qhs;

	struct bl_uhci_controller *next;
};
struct bl_uhci_controller *uhci_list = NULL;

static inline void bl_uhci_controller_add(struct bl_uhci_controller *uhci)
{
	uhci->next = uhci_list;
	uhci_list = uhci;
}

static bl_uhci_reg_t bl_uhci_port_to_reg(int port)
{
	/* Root hub has 2 ports. */
	if (port == 0)
		return BL_UHCI_IO_REG_PORTSC1;
	else if (port == 1)
		return BL_UHCI_IO_REG_PORTSC2;

	return 0;
}

static int bl_uhci_port_device_connected(struct bl_usb_host_controller *hc, int port)
{
	struct bl_uhci_controller *uhci;
	bl_uhci_reg_t reg;

	uhci = hc->data;
	reg = bl_uhci_port_to_reg(port);

	return !!(bl_inw(BL_UHCI_REG(uhci, reg)) & BL_UHCI_PORTSC_CURRENT_CONNECT_STATUS);
}

static bl_usb_speed_t bl_uhci_port_device_get_speed(struct bl_usb_host_controller *hc,
	int port)
{
	struct bl_uhci_controller *uhci;
	bl_uhci_reg_t reg;

	uhci = hc->data;
	reg = bl_uhci_port_to_reg(port);

	if (bl_inw(BL_UHCI_REG(uhci, reg)) & BL_UHCI_PORTSC_LOW_SPEED_DEVICE)
		return BL_USB_LOW_SPEED;
	else
		return BL_USB_FULL_SPEED;
}

static void bl_uhci_port_reset(struct bl_usb_host_controller *hc, int port)
{
	struct bl_uhci_controller *uhci;
	bl_uhci_reg_t reg;
	bl_uint16_t status;

	uhci = hc->data;
	reg = bl_uhci_port_to_reg(port);

	/* Port reset. */
	status = bl_inw(BL_UHCI_REG(uhci, reg));
	status |= BL_UHCI_PORTSC_PORT_RESET;
	bl_outw(status, BL_UHCI_REG(uhci, reg));

	bl_time_sleep(50);

	/* Port is not in reset mode. */
	status = bl_inw(BL_UHCI_REG(uhci, reg));
	status &= ~BL_UHCI_PORTSC_PORT_RESET;
	bl_outw(status, BL_UHCI_REG(uhci, reg));

	/* Enable port. */
	status = bl_inw(BL_UHCI_REG(uhci, reg));
	status |= BL_UHCI_PORTSC_PORT_ENABLED;
	bl_outw(status, BL_UHCI_REG(uhci, reg));
}

static volatile struct bl_uhci_qh *bl_uhci_alloc_qh(struct bl_uhci_controller *uhci)
{
	int i;

	for (i = 0; i < BL_UHCI_NUM_QH; i++)
		if (!uhci->qhs[i].used) {
			uhci->qhs[i].used = 1;
			return &uhci->qhs[i];
		}

	return NULL;
}

static volatile struct bl_uhci_td *bl_uhci_alloc_td(struct bl_uhci_controller *uhci)
{
	int i;

	for (i = 0; i < BL_UHCI_NUM_TD; i++)
		if (!uhci->tds[i].used) {
			uhci->tds[i].used = 1;
			return &uhci->tds[i];
		}

	return NULL;
}

static volatile struct bl_uhci_td *bl_uhci_init_td(struct bl_uhci_controller *uhci,
	bl_usb_speed_t speed, bl_usb_pid_t pid, bl_uint8_t address, bl_uint8_t endpoint,
	unsigned toggle, bl_size_t size, bl_addr_t buffer)
{
	volatile struct bl_uhci_td *td;

	td = bl_uhci_alloc_td(uhci);
	if (!td)
		return NULL;

	td->link_pointer = BL_UHCI_TD_LINK_POINTER_T;

	td->control_status = BL_UHCI_TD_CONTROL_STATUS_ACTUAL_LENGTH(size);
	td->control_status |= BL_UHCI_TD_CONTROL_STATUS_ACTIVE;
	if (speed == BL_USB_LOW_SPEED)
		td->control_status |= BL_UHCI_TD_CONTROL_STATUS_LOW_SPEED_DEVICE;

	td->control_status |= BL_UHCI_TD_CONTROL_STATUS_C_ERR(3);

	td->token = bl_usb_pid_format_check(pid);
	td->token |= BL_UHCI_TD_TOKEN_DEVICE_ADDRESS(address);
	td->token |= BL_UHCI_TD_TOKEN_ENDPOINT(endpoint);
	td->token |= BL_UHCI_TD_TOKEN_TOGGLE(toggle);
	td->token |= BL_UHCI_TD_TOKEN_MAXIMUM_LENGTH(size);

	td->buffer_pointer = buffer;

	return td;
}

static void bl_uhci_free_td_list(volatile struct bl_uhci_td *td)
{
	while (1) {
		td->used = 0;

		if (td->link_pointer & BL_UHCI_TD_LINK_POINTER_T)
			break;

		td = BL_UHCI_TD_LINK_POINTER_NEXT(td);
	}
}

static bl_status_t bl_uhci_transfer_status(volatile struct bl_uhci_qh *qh)
{
	bl_status_t status;
	volatile struct bl_uhci_td *td;

	td = BL_UHCI_QH_ELEMENT_LINK_POINTER(qh);

	switch (BL_UHCI_TD_CONTROL_STATUS(td->control_status)) {
	case BL_UHCI_TD_CONTROL_STATUS_BITSTUFF_ERROR:
		status = BL_STATUS_USB_BITSTUFF_ERROR;
		break;

	case BL_UHCI_TD_CONTROL_STATUS_TIMEOUT_ERROR:
		status = BL_STATUS_USB_TIMEOUT_ERROR;
		break;

	case BL_UHCI_TD_CONTROL_STATUS_NAK_RECEIVED:
		status = BL_STATUS_USB_NAK_RECEIVED;
		break;

	case BL_UHCI_TD_CONTROL_STATUS_BABBLE_DETECTED:
		status = BL_STATUS_USB_BABBLE_DETECTED;
		break;

	case BL_UHCI_TD_CONTROL_STATUS_DATA_BUFFER_ERROR:
		status = BL_STATUS_USB_DATA_BUFFER_ERROR;
		break;

	case BL_UHCI_TD_CONTROL_STATUS_STALLED:
		status = BL_STATUS_USB_STALLED;
		break;

	default:
		status = BL_STATUS_SUCCESS;
		break;
	}

	return status;
}

static bl_status_t bl_uhci_transfer_finished(volatile struct bl_uhci_qh *qh)
{
	if (qh->element_link_pointer & BL_UHCI_QH_ELEMENT_LINK_POINTER_T) {
		if (BL_UHCI_QH_ELEMENT_LINK_POINTER(qh))
			return BL_STATUS_USB_SHOULD_CHECK_STATUS;

		bl_uhci_free_td_list((volatile struct bl_uhci_td *)qh->original_element);
		qh->used = 0;

		return BL_STATUS_SUCCESS;
	}

	return BL_STATUS_USB_ACTION_NOT_FINISHED;
}

static bl_status_t bl_uhci_check_transfer_status(bl_usb_transfer_info_t info)
{
	bl_status_t status;
	volatile struct bl_uhci_qh *qh;

	qh = info;

	status = bl_uhci_transfer_finished(qh);
	if (status == BL_STATUS_USB_SHOULD_CHECK_STATUS)
		return bl_uhci_transfer_status(qh);

	return status;
}

static bl_status_t bl_uhci_check_transfer_timeout(volatile struct bl_uhci_qh *qh,
	int timeout)
{
	bl_status_t status;

	while (--timeout) {
		status = bl_uhci_transfer_finished(qh);
		if (!status)
			return status;
		else if (status == BL_STATUS_USB_SHOULD_CHECK_STATUS)
			break;

		bl_time_sleep(1);
	}

	return bl_uhci_transfer_status(qh);
}

static bl_status_t bl_uhci_control_transfer(struct bl_usb_host_controller *hc,
	int packet_size, int address, bl_usb_speed_t speed,
	struct bl_usb_setup_data *setup, bl_uint8_t *data, int length)
{
	struct bl_uhci_controller *uhci;
	volatile struct bl_uhci_qh *qh;
	volatile struct bl_uhci_td *td_head, *td, *td_prev;
	int toggle = 0;
	int off = 0;
	bl_usb_pid_t data_pid, status_pid;

	uhci = hc->data;

	if (!packet_size)
		packet_size = 8;

	qh = bl_uhci_alloc_qh(uhci);
	if (!qh)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* SETUP */
	td_head = td = bl_uhci_init_td(uhci, speed, BL_USB_PID_TOKEN_SETUP, address, 0,
		toggle, sizeof(struct bl_usb_setup_data), (bl_addr_t)setup);
	if (!td)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* DATA IN/OUT */
	data_pid = bl_usb_direction_pid(setup);

	while (length > 0) {
		td_prev = td;
		toggle = !toggle;

		td = bl_uhci_init_td(uhci, speed, data_pid, address, 0, toggle,
			BL_MIN(length, packet_size), (bl_addr_t)(data + off));
		if (!td)
			return BL_STATUS_USB_INTERNAL_ERROR;

		td_prev->link_pointer = (bl_addr_t)td | BL_UHCI_TD_LINK_POINTER_VF;

		length -= packet_size;
		off += packet_size;
	}

	/* STATUS */
	status_pid = bl_usb_status_pid(data_pid);

	td_prev = td;
	td = bl_uhci_init_td(uhci, speed, status_pid, address, 0, 1, 0 ,0);
	if (!td)
		return BL_STATUS_USB_INTERNAL_ERROR;

	td_prev->link_pointer = (bl_addr_t)td | BL_UHCI_TD_LINK_POINTER_VF;
	
	/* Start transfer. */
	qh->element_link_pointer = qh->original_element = (bl_addr_t)td_head;

	return bl_uhci_check_transfer_timeout(qh, 100);
}

static volatile struct bl_uhci_qh *bl_uhci_bulk_interrupt_transfers(
	struct bl_usb_host_controller *hc, int address, struct bl_usb_endpoint *endp,
	bl_usb_speed_t speed, int direction, bl_uint8_t *data, int length)
{
	struct bl_uhci_controller *uhci;
	volatile struct bl_uhci_qh *qh;
	volatile struct bl_uhci_td *td_head = NULL, *td = NULL, *td_prev;
	int packet_size, toggle;
	int off = 0;
	bl_usb_pid_t data_pid;

	uhci = hc->data;

	qh = bl_uhci_alloc_qh(uhci);
	if (!qh)
		return NULL;

	packet_size = endp->descriptor->max_packet_size;
	toggle = endp->toggle;

	/* DATA IN/OUT */
	data_pid = direction ? BL_USB_PID_TOKEN_OUT : BL_USB_PID_TOKEN_IN;

	while (length > 0) {
		td_prev = td;

		td = bl_uhci_init_td(uhci, speed, data_pid, address,
			endp->descriptor->endpoint_address, toggle,
			BL_MIN(length, packet_size), (bl_addr_t)(data + off));
		if (!td)
			return NULL;

		if (!td_head)
			td_head = td_prev = td;
		else
			td_prev->link_pointer = (bl_addr_t)td | BL_UHCI_TD_LINK_POINTER_VF;

		toggle = !toggle;
		length -= packet_size;
		off += packet_size;
	}

	endp->toggle = toggle;

	/* Start transfer. */
	qh->element_link_pointer = qh->original_element = (bl_addr_t)td_head;

	return qh;
}

static void bl_uhci_bulk_transfer(struct bl_usb_host_controller *hc,
	int address, struct bl_usb_endpoint *endp, bl_usb_speed_t speed,
	int direction, bl_uint8_t *data, int length)
{
	volatile struct bl_uhci_qh *qh;

	if (length == 0)
		return;

	qh = bl_uhci_bulk_interrupt_transfers(hc, address, endp, speed, direction,
		data, length);

	bl_uhci_check_transfer_timeout(qh, 100);
}

static bl_usb_transfer_info_t bl_uhci_interrupt_transfer(struct bl_usb_host_controller *hc,
	int address, struct bl_usb_endpoint *endp, bl_usb_speed_t speed,
	int direction, bl_uint8_t *data, int length)
{
	volatile struct bl_uhci_qh *qh;

	if (length == 0)
		return NULL;

	qh = bl_uhci_bulk_interrupt_transfers(hc, address, endp, speed, direction,
		data, length);

	bl_time_sleep(endp->descriptor->interval);

	return (bl_usb_transfer_info_t)qh;
}

static void bl_uhci_controller_uninit(struct bl_uhci_controller *uhci)
{
	if (!uhci)
		return;

	/* Next time the controller is used it should be reseted. */
	bl_outw(BL_UHCI_USBCMD_STOP, BL_UHCI_REG(uhci, BL_UHCI_IO_REG_USBCMD));

	if (uhci->framelist_ptrs) {
		bl_heap_free((void *)uhci->framelist_ptrs, 0x1000);
		uhci->framelist_ptrs = NULL;
	}

	if (uhci->qhs) {
		bl_heap_free((void *)uhci->qhs, BL_UHCI_NUM_QH * sizeof(struct bl_uhci_qh));
		uhci->qhs = NULL;
	}

	if (uhci->tds) {
		bl_heap_free((void *)uhci->tds, BL_UHCI_NUM_TD * sizeof(struct bl_uhci_td));
		uhci->tds = NULL;
	}

	bl_heap_free(uhci, sizeof(struct bl_uhci_controller));
}

static bl_status_t bl_uhci_init_data_structures(struct bl_uhci_controller *uhci)
{
	int i;

	/* Frame list pointers allocation. */
	uhci->framelist_ptrs = bl_heap_alloc_align(0x1000, 0x1000);
	if (!uhci->framelist_ptrs)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	/* Queue heads allocation. */
	uhci->qhs = bl_heap_alloc_align(BL_UHCI_NUM_QH * sizeof(struct bl_uhci_qh), 0x1000);
	if (!uhci->qhs)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_UHCI_NUM_QH - 1; i++) {
		/* QH are aligned on 16-byte boundary. */
		uhci->qhs[i].head_link_pointer = (bl_uint32_t)&uhci->qhs[i + 1] |
			BL_UHCI_QH_HEAD_LINK_POINTER_Q;

		uhci->qhs[i].element_link_pointer = BL_UHCI_QH_ELEMENT_LINK_POINTER_T;

		uhci->qhs[i].used = 0;
	}
	uhci->qhs[i].head_link_pointer = BL_UHCI_QH_HEAD_LINK_POINTER_T;

	/* Transfer descriptors allocation. */
	uhci->tds = bl_heap_alloc_align(BL_UHCI_NUM_TD * sizeof(struct bl_uhci_td), 0x1000);
	if (!uhci->tds)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_UHCI_NUM_TD; i++)
		uhci->tds[i].used = 0;

	/* No support for isochronous TDs. */
	for (i = 0; i < 1024; i++)
		uhci->framelist_ptrs[i] = (bl_uint32_t)uhci->qhs | BL_UHCI_FRAME_LIST_POINTER_Q;

	/* Set SOF timing. */
	bl_outb(64, BL_UHCI_REG(uhci, BL_UHCI_IO_REG_SOFMOD));

	/* Set frame list pointer info. */
	bl_outl((bl_uint32_t)uhci->framelist_ptrs, BL_UHCI_REG(uhci, BL_UHCI_IO_REG_FRBASEADD));
	bl_outw(0, BL_UHCI_REG(uhci, BL_UHCI_IO_REG_FRNUM));

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_uhci_controller_init(struct bl_uhci_controller *uhci)
{
	bl_status_t status;

	/* Reset the host controller */
	bl_outw(BL_UHCI_USBCMD_HCRESET, BL_UHCI_REG(uhci, BL_UHCI_IO_REG_USBCMD));
	bl_time_sleep(50);
	bl_outw(BL_UHCI_USBCMD_STOP, BL_UHCI_REG(uhci, BL_UHCI_IO_REG_USBCMD));

	/* Frame list, QHs, TDs.*/
	status = bl_uhci_init_data_structures(uhci);
	if (status)
		return status;

	/* Resume the execution of the host controller. */
	bl_outw(BL_UHCI_USBCMD_RUN, BL_UHCI_REG(uhci, BL_UHCI_IO_REG_USBCMD));

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_uhci_pci_init(struct bl_pci_index i)
{
	bl_status_t status;
	bl_uint32_t base_address;
	struct bl_uhci_controller *uhci;

	/* Check PCI device. */
	status = bl_pci_check_device_class(i, BL_PCI_BASE_CLASS_SERIAL,
		BL_PCI_SUBCLASS_SERIAL_USB_UHCI, BL_PCI_PROG_IF_SERIAL_USB_UHCI);
	if (status)
		return status;

	/* Check base address. */
	base_address = bl_pci_read_config_long(i.bus, i.dev, i.func, BL_PCI_CONFIG_REG_BAR4);

	if ((base_address & BL_UHCI_PCI_BAR4_RTE) == 0 ||
		(base_address & BL_UHCI_PCI_BAR4_BASE_ADDRESS) == 0)
		return BL_STATUS_USB_INVALID_DEVICE;

	/* Initialize the controller. */
	uhci = bl_heap_alloc(sizeof(struct bl_uhci_controller));
	if (!uhci)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	uhci->io = base_address & BL_UHCI_PCI_BAR4_BASE_ADDRESS;

	status = bl_uhci_controller_init(uhci);
	if (status) {
		bl_uhci_controller_uninit(uhci);
		return status;
	}

	bl_uhci_controller_add(uhci);

	return BL_STATUS_SUCCESS;
}

static struct bl_usb_host_controller_functions uhci_functions = {
	.port_device_connected = bl_uhci_port_device_connected,
	.port_device_get_speed = bl_uhci_port_device_get_speed,
	.port_reset = bl_uhci_port_reset,
	.control_transfer = bl_uhci_control_transfer,
	.bulk_transfer = bl_uhci_bulk_transfer,
	.interrupt_transfer = bl_uhci_interrupt_transfer,
	.check_transfer_status = bl_uhci_check_transfer_status,
};

BL_MODULE_INIT()
{
	struct bl_uhci_controller *uhci;

	bl_pci_iterate_devices(bl_uhci_pci_init);

	uhci = uhci_list;
	while (uhci) {
		struct bl_usb_host_controller *hc =
			bl_heap_alloc(sizeof(struct bl_usb_host_controller));
		if (!hc)
			return;

		hc->type = BL_USB_HC_UHCI,
		hc->funcs = &uhci_functions;
		hc->data = uhci;
		hc->root_hub.ports = 2;

		bl_usb_host_controller_register(hc);

		uhci = uhci->next;
	}
}

BL_MODULE_UNINIT()
{

}

