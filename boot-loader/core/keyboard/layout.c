#include "include/export.h"
#include "core/include/keyboard/keyboard.h"

/* USB HID keyboard layout. */

static bl_uint8_t bl_keyboard_layout[256] = {
	0, 0, 0, 0,

	'a', 'b', 'c', 'd',
	'e', 'f', 'g', 'h',
	'i', 'j', 'k', 'l',
	'm', 'n', 'o', 'p',
	'q', 'r', 's', 't',
	'u', 'v', 'w', 'x',
	'y', 'z',

	'1', '2', '3', '4',
	'5', '6', '7', '8',
	'9', '0',

	'\n', '\e', '\b', '\t',
	' ',

	'-', '=', '[', ']',
	'\\', '#', ';', '\'',
	'`', ',', '.', '/',
};

static bl_uint8_t bl_keyboard_layout_shift[256] = {
	0, 0, 0, 0,

	'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P',
	'Q', 'R', 's', 'T',
	'U', 'V', 'W', 'X',
	'Y', 'Z',

	'!', '@', '#', '$',
	'%', '^', '&', '*',
	'(', ')',

	'\n', '\e', '\b', '\t',
	' ',

	'_', '+', '{', '}',
	'|', '~', ':', '\"',
	'~', '<', '>', '?',
};

int bl_keyboard_layout_lookup(int key)
{
	return bl_keyboard_layout[key & 0xff];
}
BL_EXPORT_FUNC(bl_keyboard_layout_lookup);

int bl_keyboard_layout_lookup_shift(int key)
{
	return bl_keyboard_layout_shift[key & 0xff];
}
BL_EXPORT_FUNC(bl_keyboard_layout_lookup_shift);

