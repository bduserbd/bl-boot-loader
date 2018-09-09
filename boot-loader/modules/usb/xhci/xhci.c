#include "xhci.h"
#include "include/string.h"
#include "include/time.h"
#include "include/bl-utils.h"
#include "core/include/usb/usb.h"
#include "core/include/pci/pci.h"
#include "core/include/loader/loader.h"
#include "core/include/memory/heap.h"
#include "core/include/video/print.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"

BL_MODULE_NAME("USB XHCI");

#define BL_XHCI_TR_TRBS	128

#define BL_XHCI_CR_TRBS	128

#define BL_XHCI_ER_TRBS	32

struct bl_xhci_device {
	int slot_id;

	volatile union bl_xhci_trb *tr_enqueue[31], *tr[31];

	volatile struct bl_xhci_input_context *ic;

	volatile struct bl_xhci_device_context *dc;
};

struct bl_xhci_controller {
	/* Capability registers. */
	volatile struct bl_xhci_capability_registers *cap_regs;

	/* Operational registers & port info. */
	volatile struct bl_xhci_operational_registers *op_regs;
	volatile struct bl_xhci_port_register_set *ports;

	bl_uint8_t *port_usb_type;

	/* Device Context Base Address Array. */
	bl_uint64_t *dcbaa;

	/* Command ring of TRBs. */
	volatile union bl_xhci_trb *command_ring;
	volatile union bl_xhci_trb *cr_enqueue, *cr_dequeue;

	/* ERST - Only one. */
	volatile struct bl_xhci_erst *erst;

	/* Event ring of TRBs. */
	volatile union bl_xhci_trb *event_ring;
	volatile union bl_xhci_trb *er_dequeue;

	/* Runtime registers. */
	volatile struct bl_xhci_runtime_registers *rt_regs;

	/* Doorbell registers. */
	volatile bl_uint32_t *db_regs;

	/* Interrupter. */
	volatile struct bl_xhci_interrupter_register_set *interrupter;

	/* Devices. */
	struct bl_xhci_device **devices;

	struct bl_xhci_controller *next;
};

static struct bl_xhci_controller *bl_xhci_list = NULL;

static inline void bl_xhci_controller_add(struct bl_xhci_controller *xhci)
{
	xhci->next = bl_xhci_list;
	bl_xhci_list = xhci;
}

static void foo(volatile union bl_xhci_trb *trb)
{
	bl_print_hex(trb);
	bl_print_str(" ");
	bl_print_hex64(trb->template.parameter);
	bl_print_str(" ");
	bl_print_hex(trb->template.status);
	bl_print_str(" ");
	bl_print_hex(trb->template.control);
	bl_print_str("\n");
}

static volatile union bl_xhci_trb *bl_xhci_get_last_event(struct bl_xhci_controller *xhci)
{
	volatile union bl_xhci_trb *trb;

	if (xhci->er_dequeue->template.control & BL_XHCI_TRB_C) {
		trb = xhci->er_dequeue;

		// TODO: Handle the end of the queue.
		xhci->er_dequeue++;

		return trb;
	}

	return NULL;
}

static volatile union bl_xhci_trb *bl_xhci_cr_queue_trb(struct bl_xhci_controller *xhci,
	bl_uint64_t parameter, bl_uint32_t status, bl_uint32_t control)
{
	volatile union bl_xhci_trb *trb, *event;

	trb = xhci->cr_enqueue;

	trb->template.parameter = parameter;
	trb->template.status = status;
	trb->template.control = control;

	xhci->db_regs[0] = 0x0;

	int timeout = 20;
	while (--timeout) {
		event = bl_xhci_get_last_event(xhci);
		if (event) {
			foo(event);
			break;
		}

		bl_time_sleep(1);
	}

	if (!event)
		return NULL;

	// TODO: Handle the end of the queue.
	xhci->cr_enqueue++;

	return event;
}

static int bl_xhci_cr_enable_slot(struct bl_xhci_controller *xhci)
{
	volatile union bl_xhci_trb *event;

	// TODO: What about protocol_slot_type ?
	event = bl_xhci_cr_queue_trb(xhci, 0, 0, BL_XHCI_TRB_TYPE(BL_XHCI_CR_TRB_ENABLE_SLOT) |
					BL_XHCI_TRB_C);
	if (!event)
		return -1;

	if (BL_XHCI_TRB_CC(event->template.status) != BL_XHCI_TRB_CC_SUCCESS)
		return -1;

	return BL_XHCI_TRB_SLOT_ID(event->template.control);
}

static void bl_xhci_cr_address_device(struct bl_xhci_controller *xhci,
	volatile struct bl_xhci_input_context *ic, int slot_id)
{
	volatile union bl_xhci_trb *event;

	event = bl_xhci_cr_queue_trb(xhci, (bl_uint64_t)ic & BL_XHCI_TRB_INPUT_CONTEXT_PTR_RSVD,
		0, BL_XHCI_TRB_SET_SLOT_ID(slot_id) | BL_XHCI_TRB_TYPE(BL_XHCI_CR_TRB_ADDRESS_DEVICE) |
		BL_XHCI_TRB_C);
	if (!event)
		return;

	if (BL_XHCI_TRB_CC(event->template.status) != BL_XHCI_TRB_CC_SUCCESS)
		return;
}

static volatile union bl_xhci_trb *bl_xhci_alloc_transfer_ring(void)
{
	int i;
	volatile union bl_xhci_trb *tr;

	/* Command Ring Dequeue Pointer. */
	tr = bl_heap_alloc_align(BL_XHCI_TR_TRBS * sizeof(union bl_xhci_trb), 0x10);
	if (!tr)
		return NULL;

	for (i = 0; i < BL_XHCI_TR_TRBS; i++)
		bl_memset((void *)&tr[i], 0, sizeof(union bl_xhci_trb));

	/* Last element, Link TRB. */
	tr[BL_XHCI_TR_TRBS - 1].link.ring_segment_pointer = (bl_uint64_t)&tr[0];

	tr[BL_XHCI_TR_TRBS - 1].link.control |= BL_XHCI_TRB_TYPE(BL_XHCI_TR_TRB_LINK);

	return tr;
}

static void bl_xhci_device_slot_init(struct bl_xhci_controller *xhci, int port, int slot_id,
	bl_usb_speed_t speed, int address)
{
	volatile struct bl_xhci_input_context *ic;
	volatile union bl_xhci_trb *tr;
	volatile struct bl_xhci_device_context *dc;

	/* 4.3.3 Device Slot Initialization. */

	/* 1. Allocate Input Context and set to 0. */
	ic = bl_heap_alloc_align(sizeof(struct bl_xhci_input_context), 0x10);
	if (!ic)
		return;

	bl_memset((void *)ic, 0, sizeof(struct bl_xhci_input_context));

	/* 2. Set A0 & A1 so that Slot Context & Endpoint 0 Context are affected. */
	ic->icc.add_context_flags |= BL_XHCI_INPUT_CONTROL_CONTEXT_A(0) |
					BL_XHCI_INPUT_CONTROL_CONTEXT_A(1);

	/* 3. Initialize Slot Context. */
	ic->slot.root_hub_port_number = port + 1;

	//TODO Route String.

	ic->slot.context_entries = 1;

	/* 4. Initialize the Transfer Ring for the Default Control Endpoint. */
	tr = bl_xhci_alloc_transfer_ring();
	if (!tr)
		return;

	/* 5. Initialize Endpoint 0 Context. */
	ic->ep_context0.ep_type = BL_XHCI_EP_TYPE_CONTROL;

	switch (speed) {
		case BL_USB_LOW_SPEED:
		case BL_USB_FULL_SPEED:
			ic->ep_context0.max_packet_size = 8;
			break;

		case BL_USB_HIGH_SPEED:
			ic->ep_context0.max_packet_size = 64;
			break;

		case BL_USB_SUPER_SPEED:
		case BL_USB_SUPER_SPEED_PLUS:
			ic->ep_context0.max_packet_size = 512;
			break;
	};

	ic->ep_context0.max_burst_size = 0;
	ic->ep_context0.tr_dequeue_pointer = (bl_uint64_t)tr | 1 /* DCS */;
	ic->ep_context0.interval = 0;
	ic->ep_context0.max_pstreams = 0;
	ic->ep_context0.mult = 0;
	ic->ep_context0.cerr = 3;

	/* 6. Initialize Output Device Context data structure to 0. */
	dc = bl_heap_alloc_align(sizeof(struct bl_xhci_device_context), 0x40);
	if (!dc)
		return;

	bl_memset((void *)dc, 0, sizeof(struct bl_xhci_device_context));

	/* 7. Load the Output Device Context in the given slot. */
	xhci->dcbaa[slot_id] = (bl_uint64_t)dc;

	/* 8. Address Device Command. */
	ic->slot.usb_device_address = address;

	bl_xhci_cr_address_device(xhci, ic, slot_id);

	/* Save our data. */
	xhci->devices[address] = bl_heap_alloc(sizeof(struct bl_xhci_device));
	if (!xhci->devices[address])
		return;

	xhci->devices[address]->slot_id = slot_id;
	xhci->devices[address]->tr_enqueue[0] = xhci->devices[address]->tr[0] = tr;
	xhci->devices[address]->ic = ic;
	xhci->devices[address]->dc = dc;
}

static bl_usb_speed_t bl_xhci_port_init(struct bl_usb_host_controller *hc, int port, int address)
{
	int slot_id;
	bl_usb_speed_t speed = -1;
	struct bl_xhci_controller *xhci;
	volatile union bl_xhci_trb *event;

	xhci = hc->data;

	bl_print_hex(port + 1);
	bl_print_str("\n");

	if (xhci->port_usb_type[port] == 0x3) {
		/* USB 3.x should automatically advance to the Enabled state. */
		bl_print_str("Wow1\n");
		if (!(xhci->ports[port].portsc & BL_XHCI_PORTSC_PED) ||
			(xhci->ports[port].portsc & BL_XHCI_PORTSC_PR) ||
			BL_XHCI_PORTSC_PLS(xhci->ports[port].portsc) != BL_XHCI_PORTSC_PLS_U0)
			return -1;
		bl_print_str("Wow2\n");

		switch (BL_XHCI_PORTSC_PORT_SPEED(xhci->ports[port].portsc)) {
		case BL_XHCI_USB_SUPER_SPEED:
			speed = BL_USB_SUPER_SPEED;
			break;

		case BL_XHCI_USB_SUPER_SPEED_PLUS:
			speed = BL_USB_SUPER_SPEED_PLUS;
			break;

		default:
			return -1;
		}
	} else if (xhci->port_usb_type[port] <= 0x2) {
		xhci->ports[port].portsc |= BL_XHCI_PORTSC_PR;
		bl_time_sleep(50);

		event = bl_xhci_get_last_event(xhci);
		if (event) {
			foo(event);
		} else
			return -1;

		if (!(xhci->ports[port].portsc & BL_XHCI_PORTSC_PRC) ||
			!(xhci->ports[port].portsc & BL_XHCI_PORTSC_PED) ||
			BL_XHCI_PORTSC_PLS(xhci->ports[port].portsc) != BL_XHCI_PORTSC_PLS_U0 ||
			BL_XHCI_TRB_CC(event->template.status) != BL_XHCI_TRB_CC_SUCCESS ||
			BL_XHCI_TRB_PORT_ID(event->template.parameter) != port + 1)
			return -1;

		xhci->ports[port].portsc |= BL_XHCI_PORTSC_PRC;

		switch (BL_XHCI_PORTSC_PORT_SPEED(xhci->ports[port].portsc)) {
		case BL_XHCI_USB_LOW_SPEED:
			speed = BL_USB_LOW_SPEED;
			break;

		case BL_XHCI_USB_FULL_SPEED:
			speed = BL_USB_FULL_SPEED;
			break;

		case BL_XHCI_USB_HIGH_SPEED:
			speed = BL_USB_HIGH_SPEED;
			break;

		default:
			return -1;
		}
	}

	slot_id = bl_xhci_cr_enable_slot(xhci);
	if (slot_id == -1)
		return -1;

	bl_xhci_device_slot_init(xhci, port, slot_id, speed, address);

	return speed;
}

static int bl_xhci_port_device_connected(struct bl_usb_host_controller *hc, int port)
{
	int result;
	struct bl_xhci_controller *xhci;
	volatile union bl_xhci_trb *event;

	xhci = hc->data;

	if (xhci->ports[port].portsc & BL_XHCI_PORTSC_CSC)
		xhci->ports[port].portsc |= BL_XHCI_PORTSC_CSC;

	result = !!(xhci->ports[port].portsc & BL_XHCI_PORTSC_CCS);
	if (result) {
		event = bl_xhci_get_last_event(xhci);
		if (event) {
			if (BL_XHCI_TRB_CC(event->template.status) != BL_XHCI_TRB_CC_SUCCESS)
				return 0;

			if (BL_XHCI_TRB_PORT_ID(event->template.parameter) != port + 1)
				return 0;
		}
		foo(event);
	}

	return result;
}

static bl_status_t bl_xhci_control_transfer(struct bl_usb_host_controller *hc,
	int packet_size, int address, bl_usb_speed_t speed,
	struct bl_usb_setup_data *setup, bl_uint8_t *data, int length)
{
	int off = 0;
	int max_packet_size;
	struct bl_xhci_controller *xhci;
	volatile union bl_xhci_trb *trb, *event;
	union bl_xhci_trb td1, td2, td3;

	xhci = hc->data;

	max_packet_size = xhci->devices[address]->ic->ep_context0.max_packet_size;

	if (!packet_size)
		packet_size = max_packet_size;
	else {
		if (packet_size != max_packet_size) {
			// TODO: Evaluate Context Command.
		}
	}

	/* Setup Stage TD. */
	bl_memset(&td1, 0, sizeof(union bl_xhci_trb));

	td1.setup.trb_type = BL_XHCI_TR_TRB_SETUP;
	td1.setup.trt = BL_XHCI_CONTROL_TRT_IN_DATA_STAGE;
	td1.setup.transfer_length = sizeof(struct bl_usb_setup_data);
	td1.setup.ioc = 0;
	td1.setup.idt = 1;
	bl_memcpy(&td1.setup.data, setup, sizeof(struct bl_usb_setup_data));
	td1.setup.c = 1;

	/* Advance Endpoint 0 Transfer Ring Enqueue Pointer. */
	trb = xhci->devices[address]->tr_enqueue[0];

	trb->template.parameter = td1.template.parameter;
	trb->template.status = td1.template.status;
	trb->template.control = td1.template.control;

	xhci->devices[address]->tr_enqueue[0]++;

	/* Data Stage TD. */
	while (length > 0) {
		bl_memset(&td2, 0, sizeof(union bl_xhci_trb));

		td2.data.trb_type = BL_XHCI_TR_TRB_DATA;
		td2.data.dir = 1;
		td2.data.transfer_length = BL_MIN(length, packet_size);
		td2.data.ch = 0;
		td2.data.ioc = 0;
		td2.data.idt = 0;
		td2.data.data_buffer = (bl_uint64_t)data + off;
		td2.data.c = 1;

		/* Advance Endpoint 0 Transfer Ring Enqueue Pointer. */
		trb = xhci->devices[address]->tr_enqueue[0];

		trb->template.parameter = td2.template.parameter;
		trb->template.status = td2.template.status;
		trb->template.control = td2.template.control;

		xhci->devices[address]->tr_enqueue[0]++;

		length -= packet_size;
		off += packet_size;
	}

	/* Status Stage TD. */
	bl_memset(&td3, 0, sizeof(union bl_xhci_trb));

	td3.status.trb_type = BL_XHCI_TR_TRB_STATUS;
	td3.status.dir = 0;
	td3.status.ch = 0;
	td3.status.ioc = 1;
	td3.status.c = 1;

	/* Advance Endpoint 0 Transfer Ring Enqueue Pointer. */
	trb = xhci->devices[address]->tr_enqueue[0];

	trb->template.parameter = td3.template.parameter;
	trb->template.status = td3.template.status;
	trb->template.control = td3.template.control;

	xhci->devices[address]->tr_enqueue[0]++;

	/* Doorbell ring. */
	xhci->db_regs[xhci->devices[address]->slot_id] = 1;

	int timeout = 20;
	while (--timeout) {
		event = bl_xhci_get_last_event(xhci);
		if (event) {
			foo(event);
			break;
		}

		bl_time_sleep(1);
	}

	return BL_STATUS_SUCCESS;
}

static void bl_xhci_cr_configure_endpoint(struct bl_xhci_controller *xhci,
	volatile struct bl_xhci_input_context *ic, int slot_id)
{
	volatile union bl_xhci_trb *event;

	event = bl_xhci_cr_queue_trb(xhci, (bl_uint64_t)ic & BL_XHCI_TRB_INPUT_CONTEXT_PTR_RSVD,
		0, BL_XHCI_TRB_SET_SLOT_ID(slot_id) | BL_XHCI_TRB_TYPE(BL_XHCI_CR_TRB_CONFIGURE_ENDPOINT) |
		BL_XHCI_TRB_C);
	if (!event)
		return;

	if (BL_XHCI_TRB_CC(event->template.status) != BL_XHCI_TRB_CC_SUCCESS)
		return;
}

#if 0
static void bl_xhci_cr_evaluate_context(struct bl_xhci_controller *xhci,
	volatile struct bl_xhci_input_context *ic, int slot_id)
{
	volatile union bl_xhci_trb *event;

	event = bl_xhci_cr_queue_trb(xhci, (bl_uint64_t)ic & BL_XHCI_TRB_INPUT_CONTEXT_PTR_RSVD,
		0, BL_XHCI_TRB_SET_SLOT_ID(slot_id) | BL_XHCI_TRB_TYPE(BL_XHCI_CR_TRB_EVALUATE_CONTEXT) |
		BL_XHCI_TRB_C);
	if (!event)
		return;

	if (BL_XHCI_TRB_CC(event->template.status) != BL_XHCI_TRB_CC_SUCCESS)
		return;
}
#endif

static void bl_xhci_bulk_transfer(struct bl_usb_host_controller *hc,
	int address, struct bl_usb_endpoint *endp, bl_usb_speed_t speed,
	int direction, bl_uint8_t *data, int length)
{
	int ep, off = 0, packet_size;
	bl_uint8_t addr;
	struct bl_xhci_controller *xhci;
	union bl_xhci_trb td;
	volatile union bl_xhci_trb *tr, *trb, *event;
	volatile struct bl_xhci_endpoint_context *ep_context;

	bl_print_str("Bulk\n");

	xhci = hc->data;

	addr = endp->descriptor->endpoint_address;
	ep = 2 * BL_USB_ENDPOINT_ADDRESS(addr) + BL_USB_ENDPOINT_DIRECTION(addr);
	bl_print_str("EP : ");
	bl_print_hex(ep);
	bl_print_str(" ");
	bl_print_hex(address);
	bl_print_str("\n");

	packet_size = endp->descriptor->max_packet_size;

	ep_context = &xhci->devices[address]->ic->ep_context[ep - 2];

	if (ep_context->ep_type == BL_XHCI_EP_TYPE_NOT_VALID) {
		if (BL_USB_ENDPOINT_DIRECTION(addr))
			ep_context->ep_type = BL_XHCI_EP_TYPE_BULK_IN;
		else
			ep_context->ep_type = BL_XHCI_EP_TYPE_BULK_OUT;

		ep_context->max_packet_size = packet_size;

		// TODO: USB3 bMaxBurst
		ep_context->max_burst_size = 0;

		ep_context->cerr = 3;

		// TODO: Streams.

		ep_context->max_pstreams = 0;

		tr = bl_xhci_alloc_transfer_ring();
		if (!tr)
			return;

		ep_context->tr_dequeue_pointer = (bl_uint64_t)tr | 1;
		xhci->devices[address]->tr_enqueue[ep - 1] = xhci->devices[address]->tr[ep - 1] = tr;

		//ep_context->tr_dequeue_pointer = (bl_uint64_t)xhci->devices[address]->tr[0] | 1;

		xhci->devices[address]->ic->icc.add_context_flags |= BL_XHCI_INPUT_CONTROL_CONTEXT_A(ep);
		xhci->devices[address]->ic->icc.drop_context_flags &= ~BL_XHCI_INPUT_CONTROL_CONTEXT_A(ep);

		xhci->devices[address]->ic->slot.context_entries = ep;

		bl_xhci_cr_configure_endpoint(xhci, xhci->devices[address]->ic, xhci->devices[address]->slot_id);

		if (xhci->devices[address]->dc->ep_context[ep - 2].ep_state != BL_XHCI_EP_STATE_RUNNING)
			return;
	}

	while (length > 0) {
		bl_memset(&td, 0, sizeof(union bl_xhci_trb));

		td.normal.trb_type = BL_XHCI_TR_TRB_NORMAL;
		td.normal.data_buffer_pointer = (bl_uint64_t)data + off;
		td.normal.transfer_length = BL_MIN(length, packet_size);
		td.normal.td_size = 0;
		td.normal.c = 0;
		td.normal.ent = 0;
		td.normal.ch = length > packet_size;
		td.normal.ioc = 0;
		td.normal.idt = 0;
		td.normal.bei = 0;

		trb = xhci->devices[address]->tr_enqueue[ep - 1];

		trb->template.parameter = td.template.parameter;
		trb->template.status = td.template.status;
		trb->template.control = td.template.control;

		foo(trb);

		xhci->devices[address]->tr_enqueue[ep - 1]++;

		length -= packet_size;
		off += packet_size;
	}

	/* Doorbell ring. */
	xhci->db_regs[xhci->devices[address]->slot_id] = ep;

	int timeout = 20;
	while (--timeout) {
		event = bl_xhci_get_last_event(xhci);
		if (event) {
			foo(event);
			break;
		}

		bl_time_sleep(1);
	}
}

static bl_status_t bl_xhci_init_event_ring(struct bl_xhci_controller *xhci)
{
	int i;

	/* Event Ring. */
	xhci->event_ring = bl_heap_alloc_align(BL_XHCI_ER_TRBS * sizeof(union bl_xhci_trb), 0x40);
	if (!xhci->event_ring)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_XHCI_ER_TRBS; i++)
		bl_memset((void *)&xhci->event_ring[i], 0, sizeof(union bl_xhci_trb));

	/* Event Ring Segment Table. */
	xhci->erst = bl_heap_alloc_align(sizeof(struct bl_xhci_erst), 0x40);
	if (!xhci->erst)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	xhci->erst->ring_segment_address = (bl_uint64_t)xhci->event_ring;
	xhci->erst->ring_segment_size = BL_XHCI_ER_TRBS;

	/* Runtime registers & Interrupter & Event ring. */
	xhci->rt_regs = (bl_uint8_t *)xhci->cap_regs + (xhci->cap_regs->rtsoff & BL_XHCI_RTSOFF_RSVD);

	xhci->interrupter = &xhci->rt_regs->interrupters[0];

	xhci->interrupter->erstsz = 1;

	xhci->interrupter->erdp = xhci->erst->ring_segment_address;
	xhci->er_dequeue = xhci->event_ring;

	xhci->interrupter->erstba = (bl_uint64_t)&xhci->erst[0];

#if 0
	xhci->interrupter->erdp = xhci->erst->ring_segment_address | (1 << 3);

	xhci->interrupter->imod = 4000;

	xhci->op_regs->usb_cmd |= (2 << 1);

	xhci->interrupter->iman |= (1 << 0);

	xhci->interrupter->iman |= (1 << 1);

	bl_time_sleep(20);
#endif

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_xhci_init_command_ring(struct bl_xhci_controller *xhci)
{
	int i;

	/* Command Ring Dequeue Pointer. */
	xhci->command_ring = bl_heap_alloc_align(BL_XHCI_CR_TRBS * sizeof(union bl_xhci_trb), 0x40);
	if (!xhci->command_ring)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < BL_XHCI_CR_TRBS; i++)
		bl_memset((void *)&xhci->command_ring[i], 0, sizeof(union bl_xhci_trb));

	/* Last element, Link TRB. */
	xhci->command_ring[BL_XHCI_CR_TRBS - 1].link.ring_segment_pointer =
		(bl_uint64_t)&xhci->command_ring[0] | 1;

	xhci->command_ring[BL_XHCI_CR_TRBS - 1].link.control |= BL_XHCI_TRB_TYPE(BL_XHCI_CR_TRB_LINK);

	/* Write CRCR. */
	xhci->op_regs->crcr = (bl_uint64_t)xhci->command_ring | 1;

	xhci->cr_enqueue = xhci->cr_dequeue = xhci->command_ring;

	return BL_STATUS_SUCCESS;
}

// TODO: What about PSIC ?
static bl_status_t bl_xhci_get_supported_protocol(struct bl_xhci_controller *xhci)
{
	int i;
	bl_uint32_t xecp;
	volatile struct bl_xhci_ecpr *ecpr;

	xecp = BL_XHCI_HCCPARAMS1_XECP(xhci->cap_regs->hccparams1);
	if (!xecp)
		return BL_STATUS_USB_INTERNAL_ERROR;
	
	xhci->port_usb_type = bl_heap_alloc(BL_XHCI_HCSPARAMS1_MAXPORTS(xhci->cap_regs->hcsparams1));
	if (!xhci->port_usb_type)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	ecpr = (bl_uint8_t *)xhci->cap_regs + (xecp << 2);

	while (1) {
		if (ecpr->cap_id != BL_XHCI_EXTENDED_CAPABILITY_SUPPORTED_PROTOCOL)
			goto _next;

		volatile struct bl_xhci_supported_protocol *protocol = &ecpr->cap_specific[0];

		bl_print_hex(protocol->revision_major); bl_print_str(" ");
		bl_print_hex(protocol->revision_minor); bl_print_str(" ");
		bl_print_hex(protocol->name_string); bl_print_str(" ");
		bl_print_hex(protocol->compatible_port_offset); bl_print_str(" ");
		bl_print_hex(protocol->compatible_port_count); bl_print_str(" ");
		bl_print_hex(protocol->protocol_defined); bl_print_str(" ");
		bl_print_hex(protocol->psic); bl_print_str(" ");
		bl_print_hex(protocol->protocol_slot_type); bl_print_str("\n");

		for (i = protocol->compatible_port_offset; i < protocol->compatible_port_offset +
			protocol->compatible_port_count; i++)
			xhci->port_usb_type[i - 1] = protocol->revision_major;

_next:
		if (!ecpr->next)
			break;

		ecpr = (bl_uint8_t *)ecpr + (ecpr->next << 2);
	}

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_xhci_get_extended_capabilities(struct bl_xhci_controller *xhci)
{
	return bl_xhci_get_supported_protocol(xhci);
}

static bl_status_t bl_xhci_init_data_structures(struct bl_xhci_controller *xhci)
{
	bl_status_t status;

	/* Max device slots enabled. */
	xhci->op_regs->config |= xhci->cap_regs->hcsparams1 & BL_XHCI_HCSPARAMS1_MAXSLOTS;

	/* Device Context Base Array Address Pointer. */
	xhci->dcbaa = bl_heap_alloc_align((1 + (xhci->op_regs->config & BL_XHCI_CONFIG_MAXSLOTSEN)) *
		sizeof(bl_uint64_t), 0x40);
	if (!xhci->dcbaa)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	xhci->op_regs->dcbaap = (bl_uint64_t)xhci->dcbaa;

	/* Command ring initialization. */
	status = bl_xhci_init_command_ring(xhci);
	if (status)
		return status;

	/* Event ring initialization. */
	status = bl_xhci_init_event_ring(xhci);
	if (status)
		return status;

	/* XHCI Extended Capabilities. */
	status = bl_xhci_get_extended_capabilities(xhci);
	if (status)
		return status;

	/* Port Register Set. */
	xhci->ports = (bl_uint8_t *)xhci->op_regs + 0x400;

	/* Doorbell registers. */
	xhci->db_regs = (bl_uint8_t *)xhci->cap_regs + (xhci->cap_regs->dboff & BL_XHCI_DBOFF_RSVD);

	/* Available devices. */
	xhci->devices = bl_heap_alloc(127 * sizeof(struct bl_xhci_device *));
	if (!xhci->devices)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_xhci_controller_init(struct bl_xhci_controller *xhci)
{
	int timeout;
	bl_status_t status;

	xhci->op_regs = (bl_uint8_t *)xhci->cap_regs + xhci->cap_regs->caplength;

	/* Wait for Controller Not Ready (CNR) flag to be 0. */
	timeout = 1000;
	while (--timeout) {
		if ((xhci->op_regs->usbsts & BL_XHCI_USBSTS_CNR) == 0)
			break;

		bl_time_sleep(1);
	}

	if (xhci->op_regs->usbsts & BL_XHCI_USBSTS_CNR)
		return BL_STATUS_USB_INTERNAL_ERROR;
	
	/* Stop XHCI. */
	xhci->op_regs->usbcmd &= ~BL_XHCI_USBCMD_RS;
	bl_time_sleep(16);

	if ((xhci->op_regs->usbsts & BL_XHCI_USBSTS_HCH) == 0)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* Reset XHCI. */
	xhci->op_regs->usbcmd |= BL_XHCI_USBCMD_HCRST;

	timeout = 50;
	while (--timeout) {
		if ((xhci->op_regs->usbcmd & BL_XHCI_USBCMD_HCRST) == 0)
			break;

		bl_time_sleep(1);
	}

	if (xhci->op_regs->usbcmd & BL_XHCI_USBCMD_HCRST)
		return BL_STATUS_USB_INTERNAL_ERROR;

	/* Initialize data structures. */
	status = bl_xhci_init_data_structures(xhci);
	if (status)
		return status;

	/* Run the controller. */
	xhci->op_regs->usbcmd |= BL_XHCI_USBCMD_RS;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_xhci_pci_init(struct bl_pci_index i)
{
	bl_status_t status;
	bl_uint8_t sbrn;
	bl_uint32_t base_address;
	struct bl_xhci_controller *xhci;

	/* Check PCI device. */
	status = bl_pci_check_device_class(i, BL_PCI_BASE_CLASS_SERIAL,
		BL_PCI_SUBCLASS_SERIAL_USB_XHCI, BL_PCI_PROG_IF_SERIAL_USB_XHCI);
	if (status)
		return status;

	/* Check serial bus release number (Release 3.0/3.1 and 2.0). */
	sbrn = bl_pci_read_config_byte(i.bus, i.dev, i.func, BL_PCI_CONFIG_REG_60H);
	if (sbrn != BL_XHCI_PCI_SBRN_3_0 && sbrn != BL_XHCI_PCI_SBRN_3_1 &&
		sbrn != BL_XHCI_PCI_SBRN_2_0)
		return BL_STATUS_USB_INVALID_HOST_CONTROLLER;

	/* Base address. */
	base_address = bl_pci_read_config_long(i.bus, i.dev, i.func, BL_PCI_CONFIG_REG_BAR0);

	/* Initialize the controller. */
	xhci = bl_heap_alloc(sizeof(struct bl_xhci_controller));
	if (!xhci)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	/* Capability registers. */
	xhci->cap_regs = (void *)(base_address & BL_XHCI_PCI_BAR0_BASE_ADDRESS);

	/* Finally initialize XHCI. */
	status = bl_xhci_controller_init(xhci);
	if (status) {

	}

	bl_xhci_controller_add(xhci);

	return BL_STATUS_SUCCESS;
}

static struct bl_usb_host_controller_functions bl_xhci_functions = {
	.port_device_connected = bl_xhci_port_device_connected,
	.port_init = bl_xhci_port_init,
	.control_transfer = bl_xhci_control_transfer,
	.bulk_transfer = bl_xhci_bulk_transfer,
};

BL_MODULE_INIT()
{
	struct bl_xhci_controller *xhci;

	bl_pci_iterate_devices(bl_xhci_pci_init);

	xhci = bl_xhci_list;
	while (xhci) {
		struct bl_usb_host_controller *hc =
			bl_heap_alloc(sizeof(struct bl_usb_host_controller));
		if (!hc)
			return;

		hc->type = BL_USB_HC_XHCI;
		hc->funcs = &bl_xhci_functions;
		hc->data = (void *)xhci;
		hc->root_hub.ports = BL_XHCI_HCSPARAMS1_MAXPORTS(xhci->cap_regs->hcsparams1);

		bl_usb_host_controller_register(hc);

		xhci = xhci->next;
	}
}

BL_MODULE_UNINIT()
{

}

#pragma GCC diagnostic pop

