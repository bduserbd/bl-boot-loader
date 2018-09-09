#if 0
NOTE: Old version file !!!

#ifndef BL_ACPI_H
#define BL_ACPI_H

#include "bl_types.h"

#define BL_ACPI_RSDP_SIGNATURE	"RSD PTR "
#define BL_ACPI_RSDT_SIGNATURE	"RSDT"
#define BL_ACPI_FADT_SIGNATURE	"FACP"

struct bl_acpi_rsdp_version10 {
	__u8	signature[8];
	__u8	checksum;
	__u8	oemid[6];
	__u8	revision;
	__u32	rsdt_address;
} __attribute__((packed));


struct bl_acpi_rsdp_version20 {
	struct bl_acpi_rsdp_version10	rsdp_ver10;
	__u32	length;
	__u64	xsdt_address;
	__u8	extended_checksum;
	__u8	reserved[3];
} __attribute__((packed));

struct bl_acpi_sdt {
	__u8	signature[4];
	__u32	length;
	__u8	revision;
	__u8	checksum;
	__u8	oemid[6];
	__u8	oem_table_id[8];
	__u32	oem_revision;
	__u8	creator_id[4];
	__u32	creator_revision;
} __attribute__((packed));

struct bl_acpi_rsdt {
	struct bl_acpi_sdt	sdt;
	__u32	entries[0];
} __attribute__((packed));

struct bl_acpi_xsdt {
	struct bl_acpi_sdt	sdt;
	__u64	entries[0];
} __attribute__((packed));

struct bl_acpi_fadt {
	struct bl_acpi_sdt	sdt;
	__u32	facs_address;
	__u32	dsdt_address;
	__u8	reserved[1];
	__u8	preferred_pm_profile;
	__u16	sci_int;
	__u32	smi_cmd;
	__u8	acpi_enable;
	__u8	acpi_disable;
	__u8	s4bios_req;
	__u8	pstate_cnt;
	__u32	pm1a_evt_blk;
	__u32	pm1b_evt_blk;
	__u32	pm1a_cnt_blk;
	__u32	pm1b_cnt_blk;
	__u32	pm2_cnt_blk;
	__u32	pm_tmr_blk;
	__u32	gpe0_blk;
	__u32	gpe1_blk;
	__u8	pm1_evt_len;
	__u8	pm1_cnt_len;
	__u8	pm2_cnt_len;
	__u8	pm_tmr_len;
	__u8	gpe0_blk_len;
	__u8	gpe1_blk_len;
	__u8	gpe1_base;
	__u8	cst_cnt;
	__u16	p_lvl2_lat;
	__u16	p_lvl3_lat;
	__u16	flush_size;
	__u16	flush_stride;
	__u8	duty_offset;
	__u8	duty_width;
	__u8	day_alarm;
	__u8	mon_alarm;
	__u8	century;
	__u8	unused0[23];
	__u64	x_facs_address;
	__u64	x_dsdt_address;
	__u8	unused1[120];
} __attribute__((packed));

struct bl_acpi_dsdt {
	struct bl_acpi_sdt	sdt;
	__u8	aml[0];
} __attribute__((packed));

void bl_acpi_setup(void);

#endif

#endif

