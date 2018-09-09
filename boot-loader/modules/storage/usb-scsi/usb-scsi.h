#ifndef BL_USB_SCSI_H
#define BL_USB_SCSI_H

#include "include/bl-types.h"

/* SCSI Commands. */
enum {
	BL_SCSI_COMMAND_SENSE_DATA	= 0x03,
	BL_SCSI_COMMAND_INQUIRY		= 0x12,
	BL_SCSI_COMMAND_READ_CAPACITY10	= 0x25,
	BL_SCSI_COMMAND_READ6		= 0x08,
	BL_SCSI_COMMAND_READ10		= 0x28,
	BL_SCSI_COMMAND_READ_CAPACITY16	= 0x9e,
};

/* SCSI Inquiry Command. */
struct bl_scsi_inquiry {
	__u8	opcode;
	__u8	lun;
	__u8	page_code;
	__u8	reserved0;
	__u8	allocation_length;
	__u8	control;
} __attribute__((packed));

struct bl_scsi_inquiry_data {
	__u8	device_type;
	__u8	rmb;
	__u8	reserved0[2];
	__u8	additional_length;
	__u8	reserved1[3];
	__u8	vendor[8];
	__u8	product_id[16];
	__u8	firmware_revision[4];
} __attribute__((packed));

/* SCSI Request Sense. */
struct bl_scsi_request_sense {
	__u8	opcode;
	__u8	lun;
	__u8	reserved0[2];
	__u8	allocation_length;
	__u8	control;
} __attribute__((packed));

struct bl_scsi_request_sense_data {
	__u8	response_code;
	__u8	obsolete;
	__u8	flags;
	__u32	information;
	__u8	additional_sense_length;
	__u32	command_specific_information;
	__u8	additional_sense_code;
	__u8	additional_sense_code_qualifier;
	__u8	field_replaceable_unit_code;
	__u8	sense_key_specific[3];
	__u8	additional_sense_bytes[0];
} __attribute__((packed));

/* SCSI Read. */
struct bl_scsi_read6 {
	__u8	opcode;
	__u8	lba[3];
	__u8	tranfser_length;
	__u8	control;
} __attribute__((packed));

struct bl_scsi_read10 {
	__u8	opcode;
	__u8	reserved0;
	__u32	lba;
	__u8	reserved1;
	__u16	tranfser_length;
	__u8	control;
} __attribute__((packed));

/* SCSI Read Capacity. */
struct bl_scsi_read_capacity10 {
	__u8	opcode;
	__u8	lun;
	__u32	lba;
	__u16	reserved0;
	__u8	reserved1;
	__u8	control;
} __attribute__((packed));

struct bl_scsi_read_capacity10_data {
	__u32	lba;
	__u32	block_size;
} __attribute__((packed));

struct bl_scsi_read_capacity16 {
	__u8	opcode;
	__u8	lun;
	__u64	lba;
	__u32	allocation_length;
	__u8	reserved0;
	__u8	control;
} __attribute__((packed));

struct bl_scsi_read_capacity16_data {
	__u64	lba;
	__u32	block_size;
	__u8	unused0[32 - 12];
} __attribute__((packed));

/* CBW Signature. */
#define BL_USB_BBB_CBW_SIGNATURE	0x43425355

struct bl_usb_bbb_cbw {
	__u32	signature;
	__u32	tag;
	__u32	data_transfer_length;
	__u8	flags;
	__u8	lun;
	__u8	cb_length;
	__u8	cmd[16];
} __attribute__((packed));

/* CSW Signature. */
#define BL_USB_BBB_CSW_SIGNAUTE	0x53425355

struct bl_usb_bbb_csw {
	__u32	signature;
	__u32	tag;
	__u32	data_residue;
	__u8	status;
} __attribute__((packed));

#endif

