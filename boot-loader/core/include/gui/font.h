#ifndef BL_FONT_H
#define BL_FONT_H

#include "include/bl-types.h"

/* Character dimensions in pixels */
#define BL_FONT_CHARACTER_WIDTH		8
#define BL_FONT_CHARACTER_HEIGHT	16

/* ASCII only ! */
#define BL_FONT_ASCII_CHARACTERS	128

extern const bl_uint64_t bl_font_lookup_table2[16];
extern const bl_uint64_t bl_font_lookup_table4[4];

extern const bl_uint8_t bl_font[BL_FONT_ASCII_CHARACTERS][BL_FONT_CHARACTER_HEIGHT];

#endif

