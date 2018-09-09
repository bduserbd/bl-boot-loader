#ifndef BL_OHCI_H
#define BL_OHCI_H

#include "include/bl-types.h"

/* OHCI PCI Info. */
enum {
	BL_OHCI_PCI_BAR0_INDICATPR	= (1 << 0),
	BL_OHCI_PCI_BAR0_BASE_ADDRESS	= (0xfffff << 12),
};

/* OHCI HcRevision. */
enum {
	BL_OHCI_REG_REVISION_REV	= (0xff << 0), /* Revision */
};

/* OHCI HcControl. */
enum {
	BL_OHCI_REG_CONTROL_PLE		= (1 << 2), /* PeriodicListEnable */
	BL_OHCI_REG_CONTROL_CLE		= (1 << 4), /* ControlListEnable */
	BL_OHCI_REG_CONTROL_BLE		= (1 << 5), /* BulkListEnable */
	BL_OHCI_REG_CONTROL_HCFS	= (3 << 6), /* HostControlFunctionalState */
	BL_OHCI_REG_CONTROL_IR		= (1 << 8), /* InterruptRouting */
};

/* HostControlFunctionalState. */
enum {
	BL_OHCI_HCFS_USB_RESET		= (0 << 6),
	BL_OHCI_HCFS_USB_RESUME		= (1 << 6),
	BL_OHCI_HCFS_USB_OPERATIONAL	= (2 << 6),
	BL_OHCI_HCFS_USB_SUSPEND	= (3 << 6),
};

/* OHCI HcCommandStatus. */
enum {
	BL_OHCI_REG_COMMAND_STATUS_HCR	= (1 << 0), /* HostControllerReset */
	BL_OHCI_REG_COMMAND_STATUS_CLF	= (1 << 1), /* ControlListFilled */
	BL_OHCI_REG_COMMAND_STATUS_BLF	= (1 << 2), /* BulkListFilled */
	BL_OHCI_REG_COMMAND_STATUS_OCR	= (1 << 3), /* OwnershipChangeRequest */
};

/* OHCI HcInterruptStatus. */
enum {
	BL_OHCI_REG_INTERRUPT_STATUS_SO		= (1 << 0), /* SchedulingOverrun */
	BL_OHCI_REG_INTERRUPT_STATUS_WDH	= (1 << 1), /* WritebackDoneHead */
	BL_OHCI_REG_INTERRUPT_STATUS_SF		= (1 << 2), /* StartofFrame */
	BL_OHCI_REG_INTERRUPT_STATUS_RD		= (1 << 3), /* ResumedDeteced */
	BL_OHCI_REG_INTERRUPT_STATUS_UE		= (1 << 4), /* UndrecoverableError */
	BL_OHCI_REG_INTERRUPT_STATUS_FNO	= (1 << 5), /* FrameNumberOverflow */
	BL_OHCI_REG_INTERRUPT_STATUS_RHSC	= (1 << 6), /* RootHubStatusChange */
	BL_OHCI_REG_INTERRUPT_STATUS_OC		= (1 << 30), /* OwnershipChange */
};

/* OHCI HcInterruptEnable. */
enum {
	BL_OHCI_REG_INTERRUPT_ENABLE_SO		= (1 << 0), /* SchedulingOverrun */
	BL_OHCI_REG_INTERRUPT_ENABLE_WDH	= (1 << 1), /* WritebackDoneHead */
	BL_OHCI_REG_INTERRUPT_ENABLE_SF		= (1 << 2), /* StartofFrame */
	BL_OHCI_REG_INTERRUPT_ENABLE_RD		= (1 << 3), /* ResumedDeteced */
	BL_OHCI_REG_INTERRUPT_ENABLE_UE		= (1 << 4), /* UndrecoverableError */
	BL_OHCI_REG_INTERRUPT_ENABLE_FNO	= (1 << 5), /* FrameNumberOverflow */
	BL_OHCI_REG_INTERRUPT_ENABLE_RHSC	= (1 << 6), /* RootHubStatusChange */
	BL_OHCI_REG_INTERRUPT_ENABLE_OC		= (1 << 30), /* OwnershipChange */
	BL_OHCI_REG_INTERRUPT_ENABLE_MIE	= (1 << 31), /* MasterInterruptEnable */
};

/* OHCI HcInterruptStatus. */
enum {
	BL_OHCI_REG_INTERRUPT_DISABLE_SO	= (1 << 0), /* SchedulingOverrun */
	BL_OHCI_REG_INTERRUPT_DISABLE_WDH	= (1 << 1), /* WritebackDoneHead */
	BL_OHCI_REG_INTERRUPT_DISABLE_SF	= (1 << 2), /* StartofFrame */
	BL_OHCI_REG_INTERRUPT_DISABLE_RD	= (1 << 3), /* ResumedDeteced */
	BL_OHCI_REG_INTERRUPT_DISABLE_UE	= (1 << 4), /* UndrecoverableError */
	BL_OHCI_REG_INTERRUPT_DISABLE_FNO	= (1 << 5), /* FrameNumberOverflow */
	BL_OHCI_REG_INTERRUPT_DISABLE_RHSC	= (1 << 6), /* RootHubStatusChange */
	BL_OHCI_REG_INTERRUPT_DISABLE_OC	= (1 << 30), /* OwnershipChange */
	BL_OHCI_REG_INTERRUPT_DISABLE_MIE	= (1 << 31), /* MasterInterruptEnable */
};

/* OHCI HcFmInterval. */
enum {
	BL_OHCI_REG_FM_INTERVAL_FI	= (0x3fff << 0), /* FrameInterval */
};

/* OHCI HcRhDescriptorA. */
enum {
	BL_OHCI_REG_RH_DESCRIPTOR_A_NDP	= (0xff << 0), /* NumberDownstreamPorts */
};

/* OHCI HcRhPortStatus. */
enum {
	BL_OHCI_REG_RH_PORT_STATUS_CCS	= (1 << 0), /* CurrentConnectStatus */
	BL_OHCI_REG_RH_PORT_STATUS_PES	= (1 << 1), /* PortEnableStatus */
	BL_OHCI_REG_RH_PORT_STATUS_PRS	= (1 << 4), /* PortResetStatus */
	BL_OHCI_REG_RH_PORT_STATUS_LSDA	= (1 << 9), /* LowSpeedDeviceAttached */
	BL_OHCI_REG_RH_PORT_STATUS_PESC	= (1 << 16), /* PortEnableStatusChange */
	BL_OHCI_REG_RH_PORT_STATUS_PRSC	= (1 << 20), /* PortResetStatusChange */
};

/* OHCI Registers. */
struct bl_ohci_registers {
	/* Control & Status. */
	__u32	revision;
	__u32	control;
	__u32	command_status;
	__u32	interrupt_status;
	__u32	interrupt_enable;
	__u32	interrupt_disable;

	/* Memory Pointers. */
	__u32	hcca;
	__u32	period_current_ed;
	__u32	control_head_ed;
	__u32	control_current_ed;
	__u32	bulk_head_ed;
	__u32	bulk_current_ed;
	__u32	done_head;

	/* Frame Counter. */
	__u32	fm_interval;
	__u32	fm_remaining;
	__u32	fm_number;
	__u32	periodic_start;
	__u32	lsthreshold;

	/* Root Hub. */
	__u32	rh_descriptor_a;
	__u32	rh_descriptor_b;
	__u32	rh_status;
	__u32	rh_port_status[15];
} __attribute__((packed));

/* OHCI Host Controller Communications Area (HCCA). */
struct bl_ohci_hcca {
	__u32	interrupt_table[32];
	__u16	frame_number;
	__u16	pad1;
	__u32	done_head;
	__u8	reserved[116];
} __attribute__((packed));

/* OHCI Endpoint Descriptor. */
enum {
	BL_OHCI_ED_FLAGS_ADDRESS		= (0x7f << 0),
	BL_OHCI_ED_FLAGS_ENDPOINT		= (0xf << 7),
	BL_OHCI_ED_FLAGS_SPEED			= (1 << 13),
	BL_OHCI_ED_FLAGS_SKIP			= (1 << 14),
	BL_OHCI_ED_FLAGS_MPS			= (0x7ff << 16),

	BL_OHCI_ED_TD_HEAD_POINTER_HALTED	= (1 << 0),
};

#define BL_OHCI_ED_FLAGS_ADDRESS_ENDPOINT	\
	(BL_OHCI_ED_FLAGS_ADDRESS | BL_OHCI_ED_FLAGS_ENDPOINT)

#define BL_OHCI_ED_ADDRESS_ENDPOINT(address, endpoint)	\
	(((address << 0) & BL_OHCI_ED_FLAGS_ADDRESS) |	\
	 ((endpoint << 7) & BL_OHCI_ED_FLAGS_ENDPOINT))

#define BL_OHCI_ED_MPS(max_packet_size)	\
	((max_packet_size << 16) & BL_OHCI_ED_FLAGS_MPS)

#define BL_OHCI_ED_TD_POINTER(td_pointer)	\
	((volatile struct bl_ohci_general_td *)(td_pointer & (~0xf)))

struct bl_ohci_ed {
	__u32	flags;
	__u32	td_tail_pointer;
	__u32	td_head_pointer;
	__u32	next_ed;
} __attribute__((packed));

/* OHCI completion codes. */
enum {
	BL_OHCI_CC_NO_ERROR			= 0x00,
	BL_OHCI_CC_CRC				= 0x01,
	BL_OHCI_CC_BIT_STUFFING			= 0x02,
	BL_OHCI_CC_DATA_TOGGLE_MISMATCH		= 0x03,
	BL_OHCI_CC_STALL			= 0x04,
	BL_OHCI_CC_DEVICE_NOT_RESPONDING	= 0x05,
	BL_OHCI_CC_PID_CHECK_FAILURE		= 0x06,
	BL_OHCI_CC_UNEXPECTED_PID		= 0x07,
	BL_OHCI_CC_DATA_OVERRUN			= 0x08,
	BL_OHCI_CC_DATA_UNDERRUN		= 0x09,
	BL_OHCI_CC_BUFFER_OVERRUN		= 0x0c,
	BL_OHCI_CC_BUFFER_UNDERRUN		= 0x0d,
	BL_OHCI_CC_NOT_ACCESSED			= 0x0f,
};

/* OHCI General Transfser Descriptor. */
enum {
	BL_OHCI_TD_FLAGS_DP_SETUP	= (0 << 19),
	BL_OHCI_TD_FLAGS_DP_OUT		= (1 << 19),
	BL_OHCI_TD_FLAGS_DP_IN		= (2 << 19),
};

/* Derived from TD, not ED. */
#define BL_OHCI_TD_FLAGS_T(DataToggle)		((0x2 | (DataToggle & 0x1)) << 24)

#define BL_OHCI_TD_FLAGS_EC(ErrorCount)		((ErrorCount & 0x3) << 26)

#define BL_OHCI_TD_FLAGS_CC(ConditionCode)	((ConditionCode & 0xf) << 28)
#define BL_OHCI_TD_FLAGS_TO_CC(flags)		((flags >> 28) & 0xf)

struct bl_ohci_general_td {
	__u32	flags;
	__u32	current_buffer_pointer;
	__u32	next_td;
	__u32	buffer_end;

	/* Software use. */
	int	used;
	__u32	original_next_td; // Circular TD list.

	__u32	reserved[2];
} __attribute__((packed));

#endif

