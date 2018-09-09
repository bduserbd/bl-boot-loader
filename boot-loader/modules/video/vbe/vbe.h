#ifndef BL_VBE_H
#define BL_VBE_H

#include "include/bl-types.h"

/* VBE versions. */
enum {
	BL_VBE_VERSION_1_0	= 0x100,
	BL_VBE_VERSION_1_1	= 0x101,
	BL_VBE_VERSION_1_2	= 0x102,
	BL_VBE_VERSION_2_0	= 0x200,
	BL_VBE_VERSION_3_0	= 0x300,
};

/* VBE BIOS success return code */
#define BL_VBE_SUCCESS	0x004f

/* VBE video modes list end */
#define BL_VBE_VIDEO_MODES_LIST_END	0xffff

/* VBE functions & subfunctions */
#define BL_VBE_BIOS_FUNCTION	0x4f

enum {
	BL_VBE_BIOS_SUBFUNCTION_GET_INFO		= 0x00,
	BL_VBE_BIOS_SUBFUNCTION_GET_MODE_INFO		= 0x01,
	BL_VBE_BIOS_SUBFUNCTION_SET_MODE_INFO		= 0x02,
	BL_VBE_BIOS_SUBFUNCTION_GET_SET_DISPLAY_START 	= 0x07,
	BL_VBE_BIOS_SUBFUNCTION_DDC			= 0x15,
};

/* VBE mode attributes */
enum {
	BL_VBE_MODE_ATTRIBUTES_HARDWARE_SUPPORT			= (1 << 0),
	BL_VBE_MODE_ATTRIBUTES_COLOR_MODE			= (1 << 3),
	BL_VBE_MODE_ATTRIBUTES_GRPHICS_MODE			= (1 << 4),
	BL_VBE_MODE_ATTRIBUTES_LINEAR_FRAME_BUFFER		= (1 << 7),
};

/* VBE memory models */
enum {
	BL_VBE_MEMORY_MODEL_DIRECT_COLOR		= 0x06,
};

/* VBE general information */
#define BL_VBE_INFO_BLOCK_SIGNATURE	(bl_uint8_t *)"VESA"

struct bl_vbe_info_block {
	__u8	signature[4];
	__u16	version;
	__u32	oem_ptr;
	__u32	capabilities;
	__u32	video_modes;
	__u16	total_memory;
	__u16	software_revision;
	__u32	vendor_name_ptr;
	__u32	product_name_ptr;
	__u32	product_revision_ptr;
	__u8	reserved[222];
	__u8	oem_data[256];
} __attribute__((packed));

/* VBE 3.0 video mode information as in specification */
struct bl_vbe_mode_info_block {
	/* Mandatory information for all VBE revisions. */
	__u16	mode_attributes;
	__u8	win_a_attributes;
	__u8	win_b_attributes;
	__u16	win_granularity;
	__u16	win_size;
	__u16	win_a_segment;
	__u16	win_b_segment;
	__u32	win_func_ptr;
	__u16	bytes_per_scan_line;

	/* Mandatory inforamtion for VBE 1.2 and above. */
	__u16	x_resolution;
	__u16	y_resolution;
	__u8	x_char_size;
	__u8	y_char_size;
	__u8	number_of_planes;
	__u8	bits_per_pixel;
	__u8	number_of_banks;
	__u8	memory_model;
	__u8	bank_size;
	__u8	number_of_image_pages;
	__u8	reserved0;

	/* Direct Color fields (required for Direct/6 and YUV/7 memory models */
	__u8	red_mask_size;
	__u8	red_field_position;
	__u8	green_mask_size;
	__u8	green_field_position;
	__u8	blue_mask_size;
	__u8	blue_field_position;
	__u8	reserved_mask_size;
	__u8	reserved_field_position;
	__u8	direct_color_mode_info;

	/* Mandatory information for VBE 2.0 and above.  */
	__u32	physical_base_address;
	__u32	reserved2;
	__u16	reserved3;

	/* VBE 3.0 specification says to reserve 189 bytes. Add one more
	   to have this structure have size of 256. */
	__u8	reserved4[206];
} __attribute__((packed));

#endif

