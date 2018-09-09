#ifndef PCI_H
#define PCI_H

#include "include/error.h"
#include "include/bl-types.h"
#include "include/io.h"
#include "pci-ids.h"

/*
 * PCI configuration space:
 * ------------------------
 *
 * 31         24 23        16 15         8 7           0
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |        Device ID        |        Vendor ID        | 00h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |         Status          |         Command         | 04h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |              Class Code              | Revision ID| 08h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |    BIST    | Header Type| Lat. Timer |Cacheline Sz| 0Ch
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |                       BAR # 0                     | 10h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |                       BAR # 1                     | 14h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |                       BAR # 2                     | 18h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |                       BAR # 3                     | 1Ch
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |                       BAR # 4                     | 20h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |                       BAR # 5                     | 24h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |                Cardbus CIS Pointer                | 28h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |       Subsystem ID      |   Subsystem Vendor ID   | 2Ch
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |            Expansion ROM Base Address             | 30h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |               Reserved               |Cap. Pointer| 34h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |                     Reserved                      | 38h
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 * |  Max Lat.  |  Min Gnt.  |  Int. Pin  |  Int. Line | 3Ch
 * |++++++++++++|++++++++++++|++++++++++++|++++++++++++|
 */

/* Offsets into the configuration space. */
#define BL_PCI_CONFIG_REG_VENDOR_ID	0x00
#define BL_PCI_CONFIG_REG_DEVICE_ID	0x02
#define BL_PCI_CONFIG_REG_COMMAND	0x04
#define BL_PCI_CONFIG_REG_STATUS	0x06
#define BL_PCI_CONFIG_REG_REVISION_ID	0x08
#define BL_PCI_CONFIG_REG_CLASS_CODE		0x09
# define BL_PCI_CONFIG_REG_PROG_IF		0x09
# define BL_PCI_CONFIG_REG_SUBCLASS		0x0a
# define BL_PCI_CONFIG_REG_BASECLASS		0x0b
#define BL_PCI_CONFIG_REG_CACHELINE_SIZE	0x0c
#define BL_PCI_CONFIG_REG_LATENCY_TIMER	0x0d
#define BL_PCI_CONFIG_REG_HEADER_TYPE	0x0e
#define BL_PCI_CONFIG_REG_BIST		0x0f

/* Base Address - BAR. */
#define BL_PCI_CONFIG_REG_BAR0	0x10
#define BL_PCI_CONFIG_REG_BAR1	0x14
#define BL_PCI_CONFIG_REG_BAR2	0x18
#define BL_PCI_CONFIG_REG_BAR3	0x1c
#define BL_PCI_CONFIG_REG_BAR4	0x20
#define BL_PCI_CONFIG_REG_BAR5	0x24

/* Various. */
#define BL_PCI_CONFIG_REG_60H	0x60

/* PCI Structure */
#define BL_PCI_NBUSES		256
#define BL_PCI_NDEVICES		32
#define BL_PCI_NFUNCTIONS	8

/* Address & Data I/O access ports */
#define BL_PCI_CONFIG_ADDRESS	0xcf8
#define BL_PCI_CONFIG_DATA		0xcfc

/*
 * PCI configuration address register:
 * -----------------------------------
 *
 * |++++++|+++++++++|+++++|++++++|++++++++|++++++++|++++|
 * |  31  | 30 - 24 |23-16| 15-11|  10-8  |  7 - 2 | 1-0|
 * |++++++|+++++++++|+++++|++++++|++++++++|++++++++|++++|
 * |Enable| Reserved| Bus |Device|Function|Register|Zero|
 * |++++++|+++++++++|+++++|++++++|++++++++|++++++++|++++|
 *
 */
#define BL_PCI_MAKE_CONFIG_ADDRESS(bus, dev, func, reg)	\
	((1 << 31) | (bus << 16) | (dev << 11) | (func << 8) | reg)

/* Read PCI configuration address registers */ 
static inline u8 bl_pci_read_config_byte(u8 bus, u8 dev, u8 func, u8 reg)
{
	bl_outl(BL_PCI_MAKE_CONFIG_ADDRESS(bus, dev, func, reg), BL_PCI_CONFIG_ADDRESS);

	return bl_inb(BL_PCI_CONFIG_DATA + (reg & 3));
}

static inline u16 bl_pci_read_config_word(u8 bus, u8 dev, u8 func, u8 reg)
{
	bl_outl(BL_PCI_MAKE_CONFIG_ADDRESS(bus, dev, func, reg), BL_PCI_CONFIG_ADDRESS);

	return bl_inw(BL_PCI_CONFIG_DATA + (reg & 2));
}

static inline u32 bl_pci_read_config_long(u8 bus, u8 dev, u8 func, u8 reg)
{
	bl_outl(BL_PCI_MAKE_CONFIG_ADDRESS(bus, dev, func, reg), BL_PCI_CONFIG_ADDRESS);

	return bl_inl(BL_PCI_CONFIG_DATA);
}

struct bl_pci_index {
	u32 bus;
	u32 dev;
	u32 func;
};

typedef bl_status_t (*pci_init_device_t)(struct bl_pci_index);

void bl_pci_iterate_devices(pci_init_device_t);
void bl_pci_dump_devices(void);
bl_status_t bl_pci_check_device_class(struct bl_pci_index, int, int, int);

#endif

