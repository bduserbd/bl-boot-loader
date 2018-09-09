#include "ohci.h"
#include "include/export.h"
#include "include/string.h"
#include "include/time.h"
#include "include/bl-utils.h"
#include "core/include/usb/usb.h"
#include "core/include/pci/pci.h"
#include "core/include/loader/loader.h"
#include "core/include/memory/heap.h"
#include "core/include/video/print.h"

BL_MODULE_NAME("USB OHCI");

#define BL_OHCI_NUM_CONTROL_EDS	128
#define BL_OHCI_NUM_BULK_EDS	128
#define BL_OHCI_NUM_GENERAL_TDS	512

struct bl_ohci_controller {
	/* Related registers info. */
	volatile struct bl_ohci_registers *regs;
	volatile struct bl_ohci_hcca *hcca;

	/* Separate control & bulk EDs. */
	volatile struct bl_ohci_ed *control_eds;
	volatile struct bl_ohci_ed *bulk_eds;

	/* General transfer descriptors. */
	volatile struct bl_ohci_general_td *tds;

	struct bl_ohci_controller *next;
};

static struct bl_ohci_controller *ohci_list = NULL;

static inline void bl_ohci_controller_add(struct bl_ohci_controller *ohci)
{
	ohci->next = ohci_list;
	ohci_list = ohci;
}

static int bl_ohci_port_device_connected(struct bl_usb_host_controller *hc, int port)
{
	struct bl_ohci_controller *ohci;

	ohci = hc->data;

	return !!(ohci->regs->rh_port_status[port] & BL_OHCI_REG_RH_PORT_STATUS_CCS);
}

static void bl_ohci_port_reset(struct bl_usb_host_controller *hc, int port)
{
	struct bl_ohci_controller *ohci;

	ohci = hc->data;

	/* Perform actual reset. */
	ohci->regs->rh_port_status[port] |= BL_OHCI_REG_RH_PORT_STATUS_PRS;
	bl_time_sleep(50);
	if ((ohci->regs->rh_port_status[port] & BL_OHCI_REG_RH_PORT_STATUS_PRSC) == 0)
		return;
	
	/* Clear reset status bit. */
	ohci->regs->rh_port_status[port] |= BL_OHCI_REG_RH_PORT_STATUS_PRSC;

	/* Enable the port again. */
	ohci->regs->rh_port_status[port] |= BL_OHCI_REG_RH_PORT_STATUS_PES;
	bl_time_sleep(10);
	if ((ohci->regs->rh_port_status[port] & BL_OHCI_REG_RH_PORT_STATUS_PES) == 0)
		return;

	ohci->regs->rh_port_status[port] |= BL_OHCI_REG_RH_PORT_STATUS_PESC;
}

static bl_usb_speed_t bl_ohci_port_device_get_speed(struct bl_usb_host_controller *hc,
	int port)
{
	struct bl_ohci_controller *ohci;

	ohci = hc->data;

	return ohci->regs->rh_port_status[port] & BL_OHCI_REG_RH_PORT_STATUS_LSDA ?
		BL_USB_LOW_SPEED : BL_USB_FULL_SPEED;
}

static volatile struct bl_ohci_ed *bl_ohci_alloc_ed(volatile struct bl_ohci_ed *eds,
	int count, int address_endpoint)
{
	int i;

	for (i = 0; i < count; i++) {
		if ((eds[i].flags & BL_OHCI_ED_FLAGS_ADDRESS_ENDPOINT) == address_endpoint)
			return &eds[i];

		if ((eds[i].flags & BL_OHCI_ED_FLAGS_ADDRESS_ENDPOINT) == 0) {
			if (i > 0)
				eds[i - 1].next_ed = (bl_uint32_t)&eds[i] & ~0xf;

			return &eds[i];
		}
	}

	return NULL;
}

static volatile struct bl_ohci_ed *bl_ohci_init_ed(struct bl_ohci_controller *ohci,
	bl_usb_transfer_t transfer, int address, int endpoint, bl_usb_speed_t speed,
	int max_packet_size)
{
	int address_endpoint;
	volatile struct bl_ohci_ed *ed;

	address_endpoint = BL_OHCI_ED_ADDRESS_ENDPOINT(address, endpoint);

	if (transfer == BL_USB_TRANSFER_CONTROL) {
		if (address_endpoint == 0)
			ed = &ohci->control_eds[0];
		else
			ed = bl_ohci_alloc_ed(ohci->control_eds, BL_OHCI_NUM_CONTROL_EDS,
				address_endpoint);
	} else if (transfer == BL_USB_TRANSFER_BULK) {
		ed = bl_ohci_alloc_ed(ohci->bulk_eds, BL_OHCI_NUM_BULK_EDS,
			address_endpoint);
	} else
		return NULL;

	if (!ed)
		return NULL;

	ed->flags &= ~BL_OHCI_ED_FLAGS_ADDRESS_ENDPOINT;
	ed->flags |= address_endpoint;

	ed->flags &= ~BL_OHCI_ED_FLAGS_SPEED;
	ed->flags |= speed == BL_USB_LOW_SPEED ? BL_OHCI_ED_FLAGS_SPEED /* Low speed */ :
		0 /* Full speed */;

	ed->flags &= ~BL_OHCI_ED_FLAGS_MPS;
	ed->flags |= BL_OHCI_ED_MPS(max_packet_size);

	return ed;
}

static volatile struct bl_ohci_general_td *bl_ohci_alloc_general_td(
	struct bl_ohci_controller *ohci)
{
	int i;

	for (i = 0; i < BL_OHCI_NUM_GENERAL_TDS; i++)
		if (!ohci->tds[i].used) {
			ohci->tds[i].used = 1;
			return &ohci->tds[i];
		}

	return NULL;
}

static volatile struct bl_ohci_general_td *bl_ohci_init_general_td(
	struct bl_ohci_controller *ohci, u8 *buf, int length, int toggle,
	bl_usb_pid_t pid)
{
	volatile struct bl_ohci_general_td *td;

	td = bl_ohci_alloc_general_td(ohci);
	if (!td)
		return NULL;

	td->flags = 0;

	switch (pid) {
	case BL_USB_PID_TOKEN_SETUP:
		td->flags |= BL_OHCI_TD_FLAGS_DP_SETUP;
		break;

	case BL_USB_PID_TOKEN_OUT:
		td->flags |= BL_OHCI_TD_FLAGS_DP_OUT;
		break;

	case BL_USB_PID_TOKEN_IN:
		td->flags |= BL_OHCI_TD_FLAGS_DP_IN;
		break;
	}

	/* Error code. */
	td->flags |= BL_OHCI_TD_FLAGS_CC(BL_OHCI_CC_NOT_ACCESSED);

	/* Toggle bit inside TD. */
	td->flags |= BL_OHCI_TD_FLAGS_T(toggle);

	/* Error count - 3 times. */
	td->flags |= BL_OHCI_TD_FLAGS_EC(3);

	if (buf && length > 0) {
		td->current_buffer_pointer = (bl_uint32_t)buf;
		td->buffer_end = (bl_uint32_t)&buf[length - 1];
	} else
		td->current_buffer_pointer = td->buffer_end = 0;

	td->next_td = 0;

	return td;
}

static void bl_ohci_free_td_list(volatile struct bl_ohci_ed *ed)
{
	volatile struct bl_ohci_general_td *td_head, *td;
	bl_uint32_t next_td;

	td_head = td = BL_OHCI_ED_TD_POINTER(ed->td_head_pointer);
	do {
		next_td = td->original_next_td;
		bl_memset((void *)td, 0, sizeof(struct bl_ohci_general_td));

		td = BL_OHCI_ED_TD_POINTER(next_td);
	} while (td != td_head);
}

static bl_status_t bl_ohci_transfer_status(volatile struct bl_ohci_ed *ed)
{
	bl_status_t status;
	volatile struct bl_ohci_general_td *td;

	td = BL_OHCI_ED_TD_POINTER(ed->td_head_pointer);
	switch (BL_OHCI_TD_FLAGS_TO_CC(td->flags)) {
	case BL_OHCI_CC_NO_ERROR:
		status = BL_STATUS_SUCCESS;
		break;

	case BL_OHCI_CC_CRC:
		status = BL_STATUS_USB_CRC_ERROR;
		break;

	case BL_OHCI_CC_BIT_STUFFING:
		status = BL_STATUS_USB_BITSTUFF_ERROR;
		break;

	case BL_OHCI_CC_DATA_TOGGLE_MISMATCH:
		status = BL_STATUS_USB_CRC_ERROR;
		break;

	case BL_OHCI_CC_STALL:
		status = BL_STATUS_USB_STALLED;
		break;

	case BL_OHCI_CC_DEVICE_NOT_RESPONDING:
		status = BL_STATUS_USB_TIMEOUT_ERROR;
		break;

	case BL_OHCI_CC_PID_CHECK_FAILURE:
		status = BL_STATUS_USB_CRC_ERROR;
		break;

	case BL_OHCI_CC_UNEXPECTED_PID:
		status = BL_STATUS_USB_BABBLE_DETECTED;
		break;

	case BL_OHCI_CC_DATA_OVERRUN:
	case BL_OHCI_CC_DATA_UNDERRUN:
	case BL_OHCI_CC_BUFFER_OVERRUN:
	case BL_OHCI_CC_BUFFER_UNDERRUN:
		status = BL_STATUS_USB_DATA_BUFFER_ERROR;
		break;

	default:
		status = BL_STATUS_USB_NAK_RECEIVED;
		break;
	}

	return status;
}

static bl_status_t bl_ohci_transfer_finished(volatile struct bl_ohci_ed *ed)
{
	if (ed->td_head_pointer & BL_OHCI_ED_TD_HEAD_POINTER_HALTED)
		return BL_STATUS_USB_SHOULD_CHECK_STATUS;

	if (BL_OHCI_ED_TD_POINTER(ed->td_head_pointer) ==
		BL_OHCI_ED_TD_POINTER(ed->td_tail_pointer)) {
		ed->flags |= BL_OHCI_ED_FLAGS_SKIP;
		bl_ohci_free_td_list(ed);

		return BL_STATUS_SUCCESS;
	}

	return BL_STATUS_USB_ACTION_NOT_FINISHED;
}

#if 0
static bl_status_t bl_ohci_check_transfer_status(bl_usb_transfer_info_t info)
{
	bl_status_t status;
	volatile struct bl_ohci_ed *ed;

	ed = info;

	status = bl_ohci_transfer_finished(ed);
	if (status == BL_STATUS_USB_SHOULD_CHECK_STATUS)
		return bl_ohci_transfer_status(ed);

	return status;
}
#endif

static bl_status_t bl_ohci_check_transfer_timeout(volatile struct bl_ohci_ed *ed,
	int timeout)
{
	bl_status_t status;

	while (--timeout) {
		status = bl_ohci_transfer_finished(ed);
		if (!status)
			return status;
		else if (status == BL_STATUS_USB_SHOULD_CHECK_STATUS)
			break;

		bl_time_sleep(1);
	}

	return bl_ohci_transfer_status(ed);
}

static bl_status_t bl_ohci_control_transfer(struct bl_usb_host_controller *hc,
	int packet_size, int address, bl_usb_speed_t speed,
	struct bl_usb_setup_data *setup, bl_uint8_t *data, int length)
{
	struct bl_ohci_controller *ohci;
	volatile struct bl_ohci_ed *ed;
	volatile struct bl_ohci_general_td *td_head, *td, *td_prev;
	int toggle = 0;
	int off = 0;
	bl_usb_pid_t data_pid, status_pid;

	ohci = hc->data;

	if (!packet_size)
		packet_size = 8;

	ed = bl_ohci_init_ed(ohci, BL_USB_TRANSFER_CONTROL, address, 0, speed, packet_size);
	if (!ed)
		return BL_STATUS_USB_INTERNAL_ERROR;

	data_pid = bl_usb_direction_pid(setup);
	status_pid = bl_usb_status_pid(data_pid);

	/* SETUP */
	td_head = td_prev = td = bl_ohci_init_general_td(ohci, (u8 *)setup,
		sizeof(struct bl_usb_setup_data), toggle, BL_USB_PID_TOKEN_SETUP);
	if (!td)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* DATA IN/OUT */
	if (length == 0) {
		td_prev = td;
		toggle = !toggle;

		/* TODO: Works for requests like SET_ADDRESS.  Does this always work ? */
		td = bl_ohci_init_general_td(ohci, NULL, 0, toggle, status_pid);
		if (!td)
			return BL_STATUS_USB_INTERNAL_ERROR;

		td_prev->original_next_td = td_prev->next_td = (bl_uint32_t)td;
	} else
		while (length > 0) {
			td_prev = td;
			toggle = !toggle;

			td = bl_ohci_init_general_td(ohci, data + off, BL_MIN(length,
				packet_size), toggle, data_pid);
			if (!td)
				return BL_STATUS_USB_INTERNAL_ERROR;

			td_prev->original_next_td = td_prev->next_td = (bl_uint32_t)td;
			length -= packet_size;
			off += packet_size;
		}

	/* STATUS */
	td_prev = td;

	td = bl_ohci_init_general_td(ohci, NULL, 0, 1, status_pid);
	if (!td)
		return BL_STATUS_USB_INTERNAL_ERROR;

	td_prev->original_next_td = td_prev->next_td = (bl_uint32_t)td;
	td->next_td = 0;
	td->original_next_td = (bl_uint32_t)td_head;

	/* Link transfer. */
	ed->td_head_pointer = (bl_uint32_t)td_head;
	ed->td_tail_pointer = (bl_uint32_t)td;

	/* Remove skip flag. */
	ed->flags &= ~BL_OHCI_ED_FLAGS_SKIP;

	/* Actual start of control transfer. */
	ohci->regs->command_status |= BL_OHCI_REG_COMMAND_STATUS_CLF;

	return bl_ohci_check_transfer_timeout(ed, 100);
}

static volatile struct bl_ohci_ed *bl_ohci_bulk_interrupt_prepare(
	struct bl_ohci_controller *ohci, int address, struct bl_usb_endpoint *endp,
	bl_usb_speed_t speed, int direction, bl_uint8_t *data, int length)
{
	volatile struct bl_ohci_ed *ed;
	volatile struct bl_ohci_general_td *td_head = NULL, *td = NULL, *td_prev;
	int packet_size, toggle;
	int off = 0;
	bl_usb_pid_t data_pid;

	packet_size = endp->descriptor->max_packet_size;
	toggle = endp->toggle;

	ed = bl_ohci_init_ed(ohci, BL_USB_TRANSFER_BULK, address,
		endp->descriptor->endpoint_address & 0xf, speed, packet_size);
	if (!ed)
		return NULL;

	/* DATA IN/OUT */
	data_pid = direction ? BL_USB_PID_TOKEN_OUT : BL_USB_PID_TOKEN_IN;

	while (length > 0) {
		td_prev = td;

		td = bl_ohci_init_general_td(ohci, data + off, BL_MIN(length,
			packet_size), toggle, data_pid);
		if (!td)
			return NULL;

		if (!td_head)
			td_head = td_prev = td;
		else
			td_prev->original_next_td = td_prev->next_td = (__u32)td;

		toggle = !toggle;
		length -= packet_size;
		off += packet_size;
	}

	/* Final node (head and tail TD pointers can't be the same, in a case
	   of one DATA IN/OUT TD). */
	td_prev = td;
	toggle = !toggle;

	td = bl_ohci_init_general_td(ohci, NULL, 0, toggle, direction ?
		BL_USB_PID_TOKEN_IN : BL_USB_PID_TOKEN_OUT);
	if (!td)
		return NULL;

	td_prev->original_next_td = td_prev->next_td = (__u32)td;
	td->next_td = 0;
	td->original_next_td = (bl_uint32_t)td_head;

	/* Save toggle bit. */
	endp->toggle = toggle;

	/* Link transfer. */
	ed->td_head_pointer = (__u32)td_head;
	ed->td_tail_pointer = (__u32)td;

	/* Remove skip flag. */
	ed->flags &= ~BL_OHCI_ED_FLAGS_SKIP;

	return ed;
}

static void bl_ohci_bulk_transfer(struct bl_usb_host_controller *hc,
	int address, struct bl_usb_endpoint *endp, bl_usb_speed_t speed,
	int direction, bl_uint8_t *data, int length)
{
	struct bl_ohci_controller *ohci;
	volatile struct bl_ohci_ed *ed;

	if (length == 0)
		return;

	ohci = hc->data;

	ed = bl_ohci_bulk_interrupt_prepare(ohci, address, endp, speed, direction,
		data, length);

	/* Actual start of bulk transfer. */
	ohci->regs->command_status |= BL_OHCI_REG_COMMAND_STATUS_BLF;

	bl_ohci_check_transfer_timeout(ed, 100);
}

#if 0
static bl_usb_transfer_info_t bl_ohci_interrupt_transfer(
	struct bl_usb_host_controller *hc, int address, struct bl_usb_endpoint *endp,
	bl_usb_speed_t speed, int direction, bl_uint8_t *data, int length)
{
	struct bl_ohci_controller *ohci;
	volatile struct bl_ohci_ed *ed;

	if (length == 0)
		return NULL;

	ohci = hc->data;

	ed = bl_ohci_bulk_interrupt_prepare(ohci, address, endp, speed, direction,
		data, length);

	return (bl_usb_transfer_info_t)ed;
}
#endif

static void bl_ohci_controller_uninit(struct bl_ohci_controller *ohci)
{
	bl_heap_free(ohci, sizeof(struct bl_ohci_controller));
}

static bl_status_t bl_ohci_init_data_structures(struct bl_ohci_controller *ohci)
{
	int i;

	/* HCCA Block. */
	ohci->hcca = bl_heap_alloc_align(sizeof(struct bl_ohci_hcca), 256);
	if (!ohci->hcca)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	ohci->hcca->frame_number = 0;
	ohci->hcca->done_head = 0;

	/* Allocate general transfer descriptors. */
	ohci->tds = bl_heap_alloc_align(sizeof(struct bl_ohci_general_td) *
		BL_OHCI_NUM_GENERAL_TDS, 16);
	if (!ohci->tds)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_OHCI_NUM_GENERAL_TDS; i++)
		bl_memset((void *)&ohci->tds[i], 0, sizeof(struct bl_ohci_general_td));

	/* Initialize contol endpoints. */
	ohci->control_eds = bl_heap_alloc_align(sizeof(struct bl_ohci_ed) *
		BL_OHCI_NUM_CONTROL_EDS, 16);
	if (!ohci->control_eds)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_OHCI_NUM_CONTROL_EDS; i++) {
		ohci->control_eds[i].flags = BL_OHCI_ED_FLAGS_SKIP;
		ohci->control_eds[i].td_head_pointer = ohci->control_eds[i].td_tail_pointer = 0;
		ohci->control_eds[i].next_ed = 0;
	}

	/* Initialize bulk endpoints. */
	ohci->bulk_eds = bl_heap_alloc_align(sizeof(struct bl_ohci_ed) *
		BL_OHCI_NUM_BULK_EDS, 16);
	if (!ohci->bulk_eds)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_OHCI_NUM_BULK_EDS; i++) {
		ohci->bulk_eds[i].flags = BL_OHCI_ED_FLAGS_SKIP; 
		ohci->control_eds[i].td_head_pointer = ohci->control_eds[i].td_tail_pointer = 0;
		ohci->control_eds[i].next_ed = 0;
	}

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ohci_controller_init(struct bl_ohci_controller *ohci)
{
	bl_status_t status;

	/* Check current revision. */
	if ((ohci->regs->revision & BL_OHCI_REG_REVISION_REV) != 0x10)
		return BL_STATUS_USB_INVALID_DEVICE;

	/* Maximum number of root hub ports. */
	if ((ohci->regs->rh_descriptor_a & BL_OHCI_REG_RH_DESCRIPTOR_A_NDP) > 0xf)
		return BL_STATUS_USB_INVALID_DEVICE;

	/* Ownership. */
	if (ohci->regs->control & BL_OHCI_REG_CONTROL_IR) {
		/* Owned by SMM. Change ownership. */
		ohci->regs->command_status |= BL_OHCI_REG_COMMAND_STATUS_OCR;

		// TODO : How much time to wait ?
		int timeout = 10;
		while (--timeout && ohci->regs->control & BL_OHCI_REG_CONTROL_IR)
			bl_time_sleep(1);

		if (ohci->regs->control & BL_OHCI_REG_CONTROL_IR)
			return BL_STATUS_USB_INTERNAL_ERROR;
	} else {
		switch (ohci->regs->control & BL_OHCI_REG_CONTROL_HCFS) {
		/* Cold boot - USBRESET. */
		case BL_OHCI_HCFS_USB_RESET:
			// TODO : How much time to wait ?
			bl_time_sleep(10);

			break;

		/* Warm boot. */
		case BL_OHCI_HCFS_USB_OPERATIONAL: /* USBOPERATIONAL - Continue. */
			break;

		default: /* Change to USBRESUME. */
			ohci->regs->control &= ~BL_OHCI_REG_CONTROL_HCFS;
			ohci->regs->control |= BL_OHCI_HCFS_USB_RESUME;

			// TODO : How much time to wait ?
			bl_time_sleep(10);

			break;
		}
	}

	/* Set HCCA, EDs, TDs .. */
	status = bl_ohci_init_data_structures(ohci);
	if (status)
		return status;

	/* Software reset. */
	bl_uint32_t fm_interval = ohci->regs->fm_interval;
	ohci->regs->command_status |= BL_OHCI_REG_COMMAND_STATUS_HCR;
	bl_time_sleep(1); // 10 micro seconds.
	ohci->regs->fm_interval = fm_interval;

	/* Initialize memory registers. */
	ohci->regs->control_head_ed = (bl_uint32_t)ohci->control_eds;
	ohci->regs->control_current_ed = 0;

	ohci->regs->bulk_head_ed = (bl_uint32_t)ohci->bulk_eds;
	ohci->regs->bulk_current_ed = 0;

	ohci->regs->hcca = (bl_uint32_t)ohci->hcca;

	/* Enable relevant interrupts. */
	ohci->regs->interrupt_disable |= BL_OHCI_REG_INTERRUPT_DISABLE_MIE;

	ohci->regs->interrupt_enable |= BL_OHCI_REG_INTERRUPT_ENABLE_MIE |
		BL_OHCI_REG_INTERRUPT_ENABLE_SO | BL_OHCI_REG_INTERRUPT_ENABLE_WDH |
		BL_OHCI_REG_INTERRUPT_ENABLE_RD | BL_OHCI_REG_INTERRUPT_ENABLE_UE |
		BL_OHCI_REG_INTERRUPT_ENABLE_FNO | BL_OHCI_REG_INTERRUPT_ENABLE_RHSC |
		BL_OHCI_REG_INTERRUPT_ENABLE_OC;

	ohci->regs->interrupt_status &= ~(BL_OHCI_REG_INTERRUPT_STATUS_SO |
		BL_OHCI_REG_INTERRUPT_STATUS_WDH | BL_OHCI_REG_INTERRUPT_STATUS_RD |
		BL_OHCI_REG_INTERRUPT_STATUS_UE | BL_OHCI_REG_INTERRUPT_STATUS_FNO |
		BL_OHCI_REG_INTERRUPT_STATUS_RHSC | BL_OHCI_REG_INTERRUPT_STATUS_OC);

	/* List processing. */
	ohci->regs->control |= BL_OHCI_REG_CONTROL_CLE | BL_OHCI_REG_CONTROL_BLE;
	ohci->regs->periodic_start = 9 * (ohci->regs->fm_interval &
		BL_OHCI_REG_FM_INTERVAL_FI) / 10;

	/* Set USBOPERATIONAL. */
	ohci->regs->control &= ~BL_OHCI_REG_CONTROL_HCFS;
	ohci->regs->control |= BL_OHCI_HCFS_USB_OPERATIONAL;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ohci_pci_init(struct bl_pci_index i)
{
	bl_status_t status;
	bl_uint32_t base_address;
	struct bl_ohci_controller *ohci;

	/* Check PCI device. */
	status = bl_pci_check_device_class(i, BL_PCI_BASE_CLASS_SERIAL,
		BL_PCI_SUBCLASS_SERIAL_USB_OHCI, BL_PCI_PROG_IF_SERIAL_USB_OHCI);
	if (status)
		return status;

	/* Mapped memory base. */
	base_address = bl_pci_read_config_long(i.bus, i.dev, i.func, BL_PCI_CONFIG_REG_BAR0);

	if ((base_address & BL_OHCI_PCI_BAR0_INDICATPR) ||
		(base_address & BL_OHCI_PCI_BAR0_BASE_ADDRESS) == 0) 
		return BL_STATUS_USB_INVALID_DEVICE;

	/* Initialize the controller. */
	ohci = bl_heap_alloc(sizeof(struct bl_ohci_controller));
	if (!ohci)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	ohci->regs = (void *)(base_address & BL_OHCI_PCI_BAR0_BASE_ADDRESS);

	status = bl_ohci_controller_init(ohci);
	if (status) {
		bl_ohci_controller_uninit(ohci);
		return status;
	}

	bl_ohci_controller_add(ohci);

	return BL_STATUS_SUCCESS;
}

static struct bl_usb_host_controller_functions ohci_functions = {
	.port_device_connected = bl_ohci_port_device_connected,
	.port_reset = bl_ohci_port_reset,
	.port_device_get_speed = bl_ohci_port_device_get_speed,
	.control_transfer = bl_ohci_control_transfer,
	.bulk_transfer = bl_ohci_bulk_transfer,
	//.interrupt_transfer = bl_ohci_interrupt_transfer,
	//.check_transfer_status = bl_ohci_check_transfer_status,
};

BL_MODULE_INIT()
{
	struct bl_ohci_controller *ohci;

	bl_pci_iterate_devices(bl_ohci_pci_init);

	ohci = ohci_list;
	while (ohci) {
		struct bl_usb_host_controller *hc =
			bl_heap_alloc(sizeof(struct bl_usb_host_controller));
		if (!hc)
			return;

		hc->type = BL_USB_HC_OHCI,
		hc->funcs = &ohci_functions;
		hc->data = (void *)ohci;
		hc->root_hub.ports = ohci->regs->rh_descriptor_a & BL_OHCI_REG_RH_DESCRIPTOR_A_NDP;

		bl_usb_host_controller_register(hc);

		ohci = ohci->next;
	}
}

BL_MODULE_UNINIT()
{

}

