#ifndef BL_EDID_H
#define BL_EDID_H

#include "include/bl-types.h"

struct bl_video_edid {
	/* Header information. */
	bl_uint8_t header[8];
	bl_uint16_t manufacturer_id;
	bl_uint16_t product_id;
	bl_uint32_t serial_number;
	bl_uint8_t week_of_manufacture;
	bl_uint8_t year_of_manufacture;
	bl_uint8_t edid_version;
	bl_uint8_t edid_revision;

	/* Basic display information. */
	bl_uint8_t video_input_parameters;
	bl_uint8_t horizontal_screen_size;
	bl_uint8_t vertical_screen_size;
	bl_uint8_t display_gamma;
	bl_uint8_t supported_features;

	/* Color characteristics. */
	bl_uint8_t color_characteristics[10];

	/* Established timing bitmap. */
	bl_uint8_t established_timings[3];

	/* Standard timing information. */
	bl_uint16_t standard_timings[8];

	struct {
		bl_uint8_t info[18];
	} detailed_timings[4];

	bl_uint8_t extension_flag;
	bl_uint8_t checksum;
} __attribute__((packed));

#endif

