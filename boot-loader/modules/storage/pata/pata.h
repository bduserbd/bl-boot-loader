#ifndef BL_PATA_H
#define BL_PATA_H

#include "include/bl-types.h"

/* PATA PCI BAR4 register info. */
enum {
	BL_PATA_PCI_BAR4_RTE		= 0x01,
	BL_PATA_PCI_BAR4_BASE_ADDRESS	= 0xffe0,
};

/* Channels and ports. */
enum {
	BL_PATA_PRIMARY_PORT		= 0x1f0,
	BL_PATA_PRIMARY_CONTROL_PORT	= 0x3f6,

	BL_PATA_SECONDARY_PORT		= 0x170,
	BL_PATA_SECONDARY_CONTROL_PORT	= 0x376,
};

/* PATA master & slave devices. */
#define BL_PATA_MASTER_DEVICE	0
#define BL_PATA_SLAVE_DEVICE	1

/* PATA Status Register (read only). */
enum {
	BL_PATA_REG_STATUS_ERR	= (1 << 0),
	BL_PATA_REG_STATUS_DRQ	= (1 << 3),
	BL_PATA_REG_STATUS_DRDY	= (1 << 6),
	BL_PATA_REG_STATUS_BSY	= (1 << 7),
};

/*
 * Device register (write only):
 * -----------------------------
 *
 * |+++|+++|+++|+++|++++++|
 * | 7 | 6 | 5 | 4 | 3 - 0|
 * |+++|+++|+++|+++|++++++|
 * | 1 |LBA| 1 |DEV| HEAD |
 * |+++|+++|+++|+++|++++++|
 */
#define BL_PATA_REG_DEVICE_SET(lba, type)	\
	((1 << 7) | ((lba & 1) << 6) | (1 << 5) | ((type & 1) << 4) | 0x0)

/* PATA (not PATAPI) registers indexes into I/O address */
/* Registers when read */
enum {
	BL_PATA_REG__READ__DPATA	= 0x0,
	BL_PATA_REG__READ__ERROR	= 0x1,
	BL_PATA_REG__READ__SECTOR_COUNT	= 0x2,

	/* The lower 24-bit LBA */
	BL_PATA_REG__READ__LBA_LOW	= 0x3,
	BL_PATA_REG__READ__LBA_MID	= 0x4,
	BL_PATA_REG__READ__LBA_HIGH	= 0x5,

	/* The upper 24-bit LBA */
	BL_PATA_REG__READ__LBA48_LOW	= 0x3,
	BL_PATA_REG__READ__LBA48_MID	= 0x4,
	BL_PATA_REG__READ__LBA48_HIGH	= 0x5,

	BL_PATA_REG__READ__DEVICE	= 0x6,
	BL_PATA_REG__READ__STATUS	= 0x7,
};

/* Registers when written */
enum {
	BL_PATA_REG__WRITE__DPATA		= 0x0,
	BL_PATA_REG__WRITE__FEATURES		= 0x1,
	BL_PATA_REG__WRITE__SECTOR_COUNT	= 0x2,

	/* The lower 24-bit LBA */
	BL_PATA_REG__WRITE__LBA_LOW		= 0x3,
	BL_PATA_REG__WRITE__LBA_MID		= 0x4,
	BL_PATA_REG__WRITE__LBA_HIGH		= 0x5,

	/* The upper 24-bit LBA */
	BL_PATA_REG__WRITE__LBA48_LOW		= 0x3,
	BL_PATA_REG__WRITE__LBA48_MID		= 0x4,
	BL_PATA_REG__WRITE__LBA48_HIGH		= 0x5,

	BL_PATA_REG__WRITE__DEVICE		= 0x6,
	BL_PATA_REG__WRITE__COMMAND		= 0x7,
};

/* PATA commands */
enum {
	BL_PATA_CMD_IDENTIFY_DEVICE		= 0xec,
	BL_PATA_CMD_READ_SECTORS		= 0x20,
	BL_PATA_CMD_READ_SECTORS_EXT		= 0x24,
	BL_PATA_CMD_READ_SECTORS_DMA_EXT	= 0x25,
	BL_PATA_CMD_READ_SECTORS_DMA		= 0xc8,
	BL_PATA_CMD_WRITE_SECTORS		= 0x30,
	BL_PATA_CMD_WRITE_SECTORS_EXT		= 0x34,
	BL_PATA_CMD_WRITE_SECTORS_DMA_EXT	= 0x35,
	BL_PATA_CMD_WRITE_SECTORS_DMA		= 0xca,
};

/* PATA identification. */
struct bl_pata_identification {
	__u16	flags;					/* Word 0 */
	__u16	unused0[9];
	__u8	serial_number[20];			/* Word 10-19*/
	__u16	unused1[3];
	__u8	firmware_revision[8];			/* Word 23-26 */
	__u8	model_number[40];			/* Word 27-46 */
	__u16	unused2[2];
	__u32	capabilities;				/* Word 49-50 */
	__u16	unused3[9];
	__u32	sectors_lba28;				/* Word 60-61 */
	__u16	unused4[38];
	__u64	sectors_lba48;				/* Word 100-103 */
	__u16	unused5[152];
} __attribute__((packed));

#endif

